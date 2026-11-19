// Renderer half of the --coverage spec run (see script/spec-runner.js).
// NODE_V8_COVERAGE only works in the browser and utility processes, so this
// collects V8 coverage of the internal/electron/js2c bundles from every
// renderer and worker over the Chrome DevTools Protocol, through the spec
// worker's own remote debugging port (spec/index.js turns it on), and writes
// it to the coverage directory in the same format as NODE_V8_COVERAGE.
//
// Targets are auto-attached paused, so coverage starts before Electron's
// renderer bundles run. Profiler state is per V8 isolate and detaching a
// session turns precise coverage off for the whole isolate, so only one
// session per renderer process owns coverage; another one takes over when it
// detaches.
//
// Everything here is a no-op unless startRendererCoverage() was called.

import { app, webContents } from 'electron/main';

import * as fs from 'node:fs';
import * as path from 'node:path';
import { setTimeout as sleep } from 'node:timers/promises';

const JS2C_PREFIX = 'node:internal/electron/js2c/';
// How long to wait for one renderer's coverage; a hung renderer must not hang
// the spec.
const TAKE_TIMEOUT = 2_000;
const CONNECT_TIMEOUT = 30_000;

interface Session {
  sessionId: string;
  targetId: string;
  owner: boolean;
  // Started while coverage was suspended, see suspendRendererCoverage().
  excluded: boolean;
  // Last known renderer process, see rendererPid().
  pid?: number;
}

// Service workers are left alone: Chromium keeps a service worker running
// while any DevTools session is attached to it (no idle termination, and an
// unregistered one is only stopped once DevTools detaches), which breaks the
// specs of their lifecycle. A filter replaces Chromium's default one, which
// leaves out the browser and tab targets.
const autoAttach = {
  autoAttach: true,
  waitForDebuggerOnStart: true,
  flatten: true,
  filter: [
    { type: 'browser', exclude: true },
    { type: 'tab', exclude: true },
    { type: 'service_worker', exclude: true },
    {}
  ]
};
const coverageTargetTypes = new Set(['page', 'iframe', 'webview', 'worker', 'shared_worker']);

let coverageDir: string | undefined;
let socket: WebSocket | undefined;
let nextId = 1;
let takes = 0;
let suspended = false;
const pending = new Map<number, { resolve: (result: any) => void; reject: (error: Error) => void }>();
const sessions = new Map<string, Session>();

export function devToolsActivePortFile() {
  return path.join(app.getPath('sessionData'), 'DevToolsActivePort');
}

function send(method: string, params: object = {}, sessionId?: string): Promise<any> {
  if (!socket || socket.readyState !== WebSocket.OPEN) return Promise.reject(new Error('Not connected'));
  const id = nextId++;
  const result = new Promise<any>((resolve, reject) => pending.set(id, { resolve, reject }));
  socket.send(JSON.stringify({ id, method, params, sessionId }));
  return result;
}

// For commands whose result nobody waits for; their target may go away first.
function post(method: string, params: object, sessionId: string) {
  send(method, params, sessionId).catch(() => {});
}

// The renderer process of a WebContents target (it changes on cross-process
// navigations), or the last one seen once the WebContents is gone. Other
// targets (workers, out-of-process iframes) have none and are assumed to be
// alone in their isolate.
function rendererPid(session: Session) {
  const pid = webContents.fromDevToolsTargetId(session.targetId)?.getOSProcessId();
  if (pid) session.pid = pid;
  return session.pid;
}

function startCoverage(session: Session) {
  session.owner = true;
  post('Profiler.enable', {}, session.sessionId);
  post('Profiler.startPreciseCoverage', { callCount: true, detailed: true }, session.sessionId);
}

function onAttached({ sessionId, targetInfo }: { sessionId: string; targetInfo: { type: string; targetId: string } }) {
  const session: Session = { sessionId, targetId: targetInfo.targetId, owner: false, excluded: suspended };
  if (!suspended && coverageTargetTypes.has(targetInfo.type)) {
    const pid = rendererPid(session);
    const hasOwner =
      pid !== undefined && [...sessions.values()].some((other) => other.owner && rendererPid(other) === pid);
    if (!hasOwner) startCoverage(session);
  }
  sessions.set(sessionId, session);
  // The target waits for this session to resume it; awaiting the commands
  // above first would deadlock on targets that only answer once running.
  post('Target.setAutoAttach', autoAttach, sessionId);
  post('Runtime.runIfWaitingForDebugger', {}, sessionId);
}

function onDetached({ sessionId }: { sessionId: string }) {
  const session = sessions.get(sessionId);
  sessions.delete(sessionId);
  if (!session?.owner) return;
  const pid = rendererPid(session);
  if (pid === undefined) return;
  const heir = [...sessions.values()].find((other) => !other.owner && !other.excluded && rendererPid(other) === pid);
  if (heir) startCoverage(heir);
}

function onMessage(data: string) {
  const message = JSON.parse(data);
  if (message.id !== undefined) {
    const request = pending.get(message.id);
    pending.delete(message.id);
    if (message.error) request?.reject(new Error(message.error.message));
    else request?.resolve(message.result);
  } else if (message.method === 'Target.attachedToTarget') {
    onAttached(message.params);
  } else if (message.method === 'Target.detachedFromTarget') {
    onDetached(message.params);
  }
}

// Connects to this process's remote debugging port and starts collecting.
// Call once the app is ready.
export async function startRendererCoverage(dir: string) {
  coverageDir = dir;
  fs.mkdirSync(coverageDir, { recursive: true });

  // Chromium writes the port file once the server is listening.
  const portFile = devToolsActivePortFile();
  const deadline = Date.now() + CONNECT_TIMEOUT;
  let port: string | undefined;
  let browserPath: string | undefined;
  while (!browserPath) {
    try {
      [port, browserPath] = fs.readFileSync(portFile, 'utf8').split('\n');
    } catch {}
    if (browserPath) break;
    if (Date.now() > deadline) throw new Error(`Renderer coverage: ${portFile} did not appear`);
    await sleep(50);
  }

  const ws = new WebSocket(`ws://127.0.0.1:${port}${browserPath}`);
  await new Promise((resolve, reject) => {
    ws.addEventListener('open', resolve, { once: true });
    ws.addEventListener('error', () => reject(new Error(`Renderer coverage: cannot connect to port ${port}`)), {
      once: true
    });
  });
  ws.addEventListener('message', (event) => onMessage(String(event.data)));
  ws.addEventListener('close', () => {
    for (const request of pending.values()) request.reject(new Error('Connection closed'));
    pending.clear();
    sessions.clear();
  });
  socket = ws;
  await send('Target.setDiscoverTargets', { discover: true });
  await send('Target.setAutoAttach', autoAttach);
}

// Renderers and workers that start while coverage is suspended run without
// it, for specs of what V8 turns off under precise coverage: it neither
// consumes code caches nor puts compiled code into the ones it produces.
export function suspendRendererCoverage(suspend: boolean) {
  suspended = suspend;
}

function withTimeout<T>(promise: Promise<T>, ms: number): Promise<T | undefined> {
  let timer: NodeJS.Timeout;
  return Promise.race([
    promise,
    new Promise<undefined>((resolve) => {
      timer = setTimeout(() => resolve(undefined), ms);
    })
  ]).finally(() => clearTimeout(timer));
}

// Writes the coverage the renderers have collected since the last take (V8
// resets the counters on each take): of the renderer process of `contents`
// and of every target not tied to a WebContents, or of everything when no
// `contents` is given. Call before closing a window, as the coverage of a
// renderer is gone with it.
export async function takeRendererCoverage(contents?: Electron.WebContents) {
  if (!socket || !coverageDir) return;
  let owners = [...sessions.values()].filter((session) => session.owner);
  if (contents && !contents.isDestroyed()) {
    const pid = contents.getOSProcessId();
    owners = owners.filter((session) => {
      const sessionPid = rendererPid(session);
      return sessionPid === undefined || sessionPid === pid;
    });
  }
  await Promise.all(
    owners.map(async (session) => {
      const coverage = await withTimeout(
        send('Profiler.takePreciseCoverage', {}, session.sessionId).catch(() => undefined),
        TAKE_TIMEOUT
      );
      const result = (coverage?.result ?? []).filter((script: { url: string }) => script.url.startsWith(JS2C_PREFIX));
      if (result.length === 0) return;
      // One file per isolate, as script ids are only unique within one.
      const file = path.join(coverageDir!, `coverage-renderer-${process.pid}-${Date.now()}-${++takes}.json`);
      await fs.promises.writeFile(file, JSON.stringify({ result }));
    })
  );
}

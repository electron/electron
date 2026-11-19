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
// Taking coverage sits on the path of every window a spec closes, so it only
// asks the targets of that window's renderer, and only ones that can answer:
// a command to a WebContents without a live renderer waits for one, and one
// to a target that goes away is never answered (both would cost the whole
// TAKE_TIMEOUT).
//
// Everything here is a no-op unless startRendererCoverage() was called.

import { app, BaseWindow, BrowserWindow, webContents } from 'electron/main';

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
  // The session that auto-attached this one (a worker's page, an out-of-process
  // iframe's parent), if any.
  parent?: string;
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
const pending = new Map<
  number,
  { sessionId?: string; resolve: (result: any) => void; reject: (error: Error) => void }
>();
const sessions = new Map<string, Session>();

export function devToolsActivePortFile() {
  return path.join(app.getPath('sessionData'), 'DevToolsActivePort');
}

function send(method: string, params: object = {}, sessionId?: string): Promise<any> {
  if (!socket || socket.readyState !== WebSocket.OPEN) return Promise.reject(new Error('Not connected'));
  const id = nextId++;
  const result = new Promise<any>((resolve, reject) => pending.set(id, { sessionId, resolve, reject }));
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

// The WebContents a target belongs to: its own, or that of the session that
// attached it (e.g. the page of a dedicated worker). Undefined for targets
// attached at the browser level that are not WebContents (shared workers).
function owningContents(session: Session): Electron.WebContents | undefined {
  let current: Session | undefined = session;
  while (current) {
    const contents = webContents.fromDevToolsTargetId(current.targetId);
    if (contents) return contents;
    current = current.parent ? sessions.get(current.parent) : undefined;
  }
  return undefined;
}

function startCoverage(session: Session) {
  session.owner = true;
  post('Profiler.enable', {}, session.sessionId);
  post('Profiler.startPreciseCoverage', { callCount: true, detailed: true }, session.sessionId);
}

function onAttached(
  { sessionId, targetInfo }: { sessionId: string; targetInfo: { type: string; targetId: string } },
  parent?: string
) {
  const session: Session = { sessionId, targetId: targetInfo.targetId, owner: false, excluded: suspended, parent };
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
  // Chromium drops the commands a detached session had not answered yet.
  for (const [id, request] of pending) {
    if (request.sessionId !== sessionId) continue;
    pending.delete(id);
    request.reject(new Error('Target detached'));
  }
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
    // Flattened sessions: the event comes from the session that attached it.
    onAttached(message.params, message.sessionId);
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

  for (const prototype of new Set([BrowserWindow.prototype, BaseWindow.prototype])) {
    takeBeforeClose(prototype, windowContents);
  }
  const { WebContents } = (process as any)._linkedBinding('electron_browser_web_contents');
  takeBeforeClose(WebContents.prototype, (contents) => [contents]);
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

// The owning sessions to take coverage from: of the renderer processes of
// `contents` and of the targets those attached (workers, out-of-process
// iframes), or of every target when no `contents` is given. Only ones that
// can answer, see above.
function takeTargets(contents?: Electron.WebContents[]) {
  let pids: Set<number> | undefined;
  if (contents) {
    pids = new Set(
      contents
        .filter((c) => !c.isDestroyed())
        .map((c) => c.getOSProcessId())
        .filter((pid) => pid !== 0)
    );
    if (pids.size === 0) return [];
  }
  return [...sessions.values()].filter((session) => {
    if (!session.owner) return false;
    const owning = owningContents(session);
    if (!owning) return !pids;
    // Without a renderer (none yet, or it is gone) nothing would answer.
    if (owning.isDestroyed()) return false;
    const owningPid = owning.getOSProcessId();
    return owningPid !== 0 && (!pids || pids.has(owningPid));
  });
}

async function take(owners: Session[]) {
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

// Writes the coverage the renderers have collected since the last take (V8
// resets the counters on each take): of the renderer processes of `contents`,
// or of every renderer when none is given. Call before closing a window, as
// the coverage of a renderer is gone with it.
export async function takeRendererCoverage(contents?: Electron.WebContents | Electron.WebContents[]) {
  if (!socket || !coverageDir) return;
  await take(takeTargets(contents === undefined ? undefined : [contents].flat()));
}

// The WebContents that go away with these: they and the <webview> guests
// they embed.
function withGuests(contents: Electron.WebContents[]) {
  const all = new Set(contents);
  const others = webContents.getAllWebContents();
  // A Set visits what is added while iterating it, so this finds guests of
  // guests too.
  for (const host of all) {
    for (const other of others) {
      if (!other.isDestroyed() && other.hostWebContents === host) all.add(other);
    }
  }
  return [...all];
}

// A BrowserWindow takes its child windows down with it.
function windowContents(window: Electron.BaseWindow): Electron.WebContents[] {
  if (window.isDestroyed()) return [];
  const own = window instanceof BrowserWindow && window.webContents ? [window.webContents] : [];
  return [...own, ...window.getChildWindows().flatMap(windowContents)];
}

const destroying = new WeakSet<object>();

// Specs close windows and WebContents in many ways besides closeWindow() and
// cleanupWebContents() (spec/lib/window-helpers.ts), which take the coverage
// of their renderers first: useRemoteContext(), defer(() => w.close()) and
// hundreds of direct calls. So close() takes it too, and only then closes.
// close() returns nothing and finishes asynchronously anyway (it runs
// beforeunload and unload in the renderer first); what changes is that its
// 'close' events come a renderer round trip later. Without a live renderer
// to ask it closes at once.
// destroy() cannot wait, it has to be done when it returns, so what specs
// destroy() themselves keeps losing its renderer's coverage since its last
// take. Over the whole suite that is next to nothing: also taking every
// renderer's coverage after each test (before afterEach hooks) found no more.
function takeBeforeClose(prototype: any, contentsOf: (self: any) => Electron.WebContents[]) {
  const { close, destroy } = prototype;
  if (Object.hasOwn(prototype, 'close')) {
    prototype.close = function (this: any, ...args: unknown[]) {
      const owners = this.isDestroyed() ? [] : takeTargets(withGuests(contentsOf(this)));
      if (owners.length === 0) return close.apply(this, args);
      take(owners).finally(() => {
        if (!destroying.has(this) && !this.isDestroyed()) close.apply(this, args);
      });
    };
  }
  if (Object.hasOwn(prototype, 'destroy')) {
    prototype.destroy = function (this: any, ...args: unknown[]) {
      destroying.add(this);
      return destroy.apply(this, args);
    };
  }
}

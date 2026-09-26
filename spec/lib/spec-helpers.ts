import type { BrowserWindow } from 'electron/main';

import { afterAll, beforeAll, chai, describe, it } from 'vitest';

import * as childProcess from 'node:child_process';
import { once } from 'node:events';
import * as http from 'node:http';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';
import * as url from 'node:url';
import { stripVTControlCharacters } from 'node:util';
import * as v8 from 'node:v8';

import type * as http2 from 'node:http2';
import type * as https from 'node:https';
import type * as net from 'node:net';

export const ifit = (condition: boolean) => (condition ? it : it.skip);
export const ifdescribe = (condition: boolean) => (condition ? describe : describe.skip);

export const isWayland =
  process.platform === 'linux' &&
  (process.env.XDG_SESSION_TYPE === 'wayland' ||
    !!process.env.WAYLAND_DISPLAY ||
    process.argv.includes('--ozone-platform=wayland'));

// macos-x64 CI runner VMs have no Metal-capable GPU, and SwiftShader's Vulkan
// backend fails to initialize there too, so every GPU process launch fails
// until Chromium falls back to software compositing with GL disabled. Start
// spawned apps in that end state directly so they skip the failed launches.
export const ciGpuArgs: string[] =
  process.env.CI && process.platform === 'darwin' && process.arch === 'x64' ? ['--disable-gpu'] : [];

type CleanupFunction = (() => void) | (() => Promise<void>);
const cleanupFunctions: CleanupFunction[] = [];
export async function runCleanupFunctions() {
  // Take the whole list up front: a cleanup that throws must not leave the
  // rest behind to run (and throw) again after every later test.
  const pending = cleanupFunctions.splice(0, cleanupFunctions.length);
  const errors: unknown[] = [];
  for (const cleanup of pending) {
    try {
      await cleanup();
    } catch (error) {
      errors.push(error);
    }
  }
  if (errors.length === 1) throw errors[0];
  if (errors.length > 1) throw new AggregateError(errors, 'Several defer()-ed cleanup functions failed');
}

export function defer(f: CleanupFunction) {
  cleanupFunctions.unshift(f);
}

class RemoteControlApp {
  process: childProcess.ChildProcess;
  port: number;

  constructor(proc: childProcess.ChildProcess, port: number) {
    this.process = proc;
    this.port = port;
  }

  remoteEval = (js: string): Promise<any> => {
    return new Promise((resolve, reject) => {
      const req = http.request(
        {
          host: '127.0.0.1',
          port: this.port,
          method: 'POST'
        },
        (res) => {
          const chunks = [] as Buffer[];
          res.on('data', (chunk) => {
            chunks.push(chunk);
          });
          res.on('end', () => {
            const ret = v8.deserialize(Buffer.concat(chunks));
            if (Object.hasOwn(ret, 'error')) {
              reject(new Error(`remote error: ${ret.error}\n\nTriggered at:`));
            } else {
              resolve(ret.result);
            }
          });
        }
      );
      req.on('error', reject);
      req.write(js);
      req.end();
    });
  };

  remotely = (script: Function, ...args: any[]): Promise<any> => {
    return this.remoteEval(`(${script})(...${JSON.stringify(args)})`);
  };
}

export async function startRemoteControlApp(extraArgs: string[] = [], options?: childProcess.SpawnOptionsWithoutStdio) {
  const appPath = path.join(import.meta.dirname, '..', 'fixtures', 'apps', 'remote-control');
  const appProcess = childProcess.spawn(process.execPath, [appPath, ...ciGpuArgs, ...extraArgs], options);
  // Register cleanup before awaiting the port so a stalled startup that trips
  // the test's timeout doesn't leak the child into the in-job retry.
  defer(() => {
    if (appProcess.exitCode === null && appProcess.signalCode === null) {
      appProcess.kill('SIGINT');
    }
  });
  appProcess.stderr.on('data', (d) => {
    process.stderr.write(d);
  });
  const port = await new Promise<number>((resolve) => {
    appProcess.stdout.on('data', (d) => {
      const m = /Listening: (\d+)/.exec(d.toString());
      if (m && m[1] != null) {
        resolve(Number(m[1]));
      }
    });
  });
  return new RemoteControlApp(appProcess, port);
}

export interface SpawnAndWaitResult {
  code: number | null;
  signal: NodeJS.Signals | null;
  stdout: string;
  stderr: string;
}

export interface SpawnAndWaitOptions extends childProcess.SpawnOptionsWithoutStdio {
  timeout: number;
  killTimeout?: number;
  stripOutput?: boolean;
}

function formatSpawnOutput(stdout: Buffer[], stderr: Buffer[], stripOutput = false) {
  const normalize = (chunks: Buffer[]) => {
    const output = Buffer.concat(chunks).toString().trim();
    return stripOutput ? stripVTControlCharacters(output) : output;
  };

  return {
    stdout: normalize(stdout),
    stderr: normalize(stderr)
  };
}

export async function spawnAndWait(
  command: string,
  args: string[],
  options: SpawnAndWaitOptions
): Promise<SpawnAndWaitResult> {
  const { timeout, killTimeout = 5000, stripOutput, ...spawnOptions } = options;
  const child = childProcess.spawn(command, args, spawnOptions);
  const stdout: Buffer[] = [];
  const stderr: Buffer[] = [];
  child.stdout.on('data', (chunk) => stdout.push(chunk));
  child.stderr.on('data', (chunk) => stderr.push(chunk));

  let timedOut = false;
  let killTimeoutId: NodeJS.Timeout | undefined;
  const timeoutId = globalThis.setTimeout(() => {
    timedOut = true;
    child.kill();
    killTimeoutId = globalThis.setTimeout(() => {
      child.kill('SIGKILL');
    }, killTimeout);
  }, timeout);

  const [code, signal] = await new Promise<[number | null, NodeJS.Signals | null]>((resolve, reject) => {
    child.on('error', reject);
    child.on('close', (code, signal) => {
      resolve([code, signal]);
    });
  }).finally(() => {
    globalThis.clearTimeout(timeoutId);
    if (killTimeoutId) globalThis.clearTimeout(killTimeoutId);
  });

  const output = formatSpawnOutput(stdout, stderr, stripOutput);
  if (timedOut) {
    throw new Error(
      `Timed out after ${timeout}ms waiting for ${command} ${args.join(' ')} to exit. Process closed with code ${code} and signal ${signal}.\n\nstdout:\n${output.stdout}\n\nstderr:\n${output.stderr}`
    );
  }

  return {
    code,
    signal,
    ...output
  };
}

export function waitUntil(callback: () => boolean | Promise<boolean>, opts: { rate?: number; timeout?: number } = {}) {
  const { rate = 10, timeout = 10000 } = opts;
  return (async () => {
    const ac = new AbortController();
    const signal = ac.signal;
    let checkCompleted = false;
    let timedOut = false;

    const check = async () => {
      let result;

      try {
        result = await callback();
      } catch (e) {
        ac.abort();
        throw e;
      }

      return result;
    };

    setTimeout(timeout, { signal }).then(() => {
      timedOut = true;
      checkCompleted = true;
    });

    while (checkCompleted === false) {
      const checkSatisfied = await check();
      if (checkSatisfied === true) {
        ac.abort();
        checkCompleted = true;
        return;
      } else {
        await setTimeout(rate);
      }
    }

    if (timedOut) {
      throw new Error(`waitUntil timed out after ${timeout}ms`);
    }
  })();
}

export async function repeatedly<T>(fn: () => Promise<T>, opts?: { until?: (x: T) => boolean; timeLimit?: number }) {
  const { until = (x: T) => !!x, timeLimit = 10000 } = opts ?? {};
  const begin = Date.now();
  while (true) {
    const ret = await fn();
    if (until(ret)) {
      return ret;
    }
    if (Date.now() - begin > timeLimit) {
      throw new Error(`repeatedly timed out (limit=${timeLimit})`);
    }
  }
}

async function makeRemoteContext(opts?: any) {
  // Resolved here rather than with a top-level import so that this file stays
  // loadable in a utility process (see fixtures/api/utility-process/api-net-spec.js),
  // whose 'electron/main' has no BrowserWindow export.
  const { BrowserWindow } = await import('electron/main');
  const { webPreferences, setup, url = 'about:blank', ...rest } = opts ?? {};
  const w = new BrowserWindow({
    show: false,
    webPreferences: { nodeIntegration: true, contextIsolation: false, ...webPreferences },
    ...rest
  });
  await w.loadURL(url.toString());
  if (setup) await w.webContents.executeJavaScript(setup);
  return w;
}

const remoteContext: BrowserWindow[] = [];
export async function getRemoteContext() {
  if (remoteContext.length) {
    return remoteContext[0];
  }
  const w = await makeRemoteContext();
  defer(() => w.close());
  return w;
}

export function useRemoteContext(opts?: any) {
  beforeAll(async () => {
    remoteContext.unshift(await makeRemoteContext(opts));
  });
  afterAll(() => {
    const w = remoteContext.shift();
    w!.close();
  });
}

// See vitest-cjs.cjs: the remote function runs in a CommonJS context.
const vitestForRequire = path.join(import.meta.dirname, 'vitest-cjs.cjs');

async function runRemote(type: 'skip' | 'none' | 'only', name: string, fn: Function, args?: any[]) {
  const wrapped = async () => {
    const w = await getRemoteContext();
    const { ok, message } = await w.webContents.executeJavaScript(`(async () => {
      try {
        const { expect } = require(${JSON.stringify(vitestForRequire)})
        await (${fn})(...${JSON.stringify(args ?? [])})
        return {ok: true};
      } catch (e) {
        return {ok: false, message: e.message}
      }
    })()`);
    if (!ok) {
      throw new chai.AssertionError(message);
    }
  };

  let runFn: any = it;
  if (type === 'only') {
    // oxlint-disable-next-line no-only-tests/no-only-tests
    runFn = it.only;
  } else if (type === 'skip') {
    runFn = it.skip;
  }

  runFn(name, wrapped);
}

export const itremote = Object.assign(
  (name: string, fn: Function, args?: any[]) => {
    runRemote('none', name, fn, args);
  },
  {
    only: (name: string, fn: Function, args?: any[]) => {
      runRemote('only', name, fn, args);
    },
    skip: (name: string, fn: Function, args?: any[]) => {
      runRemote('skip', name, fn, args);
    }
  }
);

export async function listen(server: http.Server | https.Server | http2.Http2SecureServer) {
  const hostname = '127.0.0.1';
  await new Promise<void>((resolve) => server.listen(0, hostname, () => resolve()));
  const { port } = server.address() as net.AddressInfo;
  const protocol = server instanceof http.Server ? 'http' : 'https';
  return { port, hostname, url: url.format({ protocol, hostname, port }) };
}

export function isTestingBindingAvailable() {
  try {
    process._linkedBinding('electron_common_testing');
    return true;
  } catch {
    return false;
  }
}

export function deferKillUtilityProcess(utilityProcess: Electron.UtilityProcess) {
  defer(async () => {
    if (utilityProcess.pid) {
      const exit = once(utilityProcess, 'exit');
      utilityProcess.kill();
      await exit;
    }
  });
}

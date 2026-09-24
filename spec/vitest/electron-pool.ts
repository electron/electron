// A vitest pool whose workers are Electron browser processes instead of Node
// child processes, so that specs run in the Electron main process exactly as
// they did under the mocha runner. Each worker is `electron spec/
// --vitest-worker`; spec/index.js does the usual pre-`ready` setup and then
// hands the process to vitest's worker runtime (spec/vitest/worker.ts), which
// talks to this side over the Node IPC channel.

import * as childProcess from 'node:child_process';
import { EventEmitter } from 'node:events';
import * as path from 'node:path';

import { deserialize, serialize, WORKER_READY_MESSAGE } from './ipc-serialization.ts';

import type { PoolOptions, PoolRunnerInitializer, PoolWorker, WorkerRequest } from 'vitest/node';

export interface ElectronPoolOptions {
  /** Path to the Electron executable that runs the specs. */
  electronPath: string;
  /** The spec app directory passed to Electron (contains package.json/index.js). */
  specDir: string;
  /** Extra command line switches for every worker (e.g. --no-sandbox in CI containers). */
  electronArgs?: string[];
  /**
   * A wrapper to launch Electron through, e.g. ['python3', 'dbus_mock.py'].
   * The Electron path and its arguments are appended.
   */
  launcher?: string[];
}

const SIGKILL_TIMEOUT = 5_000;
// Time for Electron to start and reach app 'ready' (sanitizer builds are slow).
const START_TIMEOUT = 3 * 60_000;
// How long to wait for a stopped worker's output pipes to drain; a process
// the worker leaked can hold them open indefinitely.
const FLUSH_TIMEOUT = 5_000;

let nextWorkerId = 1;

export function electronPool(poolOptions: ElectronPoolOptions): PoolRunnerInitializer {
  return {
    name: 'electron',
    createPoolWorker: (options) => new ElectronPoolWorker(options, poolOptions)
  };
}

class ElectronPoolWorker implements PoolWorker {
  name = 'electron';
  // Same as the forks pool: the worker reads specs from disk itself, so the
  // main process may serve it cached fs results.
  cacheFs = true;

  private child: childProcess.ChildProcess | undefined;
  private readonly errors = new EventEmitter();
  private readonly env: Partial<NodeJS.ProcessEnv>;
  private readonly stdout: NodeJS.WritableStream;
  private readonly stderr: NodeJS.WritableStream;
  private readonly id = nextWorkerId++;
  private readonly electron: ElectronPoolOptions;

  constructor(options: PoolOptions, electron: ElectronPoolOptions) {
    this.electron = electron;
    this.env = options.env;
    this.stdout = options.project.vitest.logger.outputStream;
    this.stderr = options.project.vitest.logger.errorStream;
  }

  on(event: string, callback: (arg: any) => void): void {
    if (event === 'error') this.errors.on('error', callback);
    else this.proc.on(event, callback);
  }

  off(event: string, callback: (arg: any) => void): void {
    if (event === 'error') this.errors.off('error', callback);
    else this.child?.off(event, callback);
  }

  send(message: WorkerRequest): void {
    this.proc.send(serialize(message), (error) => {
      if (error) this.emitError(error);
    });
  }

  deserialize(data: unknown): unknown {
    return deserialize(data);
  }

  async start(): Promise<void> {
    if (this.child) return;

    const env: NodeJS.ProcessEnv = {
      ...this.env,
      ELECTRON_SPEC_WORKER_ID: String(this.id)
    };
    // Whatever the CLI's environment says, the workers must be real browser
    // processes.
    delete env.ELECTRON_RUN_AS_NODE;

    // Chromium truncates its --log-file on startup, so workers cannot share
    // one; give each its own next to the requested path.
    const electronArgs = (this.electron.electronArgs ?? []).map((arg) =>
      arg.startsWith('--log-file=') ? arg.replace(/(\.[^./\\]+)?$/, (ext) => `.worker-${this.id}${ext}`) : arg
    );
    const [command, ...prefix] = [
      ...(this.electron.launcher ?? []),
      this.electron.electronPath,
      this.electron.specDir,
      '--vitest-worker',
      ...electronArgs
    ];
    const child = childProcess.spawn(command, prefix, {
      cwd: path.dirname(this.electron.specDir),
      env,
      stdio: ['ignore', 'pipe', 'pipe', 'ipc'],
      serialization: 'json',
      // Lead a process group so that anything a spec spawned and leaked can be
      // reaped together with the worker (see stop()).
      detached: process.platform !== 'win32'
    });
    this.child = child;
    child.on('error', this.emitError);

    // The logger streams are shared by every worker; one worker ending must
    // not end them for the others.
    this.stdout.setMaxListeners(1 + this.stdout.getMaxListeners());
    this.stderr.setMaxListeners(1 + this.stderr.getMaxListeners());
    child.stdout!.pipe(this.stdout, { end: false });
    child.stderr!.pipe(this.stderr, { end: false });

    // vitest posts its 'start' request as soon as this resolves. Electron only
    // attaches the worker runtime once the app is ready, so wait for it to say
    // so rather than relying on the IPC channel buffering the request.
    await new Promise<void>((resolve, reject) => {
      const cleanup = () => {
        clearTimeout(timer);
        child.off('message', onMessage);
        child.off('exit', onExit);
        child.off('error', onError);
      };
      const onMessage = (message: unknown) => {
        if (message !== WORKER_READY_MESSAGE) return;
        cleanup();
        resolve();
      };
      const onExit = (code: number | null, signal: NodeJS.Signals | null) => {
        cleanup();
        reject(new Error(`Electron spec worker exited before it was ready (code ${code}, signal ${signal})`));
      };
      const onError = (error: Error) => {
        cleanup();
        reject(error);
      };
      const timer = setTimeout(() => {
        cleanup();
        this.killGroup(child);
        reject(new Error(`Electron spec worker did not become ready within ${START_TIMEOUT / 1000}s`));
      }, START_TIMEOUT);
      child.on('message', onMessage);
      child.once('exit', onExit);
      child.once('error', onError);
    });
  }

  // Kills the worker and, outside Windows, whatever else is left in its
  // process group (a fixture app a spec leaked, say).
  private killGroup(child: childProcess.ChildProcess) {
    try {
      if (process.platform !== 'win32' && child.pid !== undefined) process.kill(-child.pid, 'SIGKILL');
      else child.kill('SIGKILL');
    } catch {
      // Nothing left, which is the normal case.
    }
  }

  async stop(): Promise<void> {
    const child = this.child;
    if (!child) return;
    this.child = undefined;

    const exited = new Promise<void>((resolve) => {
      if (child.exitCode !== null || child.signalCode !== null) resolve();
      else child.once('exit', () => resolve());
    });
    // The worker calls app.quit() once vitest has told it to stop; only step in
    // if that does not happen.
    const sigterm = setTimeout(() => child.kill(), SIGKILL_TIMEOUT / 2);
    const sigkill = setTimeout(() => child.kill('SIGKILL'), SIGKILL_TIMEOUT);
    await exited;
    clearTimeout(sigterm);
    clearTimeout(sigkill);
    // Do not let anything a spec leaked outlive the file.
    this.killGroup(child);

    // Give the pipes a moment to drain, but no more: a process that escaped
    // the group could otherwise hold them open for the rest of the run.
    let flushTimer: NodeJS.Timeout | undefined;
    await Promise.race([
      Promise.all([streamFlushed(child.stdout!), streamFlushed(child.stderr!)]),
      new Promise((resolve) => {
        flushTimer = setTimeout(resolve, FLUSH_TIMEOUT);
      })
    ]);
    clearTimeout(flushTimer);
    child.stdout!.destroy();
    child.stderr!.destroy();
    child.stdout!.unpipe(this.stdout);
    child.stderr!.unpipe(this.stderr);
    this.stdout.setMaxListeners(this.stdout.getMaxListeners() - 1);
    this.stderr.setMaxListeners(this.stderr.getMaxListeners() - 1);
  }

  private readonly emitError = (error: Error): void => {
    this.errors.emit('error', error);
  };

  private get proc(): childProcess.ChildProcess {
    if (!this.child) {
      throw new Error('The Electron spec worker was torn down or never started.');
    }
    return this.child;
  }
}

function streamFlushed(stream: NodeJS.ReadableStream): Promise<void> {
  return new Promise((resolve) => {
    if ((stream as any).readableEnded || (stream as any).destroyed) return resolve();
    stream.once('end', () => resolve());
    stream.once('close', () => resolve());
  });
}

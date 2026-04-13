import { ForksPoolWorker, type PoolOptions, type WorkerRequest } from 'vitest/node';

import { spawn, type ChildProcess } from 'node:child_process';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';

import { getAbsoluteElectronExec } from '../../script/lib/utils';

/**
 * A vitest PoolWorker that spawns each worker as a full Electron main
 * process (not a node-mode fork) so tests can exercise real Electron APIs.
 *
 * We extend the built-in ForksPoolWorker so we inherit all IPC / teardown
 * plumbing and only override how the child process is created.
 */
class ElectronPoolWorker extends ForksPoolWorker {
  override name = 'electron';

  private readonly distPath: string;

  constructor(options: PoolOptions) {
    super(options);
    this.distPath = options.distPath;
  }

  private userDataDir: string | undefined;

  override async start() {
    if ((this as any)._fork) return;

    const electronExec = process.env.ELECTRON_TESTS_EXECUTABLE || getAbsoluteElectronExec();
    if (!fs.existsSync(electronExec)) {
      throw new Error(
        `Electron binary not found at '${electronExec}'. ` +
          `Build Electron first (e build) or set ELECTRON_TESTS_EXECUTABLE.`
      );
    }

    // Allocate a guaranteed-unique userData dir for this worker. The pool owns
    // its lifecycle so it can be removed once the worker exits.
    this.userDataDir = fs.mkdtempSync(path.join(os.tmpdir(), 'electron-vitest-'));

    const env = {
      ...(this as any).env,
      // Make sure the child is a real Electron main process, not node-mode.
      ELECTRON_RUN_AS_NODE: undefined,
      ELECTRON_DISABLE_SECURITY_WARNINGS: 'true',
      // Tell worker-entry.js where vitest's fork worker lives.
      VITEST_DIST_PATH: this.distPath,
      ELECTRON_VITEST_USER_DATA_DIR: this.userDataDir
    };

    const extraArgs = process.env.ELECTRON_EXTRA_ARGS ? process.env.ELECTRON_EXTRA_ARGS.split(' ').filter(Boolean) : [];

    // spawn() + an 'ipc' stdio slot gives the child process.send/on('message'),
    // which is what vitest's fork worker protocol relies on.
    const child: ChildProcess = spawn(electronExec, [path.resolve(__dirname), ...extraArgs], {
      env,
      execArgv: (this as any).execArgv,
      stdio: ['pipe', 'pipe', 'pipe', 'ipc'],
      serialization: 'advanced'
    } as any);

    (this as any)._fork = child;

    const stdout = (this as any).stdout;
    const stderr = (this as any).stderr;
    if (child.stdout) {
      stdout.setMaxListeners(1 + stdout.getMaxListeners());
      child.stdout.pipe(stdout);
    }
    if (child.stderr) {
      stderr.setMaxListeners(1 + stderr.getMaxListeners());
      child.stderr.pipe(stderr);
    }

    child.once('exit', (code, signal) => {
      if (!this.stopping && (code !== 0 || signal)) {
        console.error(
          `[electron-pool] worker pid=${child.pid} exited unexpectedly (code=${code} signal=${signal}) ` +
            `while running: ${this.lastFiles.join(', ') || '<no files assigned yet>'}`
        );
      }
    });
  }

  private lastFiles: string[] = [];
  private stopping = false;

  override send(message: WorkerRequest) {
    const files = (message as { files?: string[] } | undefined)?.files;
    if (Array.isArray(files)) this.lastFiles = files;
    super.send(message);
  }

  override async stop() {
    this.stopping = true;
    try {
      await super.stop();
    } finally {
      if (this.userDataDir) {
        fs.rmSync(this.userDataDir, { recursive: true, force: true });
        this.userDataDir = undefined;
      }
    }
  }
}

export default {
  name: 'electron',
  createPoolWorker: (options: PoolOptions) => new ElectronPoolWorker(options)
};

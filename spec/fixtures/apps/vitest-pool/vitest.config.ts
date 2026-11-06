import { defineConfig } from 'vitest/config';

import * as fs from 'node:fs';
import * as path from 'node:path';

import { electronPool } from '../../../vitest/electron-pool.ts';

import type { PoolRunnerInitializer } from 'vitest/node';

// A vitest run of its own, started by spec/vitest-pool.spec.ts on the Node.js
// that runs the spec suite's vitest, with workers on the same Electron.
const root = import.meta.dirname;
const electronPath = process.env.ELECTRON_SPEC_ELECTRON_PATH;
const tmpDir = process.env.ELECTRON_SPEC_FIXTURE_TMP_DIR;
if (!electronPath || !tmpDir) {
  throw new Error(
    'Run by spec/vitest-pool.spec.ts, which sets ELECTRON_SPEC_ELECTRON_PATH and ELECTRON_SPEC_FIXTURE_TMP_DIR'
  );
}
const timeout = Number(process.env.ELECTRON_SPEC_TIMEOUT) || 30_000;
// As in spec/vitest.config.ts, unless the spec asks for a shorter one.
const quitTimeout = Number(process.env.ELECTRON_SPEC_FIXTURE_QUIT_TIMEOUT) || timeout / 6;

// The Electron pool, which also tells wait.fixture.ts once this vitest has
// handled a worker's exit. The listener is added after the pool's own, which
// reports a failed quit.
const pool = electronPool({ electronPath, specDir: root, quitTimeout });
const poolWithExitMarker: PoolRunnerInitializer = {
  name: pool.name,
  createPoolWorker(options) {
    const worker = pool.createPoolWorker(options);
    const start = worker.start.bind(worker);
    worker.start = async () => {
      await start();
      worker.on('exit', () => fs.writeFileSync(path.join(tmpDir, 'worker-exited'), ''));
    };
    return worker;
  }
};

export default defineConfig({
  // Keep vitest's results cache out of the source tree.
  cacheDir: path.join(tmpDir, 'cache'),
  test: {
    root,
    include: ['*.fixture.ts'],
    experimental: {
      viteModuleRunner: false,
      nodeLoader: false
    },
    pool: poolWithExitMarker,
    // wait.fixture.ts runs beside quit.fixture.ts.
    maxWorkers: 2,
    testTimeout: timeout,
    // Also stops vitest adding its GitHub Actions reporter on CI, which would
    // annotate the PR with this run's expected failure.
    reporters: ['default']
  }
});

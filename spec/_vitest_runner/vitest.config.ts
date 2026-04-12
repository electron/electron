import { defineConfig } from 'vitest/config';

import * as path from 'node:path';

import electronPool from './electron-pool';

const electronShim = path.resolve(__dirname, 'electron-shim.cjs');

export default defineConfig({
  resolve: {
    alias: {
      electron: electronShim,
      'electron/main': electronShim,
      'electron/common': electronShim,
      'electron/renderer': electronShim
    }
  },
  test: {
    include: ['spec/**/*.spec.ts'],
    exclude: ['spec/fixtures/**', 'spec/node_modules/**'],
    setupFiles: ['./spec/_vitest_runner/setup.ts'],
    // Custom pool: each worker is a real Electron main process.
    pool: electronPool as any,
    // Run test *files* in parallel across workers...
    fileParallelism: true,
    isolate: true,
    // ...but keep tests *within* a file sequential, matching current mocha behaviour.
    sequence: { concurrent: false },
    testTimeout: 30_000,
    hookTimeout: 30_000,
    server: {
      deps: {
        external: [/electron-shim\.cjs$/]
      }
    }
  }
});

#!/usr/bin/env node

const { spawnSync } = require('node:child_process');
const path = require('node:path');

const { getAbsoluteElectronExec } = require('../../script/lib/utils');

const ROOT = path.resolve(__dirname, '..', '..');
const vitestBin = path.join(ROOT, 'node_modules', '.bin', 'vitest');

const exe = process.env.ELECTRON_TESTS_EXECUTABLE || getAbsoluteElectronExec();
console.log(`Electron binary: ${exe}`);

const args = ['run', '--config', path.join(__dirname, 'vitest.config.ts'), ...process.argv.slice(2)];

const result = spawnSync(vitestBin, args, {
  cwd: ROOT,
  stdio: 'inherit',
  env: {
    ...process.env,
    ELECTRON_TESTS_EXECUTABLE: exe
  }
});

process.exit(result.status ?? 1);

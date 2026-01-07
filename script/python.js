#!/usr/bin/env node

const childProcess = require('node:child_process');
const { getPythonBinaryName } = require('./lib/utils');

const args = process.argv.slice(2);
const pythonBinary = getPythonBinaryName();

const result = childProcess.spawnSync(pythonBinary, args, {
    stdio: 'inherit',
    shell: process.platform === 'win32'
});

process.exit(result.status ?? 1);

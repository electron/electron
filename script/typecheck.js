#!/usr/bin/env node
// Runs `tsgo --noEmit -p <tsconfig>` and writes a stamp file on success,
// so GN can track typecheck results as a build output.
//
// Usage: node script/typecheck.js --tsconfig <path> --stamp <path>

'use strict';

const cp = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');

function parseArgs (argv) {
  const out = {};
  for (let i = 0; i < argv.length; i++) {
    const a = argv[i];
    if (a.startsWith('--')) {
      const next = argv[i + 1];
      if (next === undefined || next.startsWith('--')) {
        out[a.slice(2)] = true;
      } else {
        out[a.slice(2)] = next;
        i++;
      }
    }
  }
  return out;
}

const args = parseArgs(process.argv.slice(2));
if (!args.tsconfig || !args.stamp) {
  console.error('Usage: typecheck.js --tsconfig <path> --stamp <path>');
  process.exit(1);
}

const electronRoot = path.resolve(__dirname, '..');
// Resolve tsgo's bin entry directly from the package's `bin` map and run it
// via the current Node executable. We can't `require.resolve` the bin path
// (the package's `exports` field doesn't expose it) and we can't spawn
// `node_modules/.bin/tsgo` directly on Windows (it's a `.cmd` shim).
const tsgoPkgPath = require.resolve('@typescript/native-preview/package.json', {
  paths: [electronRoot]
});
const tsgoPkg = JSON.parse(fs.readFileSync(tsgoPkgPath, 'utf8'));
const tsgoEntry = path.resolve(path.dirname(tsgoPkgPath), tsgoPkg.bin.tsgo);

const child = cp.spawnSync(
  process.execPath,
  [tsgoEntry, '--noEmit', '-p', path.resolve(args.tsconfig)],
  { cwd: electronRoot, stdio: 'inherit' }
);

if (child.status !== 0) {
  process.exit(child.status || 1);
}

fs.mkdirSync(path.dirname(args.stamp), { recursive: true });
fs.writeFileSync(args.stamp, '');

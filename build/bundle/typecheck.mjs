// Type checks a TypeScript project without emitting, for the code that
// bundle.mjs builds (rolldown only strips types, it does not check them).
//
//   node build/bundle/typecheck.mjs --project tsconfig.electron.json [--stamp file]

import { spawnSync } from 'node:child_process';
import * as fs from 'node:fs';
import * as path from 'node:path';
import { parseArgs } from 'node:util';

const { values: args } = parseArgs({
  options: {
    project: { type: 'string' },
    stamp: { type: 'string' }
  },
  strict: true
});

if (!args.project) {
  throw new Error('--project is required');
}

const tsc = path.resolve(import.meta.dirname, '..', '..', 'node_modules', 'typescript', 'bin', 'tsc');
const { status, error } = spawnSync(
  process.execPath,
  [tsc, '--project', path.resolve(args.project), '--noEmit', '--pretty'],
  { stdio: 'inherit' }
);
if (error) throw error;
if (status !== 0) process.exit(status ?? 1);

// An existing stamp is left untouched: its dependents only need to rebuild when
// their own sources change, and ninja (GN actions are restat) can then tell.
if (args.stamp && !fs.existsSync(args.stamp)) {
  fs.mkdirSync(path.dirname(args.stamp), { recursive: true });
  fs.writeFileSync(args.stamp, '');
}

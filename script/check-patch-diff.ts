import { spawnSync } from 'node:child_process';
import * as path from 'node:path';

const srcPath = path.resolve(__dirname, '..', '..', '..');
const patchExportFnPath = path.resolve(__dirname, 'export_all_patches.py');
const configPath = path.resolve(__dirname, '..', 'patches', 'config.json');

// Re-export all the patches to check if there were changes.
const proc = spawnSync('python3', [patchExportFnPath, configPath, '--dry-run'], {
  cwd: srcPath
});

if (proc.error) {
  console.error(proc.error.message);
  process.exit(1);
}

if (proc.signal) {
  console.error(`Patch export terminated by signal ${proc.signal}`);
  process.exit(1);
}

// Fail if patch exporting failed, e.g. dry run found changes.
if (proc.status !== 0) {
  console.log(proc.stdout.toString('utf8'));
  console.error(proc.stderr.toString('utf8'));
  process.exit(proc.status ?? 1);
}

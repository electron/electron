const fs = require('node:fs');
const path = require('node:path');
const { styleText } = require('node:util');

// build/siso_revision records the siso commit that the pinned Chromium
// (chromium_version in DEPS) expects, i.e. the `siso_version` git_revision in
// Chromium's own DEPS. CI builds a patched siso from exactly that commit
// (.github/workflows/pipeline-segment-build-siso.yml) before any Chromium
// checkout exists, so the value has to live in this repo rather than be
// fetched at build time. This script derives it from the synced Chromium tree:
// the `gen_siso_revision` gclient hook and lint-staged run it to keep the file
// current, and CI runs it with --check to fail a roll that forgot to commit it.

const check = process.argv.includes('--check');

const OUT_FILE = path.join('build', 'siso_revision');
const outPath = path.resolve(__dirname, '..', OUT_FILE);
const chromiumDepsPath = path.resolve(__dirname, '..', '..', 'DEPS');

if (!fs.existsSync(chromiumDepsPath)) {
  // No Chromium checkout next to us (e.g. a standalone clone); nothing to derive from.
  console.log(`${chromiumDepsPath} not found; leaving ${OUT_FILE} as is`);
  process.exit(0);
}

const chromiumDeps = fs.readFileSync(chromiumDepsPath, 'utf8');
const match = /'siso_version':\s*'git_revision:([0-9a-f]{40})'/.exec(chromiumDeps);
if (!match) {
  console.error(styleText('bold', `Could not find siso_version in ${chromiumDepsPath}`));
  process.exit(1);
}
const expected = `${match[1]}\n`;
const current = fs.existsSync(outPath) ? fs.readFileSync(outPath, 'utf8') : '';

if (current === expected) {
  process.exit(0);
}

if (check) {
  console.error(styleText('bold', `${OUT_FILE} is out of date:`));
  console.error(`  recorded: ${current.trim() || '(missing)'}`);
  console.error(`  Chromium: ${expected.trim()}`);
  console.error(styleText('bold', `\nRun node script/gen-siso-revision.js (or e sync) and commit ${OUT_FILE}`));
  process.exit(1);
}

fs.writeFileSync(outPath, expected);
console.log(`Updated ${OUT_FILE} to ${expected.trim()}; commit this alongside the Chromium roll.`);

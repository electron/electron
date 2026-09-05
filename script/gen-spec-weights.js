// Regenerates script/spec-weights.json (per-platform seconds per spec file,
// used by script/split-tests.js) from the spec-timings.json files that CI test
// jobs upload in their test_artifacts_* bundles.
//
// Usage, from a recent green build.yml run on main:
//   gh run download <run-id> --repo electron/electron -D /tmp/spec-timings \
//     -p 'test_artifacts_darwin_x64_*' -p 'test_artifacts_linux_x64_x11_*' -p 'test_artifacts_win_x64_*'
//   node script/gen-spec-weights.js /tmp/spec-timings
//
// Where several artifacts cover the same platform (arches, mas/darwin, shards)
// the largest time seen for a file wins.

const fs = require('node:fs');
const path = require('node:path');

const roots = process.argv.slice(2);
if (!roots.length) {
  console.error('Usage: node script/gen-spec-weights.js <dir-with-test-artifacts> [...]');
  process.exit(1);
}

const findTimings = (dir, found = []) => {
  for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
    const p = path.join(dir, entry.name);
    if (entry.isDirectory()) findTimings(p, found);
    else if (entry.name === 'spec-timings.json') found.push(p);
  }
  return found;
};

const weights = {};
let inputs = 0;
for (const root of roots) {
  for (const file of findTimings(root)) {
    const { platform, files } = JSON.parse(fs.readFileSync(file, 'utf8'));
    inputs++;
    weights[platform] ??= {};
    for (const [spec, seconds] of Object.entries(files)) {
      weights[platform][spec] = Math.max(weights[platform][spec] ?? 0, Math.round(seconds));
    }
  }
}

if (!inputs) {
  console.error('No spec-timings.json files found under', roots.join(', '));
  process.exit(1);
}

const sorted = {};
for (const platform of Object.keys(weights).sort()) {
  sorted[platform] = Object.fromEntries(Object.entries(weights[platform]).sort(([a], [b]) => a.localeCompare(b)));
}

const outPath = path.resolve(__dirname, 'spec-weights.json');
fs.writeFileSync(outPath, JSON.stringify(sorted, null, 2) + '\n');
console.log(
  `Wrote ${outPath} from ${inputs} timing files:`,
  Object.entries(sorted)
    .map(([p, f]) => `${p} ${Object.keys(f).length} specs`)
    .join(', ')
);

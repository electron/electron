// Regenerates script/spec-weights.json (seconds per spec file for each CI
// test job, used by script/split-tests.js) from the spec-timings.json files
// the test jobs upload in their test_artifacts_* bundles.
//
// Usage, from a recent green build.yml run on the branch:
//   gh run download <run-id> --repo electron/electron -D /tmp/spec-timings -p 'test_artifacts_*'
//   node script/gen-spec-weights.js /tmp/spec-timings
//
// Each job gets its own table, keyed as its artifact is named:
// `<build type>_<arch>[_<sanitizer>]`. Where several shards of one job cover
// a file (a retry) the largest time wins. The Wayland job runs an allowlist
// and is left out.

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

const BUILD_TYPES = { darwin: 'darwin', linux: 'linux', win32: 'win' };

// `darwin_x64`, `mas_arm64`, `linux_x64_asan`, ... - what split-tests.js reads
// from ARTIFACT_KEY in CI. Older timing files carry no sanitizer field; for
// those the artifact directory name says which job wrote them.
const jobKeyOf = (timings, file) => {
  const buildType = timings.mas ? 'mas' : (BUILD_TYPES[timings.platform] ?? timings.platform);
  const sanitizer = timings.sanitizer ?? (/_(asan|ubsan)_/.exec(file)?.[1] || null);
  return `${buildType}_${timings.arch}${sanitizer ? `_${sanitizer}` : ''}`;
};

const weights = {};
let inputs = 0;
for (const root of roots) {
  for (const file of findTimings(root)) {
    if (/_wayland_/.test(file)) continue;
    const timings = JSON.parse(fs.readFileSync(file, 'utf8'));
    const key = jobKeyOf(timings, file);
    inputs++;
    weights[key] ??= {};
    for (const [spec, seconds] of Object.entries(timings.files)) {
      weights[key][spec] = Math.max(weights[key][spec] ?? 0, Math.round(seconds));
    }
  }
}

if (!inputs) {
  console.error('No spec-timings.json files found under', roots.join(', '));
  process.exit(1);
}

const sorted = {};
for (const key of Object.keys(weights).sort()) {
  sorted[key] = Object.fromEntries(Object.entries(weights[key]).sort(([a], [b]) => a.localeCompare(b)));
}

const outPath = path.resolve(__dirname, 'spec-weights.json');
fs.writeFileSync(outPath, JSON.stringify(sorted, null, 2) + '\n');
console.log(
  `Wrote ${outPath} from ${inputs} timing files:`,
  Object.entries(sorted)
    .map(([key, f]) => `${key} ${Object.keys(f).length} specs`)
    .join(', ')
);

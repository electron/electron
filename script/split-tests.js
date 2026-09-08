// Usage: node script/split-tests <shard> <shard-count>
//
// Prints the spec files that belong to <shard> (1-based). Files are packed
// into shards by expected duration, largest first into the currently lightest
// shard, using script/spec-weights.json (seconds per file, regenerate with
// script/gen-spec-weights.js). Files without a weight get the median of the
// known ones; with no weights at all we fall back to `it(` count.
//
// The weights are kept per CI test job, keyed the way CI names the job's
// test_artifacts_* upload: `<build type>_<arch>[_<sanitizer>]`, e.g.
// `darwin_x64`, `mas_arm64`, `linux_x64_asan`, `win_x64`. Jobs differ by
// more than a constant factor - MAS builds skip the updater specs, ASan
// slows native-heavy specs 7x and others not at all, arm64 is faster in
// some files than others - so one table per platform packs the other jobs'
// shards unevenly. CI passes the job's key as ARTIFACT_KEY; elsewhere the
// host's platform and arch are used, and a missing table falls back to the
// nearest one.

const glob = require('glob');

const fs = require('node:fs');
const path = require('node:path');

const currentShard = parseInt(process.argv[2], 10);
const shardCount = parseInt(process.argv[3], 10);

const specFiles = glob.sync('spec/*-spec.ts').map((f) => path.normalize(f));

const BUILD_TYPES = { darwin: 'darwin', linux: 'linux', win32: 'win' };

const jobKey = () =>
  process.env.ARTIFACT_KEY || `${BUILD_TYPES[process.platform] ?? process.platform}_${process.arch}`;

// The table for this job, else the nearest one of the same build type (MAS
// falls back to darwin): same arch first, then the plainest, else a legacy
// per-platform table, else anything.
const pickTable = (all, key) => {
  if (all[key]) return all[key];
  const [buildType, arch] = key.split('_');
  const family = buildType === 'mas' ? ['mas', 'darwin'] : [buildType];
  for (const type of family) {
    const nearest = Object.keys(all)
      .filter((k) => k.startsWith(`${type}_`))
      .sort(
        (a, b) =>
          (a.split('_')[1] === arch ? 0 : 1) - (b.split('_')[1] === arch ? 0 : 1) ||
          a.split('_').length - b.split('_').length ||
          a.localeCompare(b)
      )[0];
    if (nearest) return all[nearest];
  }
  return all[process.platform] ?? all.darwin_x64 ?? all.darwin ?? Object.values(all)[0] ?? {};
};

const loadWeights = () => {
  const weightsPath = path.resolve(__dirname, 'spec-weights.json');
  if (!fs.existsSync(weightsPath)) return {};
  const all = JSON.parse(fs.readFileSync(weightsPath, 'utf8'));
  const weights = {};
  for (const [file, seconds] of Object.entries(pickTable(all, jobKey()))) {
    weights[path.normalize(file)] = seconds;
  }
  return weights;
};

const weights = loadWeights();
const known = specFiles.filter((f) => weights[f] !== undefined).map((f) => weights[f]);
const median = known.length ? known.sort((a, b) => a - b)[Math.floor(known.length / 2)] : 0;

const weightOf = (file) => {
  if (weights[file] !== undefined) return weights[file];
  if (known.length) return median;
  return fs.readFileSync(file, 'utf8').split('it(').length;
};

const buckets = Array.from({ length: shardCount }, () => ({ total: 0, files: [] }));

const ordered = specFiles
  .map((file) => ({ file, weight: weightOf(file) }))
  .sort((a, b) => b.weight - a.weight || a.file.localeCompare(b.file));

for (const { file, weight } of ordered) {
  let target = buckets[0];
  for (const bucket of buckets) {
    if (bucket.total < target.total) target = bucket;
  }
  target.files.push(file);
  target.total += weight;
}

console.log(buckets[currentShard - 1].files.join(' '));

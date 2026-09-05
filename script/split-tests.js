// Usage: node script/split-tests <shard> <shard-count>
//
// Prints the spec files that belong to <shard> (1-based). Files are packed
// into shards by expected duration, largest first into the currently lightest
// shard, using script/spec-weights.json (seconds per file for this platform,
// regenerate with script/gen-spec-weights.js). Files without a weight get the
// median of the known ones; with no weights at all we fall back to `it(` count.

const glob = require('glob');

const fs = require('node:fs');
const path = require('node:path');

const currentShard = parseInt(process.argv[2], 10);
const shardCount = parseInt(process.argv[3], 10);

const specFiles = glob.sync('spec/*-spec.ts').map((f) => path.normalize(f));

const loadWeights = () => {
  const weightsPath = path.resolve(__dirname, 'spec-weights.json');
  if (!fs.existsSync(weightsPath)) return {};
  const all = JSON.parse(fs.readFileSync(weightsPath, 'utf8'));
  const forPlatform = all[process.platform] ?? all.darwin ?? Object.values(all)[0] ?? {};
  const weights = {};
  for (const [file, seconds] of Object.entries(forPlatform)) {
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

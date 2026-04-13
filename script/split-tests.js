const glob = require('glob');

const fs = require('node:fs');
const path = require('node:path');

const currentShard = parseInt(process.argv[2], 10);
const shardCount = parseInt(process.argv[3], 10);

const buckets = [];
for (let i = 0; i < shardCount; i++) {
  buckets.push([]);
}

function testCountIn(file) {
  return fs.readFileSync(file, 'utf8').split('it(').length;
}

function distribute(files) {
  files.sort((a, b) => testCountIn(b) - testCountIn(a));
  let shard = 0;
  for (const file of files) {
    buckets[shard].push(path.normalize(file));
    shard++;
    if (shard === shardCount) shard = 0;
  }
}

// Parallel and serial sets are distributed independently so each shard gets a
// proportional slice of both; run.js routes spec/serial/* to the
// --no-file-parallelism phase.
distribute(glob.sync('spec/*.spec.ts'));
distribute(glob.sync('spec/serial/*.spec.ts'));

console.log(buckets[currentShard - 1].join(' '));

const glob = require('glob');

const path = require('node:path');

const currentShard = parseInt(process.argv[2], 10);
const shardCount = parseInt(process.argv[3], 10);

const specFiles = glob.sync('spec/*-spec.ts');

const buckets = [];

for (let i = 0; i < shardCount; i++) {
  buckets.push([]);
}

specFiles.sort();

let shard = 0;
for (const specFile of specFiles) {
  buckets[shard].push(path.normalize(specFile));
  shard++;
  if (shard === shardCount) shard = 0;
}

console.log(buckets[currentShard - 1].join(' '));

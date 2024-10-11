const glob = require('glob');

const fs = require('node:fs');
const path = require('node:path');

const VISIBILITY_SPEC = path.join('spec', 'visibility-state-spec.ts');

const currentShard = parseInt(process.argv[2], 10);
const shardCount = parseInt(process.argv[3], 10);

const specFiles = glob.sync('spec/*-spec.ts');

const buckets = [];

for (let i = 0; i < shardCount; i++) {
  buckets.push([]);
}

const testsInSpecFile = Object.create(null);
for (const specFile of specFiles) {
  const testContent = fs.readFileSync(specFile, 'utf8');
  testsInSpecFile[specFile] = testContent.split('it(').length;
}

specFiles.sort((a, b) => {
  return testsInSpecFile[b] - testsInSpecFile[a];
});

let shard = 0;
for (const specFile of specFiles) {
  buckets[shard].push(path.normalize(specFile));
  shard++;
  if (shard === shardCount) shard = 0;
}

const visiblitySpecIdx = buckets[currentShard - 1].indexOf(VISIBILITY_SPEC);
if (visiblitySpecIdx > -1) {
  // If visibility-state-spec is in the list, move it to the first position
  // so that it gets executed first to avoid other specs interferring with it.
  buckets[currentShard - 1].splice(visiblitySpecIdx, 1);
  buckets[currentShard - 1].unshift(VISIBILITY_SPEC);
}
console.log(buckets[currentShard - 1].join(' '));

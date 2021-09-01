const fs = require('fs');

const plistPath = process.argv[2];
const outputPath = process.argv[3];
const keySet = process.argv.slice(4);
const keyPairs = {};
for (let i = 0; i * 2 < keySet.length; i++) {
  keyPairs[keySet[i]] = fs.readFileSync(keySet[(keySet.length / 2) + i], 'utf8');
}

let plistContents = fs.readFileSync(plistPath, 'utf8');

for (const key of Object.keys(keyPairs)) {
  plistContents = plistContents.replace(`$\{${key}}`, keyPairs[key]);
}

fs.writeFileSync(outputPath, plistContents);

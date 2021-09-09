const fs = require('fs');

const [,, plistPath, outputPath, ...keySet] = process.argv;

const keyPairs = {};
for (let i = 0; i * 2 < keySet.length; i++) {
  keyPairs[keySet[i]] = fs.readFileSync(keySet[(keySet.length / 2) + i], 'utf8');
}

let plistContents = fs.readFileSync(plistPath, 'utf8');

for (const key of Object.keys(keyPairs)) {
  plistContents = plistContents.replace(`$\{${key}}`, keyPairs[key]);
}

fs.writeFileSync(outputPath, plistContents);

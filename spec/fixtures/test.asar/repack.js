// Use this script to regenerate these fixture files
// using a new version of the asar package

const asar = require('asar');
const fs = require('fs');
const os = require('os');
const path = require('path');

const archives = [];
for (const child of fs.readdirSync(__dirname)) {
  if (child.endsWith('.asar')) {
    archives.push(path.resolve(__dirname, child));
  }
}

for (const archive of archives) {
  const tmp = fs.mkdtempSync(path.resolve(os.tmpdir(), 'asar-spec-'));
  asar.extractAll(archive, tmp);
  asar.createPackageWithOptions(tmp, archive, {
    unpack: fs.existsSync(archive + '.unpacked') ? '*' : undefined
  });
}

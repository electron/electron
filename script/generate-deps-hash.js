const crypto = require('node:crypto');
const fs = require('node:fs');
const path = require('node:path');

const HASH_VERSIONS = {
  darwin: 4,
  win32: 4,
  linux: 4
};

const filesToHash = [
  path.resolve(__dirname, '../DEPS'),
  path.resolve(__dirname, '../yarn.lock'),
  path.resolve(__dirname, '../script/sysroots.json'),
  path.resolve(__dirname, '../.github/actions/checkout/action.yml')
];

const addAllFiles = (dir) => {
  for (const child of fs.readdirSync(dir).sort()) {
    const childPath = path.resolve(dir, child);
    if (fs.statSync(childPath).isDirectory()) {
      addAllFiles(childPath);
    } else {
      filesToHash.push(childPath);
    }
  }
};

addAllFiles(path.resolve(__dirname, '../patches'));

const hasher = crypto.createHash('SHA256');
const addToHashAndLog = (s) => {
  return hasher.update(s);
};
addToHashAndLog(`HASH_VERSION:${HASH_VERSIONS[process.platform]}`);
for (const file of filesToHash) {
  hasher.update(fs.readFileSync(file));
}

const extraArgs = process.env.GCLIENT_EXTRA_ARGS || 'no_extra_args';
addToHashAndLog(extraArgs);

fs.writeFileSync(path.resolve(__dirname, '../.depshash'), hasher.digest('hex'));

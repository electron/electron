const crypto = require('node:crypto');
const fs = require('node:fs');
const path = require('node:path');

// Per platform hash versions to bust the cache on different platforms
const HASH_VERSIONS = {
  darwin: 4,
  win32: 4,
  linux: 4
};

// Base files to hash
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

// Add all patch files to the hash
addAllFiles(path.resolve(__dirname, '../patches'));

// Create Hash
const hasher = crypto.createHash('SHA256');
const addToHashAndLog = (s) => {
  return hasher.update(s);
};
addToHashAndLog(`HASH_VERSION:${HASH_VERSIONS[process.platform]}`);
for (const file of filesToHash) {
  hasher.update(fs.readFileSync(file));
}

// Add the GCLIENT_EXTRA_ARGS variable to the hash
const extraArgs = process.env.GCLIENT_EXTRA_ARGS || 'no_extra_args';
addToHashAndLog(extraArgs);

// Write the hash to disk
fs.writeFileSync(path.resolve(__dirname, '../.depshash'), hasher.digest('hex'));

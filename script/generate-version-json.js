const fs = require('fs');
const path = require('path');
const semver = require('semver');

const outputPath = process.argv[2];

const currentVersion = fs.readFileSync(path.resolve(__dirname, '../ELECTRON_VERSION'), 'utf8').trim();

const parsed = semver.parse(currentVersion);

let prerelease = '';
if (parsed.prerelease && parsed.prerelease.length > 0) {
  prerelease = parsed.prerelease.join('.');
}

const {
  major,
  minor,
  patch
} = parsed;

fs.writeFileSync(outputPath, JSON.stringify({
  major,
  minor,
  patch,
  prerelease,
  has_prerelease: prerelease === '' ? 0 : 1
}, null, 2));

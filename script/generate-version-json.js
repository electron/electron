const semver = require('semver');

const fs = require('node:fs');

const outputPath = process.argv[2];
const currentVersion = process.argv[3];

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
  full_version: currentVersion,
  major,
  minor,
  patch,
  prerelease,
  prerelease_number: prerelease ? parsed.prerelease[parsed.prerelease.length - 1] : '0',
  has_prerelease: prerelease === '' ? 0 : 1
}, null, 2));

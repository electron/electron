#!/usr/bin/env node

const semver = require('semver');
const minimist = require('minimist');

const { getElectronVersion } = require('../lib/get-version');
const versionUtils = require('./version-utils');

function parseCommandLine () {
  let help;
  const opts = minimist(process.argv.slice(2), {
    string: ['bump', 'version'],
    boolean: ['dryRun', 'help'],
    alias: { version: ['v'] },
    unknown: () => { help = true; }
  });
  if (help || opts.help || !opts.bump) {
    console.log(`
      Bump release version number. Possible arguments:\n
        --bump=patch to increment patch version\n
        --version={version} to set version number directly\n
        --dryRun to print the next version without updating files
      Note that you can use both --bump and --stable  simultaneously.
    `);
    process.exit(0);
  }
  return opts;
}

// run the script
async function main () {
  const opts = parseCommandLine();
  const currentVersion = getElectronVersion();
  const version = await nextVersion(opts.bump, currentVersion);

  // print would-be new version and exit early
  if (opts.dryRun) {
    console.log(`new version number would be: ${version}\n`);
    return 0;
  }

  console.log(`Bumped to version: ${version}`);
}

// get next version for release based on [nightly, alpha, beta, stable]
async function nextVersion (bumpType, version) {
  if (
    versionUtils.isNightly(version) ||
    versionUtils.isAlpha(version) ||
    versionUtils.isBeta(version)
  ) {
    switch (bumpType) {
      case 'nightly':
        version = await versionUtils.nextNightly(version);
        break;
      case 'alpha':
        version = await versionUtils.nextAlpha(version);
        break;
      case 'beta':
        version = await versionUtils.nextBeta(version);
        break;
      case 'stable':
        version = semver.valid(semver.coerce(version));
        break;
      default:
        throw new Error('Invalid bump type.');
    }
  } else if (versionUtils.isStable(version)) {
    switch (bumpType) {
      case 'nightly':
        version = versionUtils.nextNightly(version);
        break;
      case 'alpha':
        throw new Error('Cannot bump to alpha from stable.');
      case 'beta':
        throw new Error('Cannot bump to beta from stable.');
      case 'minor':
        version = semver.inc(version, 'minor');
        break;
      case 'stable':
        version = semver.inc(version, 'patch');
        break;
      default:
        throw new Error('Invalid bump type.');
    }
  } else {
    throw new Error(`Invalid current version: ${version}`);
  }
  return version;
}

if (require.main === module) {
  main().catch((error) => {
    console.error(error);
    process.exit(1);
  });
}

module.exports = { nextVersion };

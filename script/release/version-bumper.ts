#!/usr/bin/env node

import { valid, coerce, inc } from 'semver';

import { parseArgs } from 'node:util';

import { VersionBumpType } from './types';
import {
  isNightly,
  isAlpha,
  isBeta,
  nextNightly,
  nextAlpha,
  nextBeta,
  isStable
} from './version-utils';
import { getElectronVersion } from '../lib/get-version';

// run the script
async function main () {
  const { values: { bump, dryRun, help } } = parseArgs({
    options: {
      bump: {
        type: 'string'
      },
      dryRun: {
        type: 'boolean'
      },
      help: {
        type: 'boolean'
      }
    }
  });

  if (!bump || help) {
    console.log(`
        Bump release version number. Possible arguments:\n
          --bump=patch to increment patch version\n
          --version={version} to set version number directly\n
          --dryRun to print the next version without updating files
        Note that you can use both --bump and --stable  simultaneously.
      `);
    if (!bump) process.exit(0);
    else process.exit(1);
  }

  const currentVersion = getElectronVersion();
  const version = await nextVersion(bump as VersionBumpType, currentVersion);

  // print would-be new version and exit early
  if (dryRun) {
    console.log(`new version number would be: ${version}\n`);
    return 0;
  }

  console.log(`Bumped to version: ${version}`);
}

// get next version for release based on [nightly, alpha, beta, stable]
export async function nextVersion (bumpType: VersionBumpType, version: string) {
  if (isNightly(version) || isAlpha(version) || isBeta(version)) {
    switch (bumpType) {
      case 'nightly':
        version = await nextNightly(version);
        break;
      case 'alpha':
        version = await nextAlpha(version);
        break;
      case 'beta':
        version = await nextBeta(version);
        break;
      case 'stable':
        version = valid(coerce(version))!;
        break;
      default:
        throw new Error('Invalid bump type.');
    }
  } else if (isStable(version)) {
    switch (bumpType) {
      case 'nightly':
        version = await nextNightly(version);
        break;
      case 'alpha':
        throw new Error('Cannot bump to alpha from stable.');
      case 'beta':
        throw new Error('Cannot bump to beta from stable.');
      case 'minor':
        version = inc(version, 'minor')!;
        break;
      case 'stable':
        version = inc(version, 'patch')!;
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

#!/usr/bin/env node

import { Octokit } from '@octokit/rest';
import { GitProcess } from 'dugite';
import { execSync } from 'node:child_process';
import { join } from 'node:path';
import { createInterface } from 'node:readline';
import { parseArgs } from 'node:util';

import ciReleaseBuild from './ci-release-build';
import releaseNotesGenerator from './notes';
import { getCurrentBranch, ELECTRON_DIR } from '../lib/utils.js';
import { createGitHubTokenStrategy } from './github-token';
import { ELECTRON_REPO, ElectronReleaseRepo, NIGHTLY_REPO } from './types';

const { values: { notesOnly, dryRun: dryRunArg, stable: isStableArg, branch: branchArg, automaticRelease }, positionals } = parseArgs({
  options: {
    notesOnly: {
      type: 'boolean'
    },
    dryRun: {
      type: 'boolean'
    },
    stable: {
      type: 'boolean'
    },
    branch: {
      type: 'string'
    },
    automaticRelease: {
      type: 'boolean'
    }
  },
  allowPositionals: true
});

const bumpType = positionals[0];
const targetRepo = getRepo();

function getRepo (): ElectronReleaseRepo {
  return bumpType === 'nightly' ? NIGHTLY_REPO : ELECTRON_REPO;
}

const octokit = new Octokit({
  authStrategy: createGitHubTokenStrategy(getRepo())
});

require('colors');
const pass = '✓'.green;
const fail = '✗'.red;

if (!bumpType && !notesOnly) {
  console.log('Usage: prepare-release [stable | minor | beta | alpha | nightly]' +
     ' (--stable) (--notesOnly) (--automaticRelease) (--branch)');
  process.exit(1);
}

enum DryRunMode {
  DRY_RUN,
  REAL_RUN,
}

async function getNewVersion (dryRunMode: DryRunMode) {
  if (dryRunMode === DryRunMode.REAL_RUN) {
    console.log(`Bumping for new "${bumpType}" version.`);
  }
  const bumpScript = join(__dirname, 'version-bumper.js');
  const scriptArgs = ['node', bumpScript, `--bump=${bumpType}`];
  if (dryRunMode === DryRunMode.DRY_RUN) scriptArgs.push('--dryRun');
  try {
    let bumpVersion = execSync(scriptArgs.join(' '), { encoding: 'utf-8' });
    bumpVersion = bumpVersion.substr(bumpVersion.indexOf(':') + 1).trim();
    const newVersion = `v${bumpVersion}`;
    if (dryRunMode === DryRunMode.REAL_RUN) {
      console.log(`${pass} Successfully bumped version to ${newVersion}`);
    }
    return newVersion;
  } catch (err) {
    console.log(`${fail} Could not bump version, error was:`, err);
    throw err;
  }
}

async function getReleaseNotes (currentBranch: string, newVersion: string) {
  if (bumpType === 'nightly') {
    return { text: 'Nightlies do not get release notes, please compare tags for info.' };
  }
  console.log(`Generating release notes for ${currentBranch}.`);
  const releaseNotes = await releaseNotesGenerator(currentBranch, newVersion);
  if (releaseNotes.warning) {
    console.warn(releaseNotes.warning);
  }
  return releaseNotes;
}

async function createRelease (branchToTarget: string, isPreRelease: boolean) {
  const newVersion = await getNewVersion(DryRunMode.REAL_RUN);
  const releaseNotes = await getReleaseNotes(branchToTarget, newVersion);
  await tagRelease(newVersion);

  console.log('Checking for existing draft release.');
  const releases = await octokit.repos.listReleases({
    owner: 'electron',
    repo: targetRepo
  }).catch(err => {
    console.log(`${fail} Could not get releases. Error was: `, err);
    throw err;
  });

  const drafts = releases.data.filter(release => release.draft &&
    release.tag_name === newVersion);
  if (drafts.length > 0) {
    console.log(`${fail} Aborting because draft release for
      ${drafts[0].tag_name} already exists.`);
    process.exit(1);
  }
  console.log(`${pass} A draft release does not exist; creating one.`);

  let releaseBody;
  let releaseIsPrelease = false;
  if (isPreRelease) {
    if (newVersion.indexOf('nightly') > 0) {
      releaseBody = 'Note: This is a nightly release.  Please file new issues ' +
        'for any bugs you find in it.\n \n This release is published to npm ' +
        'under the electron-nightly package and can be installed via `npm install electron-nightly`, ' +
        `or \`npm install electron-nightly@${newVersion.substr(1)}\`.\n \n ${releaseNotes.text}`;
    } else if (newVersion.indexOf('alpha') > 0) {
      releaseBody = 'Note: This is an alpha release.  Please file new issues ' +
        'for any bugs you find in it.\n \n This release is published to npm ' +
        'under the alpha tag and can be installed via `npm install electron@alpha`, ' +
        `or \`npm install electron@${newVersion.substr(1)}\`.\n \n ${releaseNotes.text}`;
    } else {
      releaseBody = 'Note: This is a beta release.  Please file new issues ' +
        'for any bugs you find in it.\n \n This release is published to npm ' +
        'under the beta tag and can be installed via `npm install electron@beta`, ' +
        `or \`npm install electron@${newVersion.substr(1)}\`.\n \n ${releaseNotes.text}`;
    }
    releaseIsPrelease = true;
  } else {
    releaseBody = releaseNotes.text;
  }

  const release = await octokit.repos.createRelease({
    owner: 'electron',
    repo: targetRepo,
    tag_name: newVersion,
    draft: true,
    name: `electron ${newVersion}`,
    body: releaseBody,
    prerelease: releaseIsPrelease,
    target_commitish: newVersion.includes('nightly') ? 'main' : branchToTarget
  }).catch(err => {
    console.log(`${fail} Error creating new release: `, err);
    process.exit(1);
  });

  console.log(`Release has been created with id: ${release.data.id}.`);
  console.log(`${pass} Draft release for ${newVersion} successful.`);
}

async function pushRelease (branch: string) {
  const pushDetails = await GitProcess.exec(['push', 'origin', `HEAD:${branch}`, '--follow-tags'], ELECTRON_DIR);
  if (pushDetails.exitCode === 0) {
    console.log(`${pass} Successfully pushed the release.  Wait for ` +
      'release builds to finish before running "npm run release".');
  } else {
    console.log(`${fail} Error pushing the release: ${pushDetails.stderr}`);
    process.exit(1);
  }
}

async function runReleaseBuilds (branch: string, newVersion: string) {
  await ciReleaseBuild(branch, {
    ci: undefined,
    ghRelease: true,
    newVersion
  });
}

async function tagRelease (version: string) {
  console.log(`Tagging release ${version}.`);
  const checkoutDetails = await GitProcess.exec(['tag', '-a', '-m', version, version], ELECTRON_DIR);
  if (checkoutDetails.exitCode === 0) {
    console.log(`${pass} Successfully tagged ${version}.`);
  } else {
    console.log(`${fail} Error tagging ${version}: ` +
      `${checkoutDetails.stderr}`);
    process.exit(1);
  }
}

async function verifyNewVersion () {
  const newVersion = await getNewVersion(DryRunMode.DRY_RUN);
  let response;
  if (automaticRelease) {
    response = 'y';
  } else {
    response = await promptForVersion(newVersion);
  }
  if (response.match(/^y/i)) {
    console.log(`${pass} Starting release of ${newVersion}`);
  } else {
    console.log(`${fail} Aborting release of ${newVersion}`);
    process.exit();
  }

  return newVersion;
}

async function promptForVersion (version: string) {
  return new Promise<string>(resolve => {
    const rl = createInterface({
      input: process.stdin,
      output: process.stdout
    });
    rl.question(`Do you want to create the release ${version.green} (y/N)? `, (answer) => {
      rl.close();
      resolve(answer);
    });
  });
}

// function to determine if there have been commits to main since the last release
async function changesToRelease () {
  const lastCommitWasRelease = /^Bump v[0-9]+.[0-9]+.[0-9]+(-beta.[0-9]+)?(-alpha.[0-9]+)?(-nightly.[0-9]+)?$/g;
  const lastCommit = await GitProcess.exec(['log', '-n', '1', '--pretty=format:\'%s\''], ELECTRON_DIR);
  return !lastCommitWasRelease.test(lastCommit.stdout);
}

async function prepareRelease (isPreRelease: boolean, dryRunMode: DryRunMode) {
  if (dryRunMode === DryRunMode.DRY_RUN) {
    const newVersion = await getNewVersion(DryRunMode.DRY_RUN);
    console.log(newVersion);
  } else {
    const currentBranch = branchArg || await getCurrentBranch(ELECTRON_DIR);
    if (notesOnly) {
      const newVersion = await getNewVersion(DryRunMode.DRY_RUN);
      const releaseNotes = await getReleaseNotes(currentBranch, newVersion);
      console.log(`Draft release notes are: \n${releaseNotes.text}`);
    } else {
      const changes = await changesToRelease();
      if (changes) {
        const newVersion = await verifyNewVersion();
        await createRelease(currentBranch, isPreRelease);
        await pushRelease(currentBranch);
        await runReleaseBuilds(currentBranch, newVersion);
      } else {
        console.log('There are no new changes to this branch since the last release, aborting release.');
        process.exit(1);
      }
    }
  }
}

prepareRelease(!isStableArg, dryRunArg ? DryRunMode.DRY_RUN : DryRunMode.REAL_RUN)
  .catch((err) => {
    console.error(err);
    process.exit(1);
  });

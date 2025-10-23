import { Octokit } from '@octokit/rest';
import * as chalk from 'chalk';
import { GitProcess } from 'dugite';

import { execSync } from 'node:child_process';
import { join } from 'node:path';

import { createGitHubTokenStrategy } from './github-token';
import releaseNotesGenerator from './notes';
import { runReleaseCIJobs } from './run-release-ci-jobs';
import { ELECTRON_ORG, ElectronReleaseRepo, VersionBumpType } from './types';
import { getCurrentBranch, ELECTRON_DIR } from '../lib/utils.js';

const pass = chalk.green('✓');
const fail = chalk.red('✗');

enum DryRunMode {
  DRY_RUN,
  REAL_RUN,
}

type PrepareReleaseOptions = {
  targetRepo: ElectronReleaseRepo;
  targetBranch?: string;
  bumpType: VersionBumpType;
  isPreRelease: boolean;
};

async function getNewVersion (
  options: PrepareReleaseOptions,
  dryRunMode: DryRunMode
) {
  if (dryRunMode === DryRunMode.REAL_RUN) {
    console.log(`Bumping for new "${options.bumpType}" version.`);
  }
  const bumpScript = join(__dirname, 'version-bumper.ts');
  const scriptArgs = [
    'node',
    'node_modules/.bin/ts-node',
    bumpScript,
    `--bump=${options.bumpType}`
  ];
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

async function getReleaseNotes (
  options: PrepareReleaseOptions,
  currentBranch: string,
  newVersion: string
) {
  if (options.bumpType === 'nightly') {
    return {
      text: 'Nightlies do not get release notes, please compare tags for info.'
    };
  }
  console.log(`Generating release notes for ${currentBranch}.`);
  const releaseNotes = await releaseNotesGenerator(currentBranch, newVersion);
  if (releaseNotes.warning) {
    console.warn(releaseNotes.warning);
  }
  return releaseNotes;
}

async function createRelease (
  options: PrepareReleaseOptions,
  branchToTarget: string
) {
  const newVersion = await getNewVersion(options, DryRunMode.REAL_RUN);
  const releaseNotes = await getReleaseNotes(
    options,
    branchToTarget,
    newVersion
  );
  await tagRelease(newVersion);

  const octokit = new Octokit({
    authStrategy: createGitHubTokenStrategy(options.targetRepo)
  });

  console.log('Checking for existing draft release.');
  const releases = await octokit.repos
    .listReleases({
      owner: ELECTRON_ORG,
      repo: options.targetRepo
    })
    .catch((err) => {
      console.log(`${fail} Could not get releases. Error was: `, err);
      throw err;
    });

  const drafts = releases.data.filter(
    (release) => release.draft && release.tag_name === newVersion
  );
  if (drafts.length > 0) {
    console.log(`${fail} Aborting because draft release for
      ${drafts[0].tag_name} already exists.`);
    process.exit(1);
  }
  console.log(`${pass} A draft release does not exist; creating one.`);

  let releaseBody;
  let releaseIsPrelease = false;
  if (options.isPreRelease) {
    if (newVersion.indexOf('nightly') > 0) {
      releaseBody =
        'Note: This is a nightly release.  Please file new issues ' +
        'for any bugs you find in it.\n \n This release is published to npm ' +
        'under the electron-nightly package and can be installed via `npm install electron-nightly`, ' +
        `or \`npm install electron-nightly@${newVersion.substr(1)}\`.\n \n ${
          releaseNotes.text
        }`;
    } else if (newVersion.indexOf('alpha') > 0) {
      releaseBody =
        'Note: This is an alpha release.  Please file new issues ' +
        'for any bugs you find in it.\n \n This release is published to npm ' +
        'under the alpha tag and can be installed via `npm install electron@alpha`, ' +
        `or \`npm install electron@${newVersion.substr(1)}\`.\n \n ${
          releaseNotes.text
        }`;
    } else {
      releaseBody =
        'Note: This is a beta release.  Please file new issues ' +
        'for any bugs you find in it.\n \n This release is published to npm ' +
        'under the beta tag and can be installed via `npm install electron@beta`, ' +
        `or \`npm install electron@${newVersion.substr(1)}\`.\n \n ${
          releaseNotes.text
        }`;
    }
    releaseIsPrelease = true;
  } else {
    releaseBody = releaseNotes.text;
  }

  const release = await octokit.repos
    .createRelease({
      owner: ELECTRON_ORG,
      repo: options.targetRepo,
      tag_name: newVersion,
      draft: true,
      name: `electron ${newVersion}`,
      body: releaseBody,
      prerelease: releaseIsPrelease,
      target_commitish: newVersion.includes('nightly')
        ? 'main'
        : branchToTarget
    })
    .catch((err) => {
      console.log(`${fail} Error creating new release: `, err);
      process.exit(1);
    });

  console.log(`Release has been created with id: ${release.data.id}.`);
  console.log(`${pass} Draft release for ${newVersion} successful.`);
}

async function pushRelease (branch: string) {
  const pushDetails = await GitProcess.exec(
    ['push', 'origin', `HEAD:${branch}`, '--follow-tags'],
    ELECTRON_DIR
  );
  if (pushDetails.exitCode === 0) {
    console.log(
      `${pass} Successfully pushed the release.  Wait for ` +
        'release builds to finish before running "npm run release".'
    );
  } else {
    console.log(`${fail} Error pushing the release: ${pushDetails.stderr}`);
    process.exit(1);
  }
}

async function runReleaseBuilds (branch: string, newVersion: string) {
  await runReleaseCIJobs(branch, {
    ci: undefined,
    ghRelease: true,
    newVersion
  });
}

async function tagRelease (version: string) {
  console.log(`Tagging release ${version}.`);
  const checkoutDetails = await GitProcess.exec(
    ['tag', '-a', '-m', version, version],
    ELECTRON_DIR
  );
  if (checkoutDetails.exitCode === 0) {
    console.log(`${pass} Successfully tagged ${version}.`);
  } else {
    console.log(
      `${fail} Error tagging ${version}: ` + `${checkoutDetails.stderr}`
    );
    process.exit(1);
  }
}

export async function printNextVersion (options: PrepareReleaseOptions) {
  const newVersion = await getNewVersion(options, DryRunMode.DRY_RUN);
  console.log(newVersion);
}

export async function prepareRelease (options: PrepareReleaseOptions) {
  const currentBranch =
    options.targetBranch || (await getCurrentBranch(ELECTRON_DIR));

  const newVersion = await getNewVersion(options, DryRunMode.DRY_RUN);
  console.log(`${pass} Starting release of ${newVersion}`);
  await createRelease(options, currentBranch);
  await pushRelease(currentBranch);
  await runReleaseBuilds(currentBranch, newVersion);
}

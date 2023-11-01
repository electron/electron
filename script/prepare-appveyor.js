if (!process.env.CI) require('dotenv-safe').load();

const assert = require('node:assert');
const fs = require('node:fs');
const got = require('got');
const path = require('node:path');
const { handleGitCall, ELECTRON_DIR } = require('./lib/utils.js');
const { Octokit } = require('@octokit/rest');
const octokit = new Octokit();

const APPVEYOR_IMAGES_URL = 'https://ci.appveyor.com/api/build-clouds';
const APPVEYOR_JOB_URL = 'https://ci.appveyor.com/api/builds';
const ROLLER_BRANCH_PATTERN = /^roller\/chromium$/;

const DEFAULT_BUILD_CLOUD_ID = '1598';
const DEFAULT_BUILD_CLOUD = 'electronhq-16-core';
const DEFAULT_BAKE_BASE_IMAGE = 'base-bake-image';
const DEFAULT_BUILD_IMAGE = 'base-bake-image';

const appveyorBakeJob = 'electron-bake-image';
const appVeyorJobs = {
  'electron-x64': 'electron-x64-testing',
  'electron-woa': 'electron-woa-testing',
  'electron-ia32': 'electron-ia32-testing'
};

async function makeRequest ({ auth, username, password, url, headers, body, method }) {
  const clonedHeaders = {
    ...(headers || {})
  };
  if (auth?.bearer) {
    clonedHeaders.Authorization = `Bearer ${auth.bearer}`;
  }

  const options = {
    headers: clonedHeaders,
    body,
    method
  };

  if (username || password) {
    options.username = username;
    options.password = password;
  }

  const response = await got(url, options);

  if (response.statusCode < 200 || response.statusCode >= 300) {
    console.error('Error: ', `(status ${response.statusCode})`, response.body);
    throw new Error(`Unexpected status code ${response.statusCode} from ${url}`);
  }
  return JSON.parse(response.body);
}

async function checkAppVeyorImage (options) {
  const IMAGE_URL = `${APPVEYOR_IMAGES_URL}/${options.cloudId}`;
  const requestOpts = {
    url: IMAGE_URL,
    auth: {
      bearer: process.env.APPVEYOR_TOKEN
    },
    headers: {
      'Content-Type': 'application/json'
    },
    method: 'GET'
  };

  try {
    const { settings } = await makeRequest(requestOpts);
    const { cloudSettings } = settings;
    return cloudSettings.images.find(image => image.name === `${options.imageVersion}`) || null;
  } catch (err) {
    console.log('Could not call AppVeyor: ', err);
  }
}

async function getPullRequestId (targetBranch) {
  const prsForBranch = await octokit.pulls.list({
    owner: 'electron',
    repo: 'electron',
    state: 'open',
    head: `electron:${targetBranch}`
  });
  if (prsForBranch.data.length === 1) {
    return prsForBranch.data[0].number;
  } else {
    return null;
  }
}

function useAppVeyorImage (targetBranch, options) {
  const validJobs = Object.keys(appVeyorJobs);
  if (options.job) {
    assert(validJobs.includes(options.job), `Unknown AppVeyor CI job name: ${options.job}.  Valid values are: ${validJobs}.`);
    callAppVeyorBuildJobs(targetBranch, options.job, options);
  } else {
    for (const job of validJobs) {
      callAppVeyorBuildJobs(targetBranch, job, options);
    }
  }
}

async function callAppVeyorBuildJobs (targetBranch, job, options) {
  console.log(`Using AppVeyor image ${options.version} for ${job}`);

  const pullRequestId = await getPullRequestId(targetBranch);
  const environmentVariables = {
    APPVEYOR_BUILD_WORKER_CLOUD: DEFAULT_BUILD_CLOUD,
    APPVEYOR_BUILD_WORKER_IMAGE: options.version,
    ELECTRON_OUT_DIR: 'Default',
    ELECTRON_ENABLE_STACK_DUMPING: 1,
    ELECTRON_ALSO_LOG_TO_STDERR: 1,
    GOMA_FALLBACK_ON_AUTH_FAILURE: true,
    DEPOT_TOOLS_WIN_TOOLCHAIN: 0,
    PYTHONIOENCODING: 'UTF-8'
  };

  const requestOpts = {
    url: APPVEYOR_JOB_URL,
    auth: {
      bearer: process.env.APPVEYOR_TOKEN
    },
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      accountName: 'electron-bot',
      projectSlug: appVeyorJobs[job],
      branch: targetBranch,
      pullRequestId: pullRequestId || undefined,
      commitId: options.commit || undefined,
      environmentVariables
    }),
    method: 'POST'
  };

  try {
    const { version } = await makeRequest(requestOpts);
    const buildUrl = `https://ci.appveyor.com/project/electron-bot/${appVeyorJobs[job]}/build/${version}`;
    console.log(`AppVeyor CI request for ${job} successful.  Check status at ${buildUrl}`);
  } catch (err) {
    console.log('Could not call AppVeyor: ', err);
  }
}

async function bakeAppVeyorImage (targetBranch, options) {
  console.log(`Baking a new AppVeyor image for ${options.version}, on build cloud ${options.cloudId}`);

  const environmentVariables = {
    APPVEYOR_BUILD_WORKER_CLOUD: DEFAULT_BUILD_CLOUD,
    APPVEYOR_BUILD_WORKER_IMAGE: DEFAULT_BAKE_BASE_IMAGE,
    APPVEYOR_BAKE_IMAGE: options.version
  };

  const requestOpts = {
    url: APPVEYOR_JOB_URL,
    auth: {
      bearer: process.env.APPVEYOR_TOKEN
    },
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      accountName: 'electron-bot',
      projectSlug: appveyorBakeJob,
      branch: targetBranch,
      commitId: options.commit || undefined,
      environmentVariables
    }),
    method: 'POST'
  };

  try {
    const { version } = await makeRequest(requestOpts);
    const bakeUrl = `https://ci.appveyor.com/project/electron-bot/${appveyorBakeJob}/build/${version}`;
    console.log(`AppVeyor image bake request for ${options.version} successful.  Check bake status at ${bakeUrl}`);
  } catch (err) {
    console.log('Could not call AppVeyor: ', err);
  }
}

async function prepareAppVeyorImage (opts) {
  const branch = await handleGitCall(['rev-parse', '--abbrev-ref', 'HEAD'], ELECTRON_DIR);
  if (ROLLER_BRANCH_PATTERN.test(branch)) {
    useAppVeyorImage(branch, { ...opts, version: DEFAULT_BUILD_IMAGE, cloudId: DEFAULT_BUILD_CLOUD_ID });
  } else {
    const versionRegex = /chromium_version':\n +'(.+?)',/m;
    const deps = fs.readFileSync(path.resolve(__dirname, '..', 'DEPS'), 'utf8');
    const [, CHROMIUM_VERSION] = versionRegex.exec(deps);

    const cloudId = opts.cloudId || DEFAULT_BUILD_CLOUD_ID;
    const imageVersion = opts.imageVersion || `e-${CHROMIUM_VERSION}`;
    const image = await checkAppVeyorImage({ cloudId, imageVersion });

    if (image && image.name) {
      console.log(`Image exists for ${image.name}. Continuing AppVeyor jobs using ${cloudId}.\n`);
    } else {
      console.log(`No AppVeyor image found for ${imageVersion} in ${cloudId}.
                   Creating new image for ${imageVersion}, using Chromium ${CHROMIUM_VERSION} - job will run after image is baked.`);
      await bakeAppVeyorImage(branch, { ...opts, version: imageVersion, cloudId });

      // write image to temp file if running on CI
      if (process.env.CI) fs.writeFileSync('./image_version.txt', imageVersion);
    }
  }
}

module.exports = prepareAppVeyorImage;

//   Load or bake AppVeyor images for Windows CI.
//   Usage: prepare-appveyor.js [--cloudId=CLOUD_ID] [--appveyorJobId=xxx] [--imageVersion=xxx]
//   [--commit=sha] [--branch=branch_name]
if (require.main === module) {
  const args = require('minimist')(process.argv.slice(2));
  prepareAppVeyorImage(args)
    .catch((err) => {
      console.error(err);
      process.exit(1);
    });
}

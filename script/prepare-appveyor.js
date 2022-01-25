if (!process.env.CI) require('dotenv-safe').load();

const assert = require('assert');
const fs = require('fs');
const got = require('got');
const path = require('path');
const { handleGitCall, ELECTRON_DIR } = require('./lib/utils.js');

// Appveyor image constants | https://ci.appveyor.com/api/build-clouds/{buildCloudId}
const APPVEYOR_IMAGES_URL = 'https://ci.appveyor.com/api/build-clouds';
const APPVEYOR_JOB_URL = 'https://ci.appveyor.com/api/builds';
const DEFAULT_IMAGE = 'base-electron';

const appVeyorJobs = {
  'electron-x64': 'electron-x64-testing'
  // TODO: Disabling so I don't blast this across three jobs while testing
  // 'electron-ia32': 'electron-ia32-testing',
  // 'electron-woa': 'electron-woa-testing'
};
const appveyorBakeJob = 'electron-bake-image';

async function makeRequest ({ auth, url, headers, body, method }) {
  const clonedHeaders = {
    ...(headers || {})
  };
  if (auth && auth.bearer) {
    clonedHeaders.Authorization = `Bearer ${auth.bearer}`;
  }
  const response = await got(url, {
    headers: clonedHeaders,
    body,
    method,
    auth: auth && (auth.username || auth.password) ? `${auth.username}:${auth.password}` : undefined
  });
  if (response.statusCode < 200 || response.statusCode >= 300) {
    console.error('Error: ', `(status ${response.statusCode})`, response.body);
    throw new Error(`Unexpected status code ${response.statusCode} from ${url}`);
  }
  return JSON.parse(response.body);
}

async function checkAppVeyorImage (options) {
  const IMAGE_URL = `${APPVEYOR_IMAGES_URL}/${options.buildCloudId}`;
  const requestOpts = {
    url: IMAGE_URL,
    auth: {
      bearer: process.env.APPVEYOR_CLOUD_TOKEN
    },
    headers: {
      'Content-Type': 'application/json'
    },
    method: 'GET'
  };

  try {
    const { settings } = await makeRequest(requestOpts, true);
    const { cloudSettings } = settings;
    return cloudSettings.images.find(image => image.name === `${options.imageVersion}`) || null;
  } catch (err) {
    console.log('Could not call AppVeyor: ', err);
  }
}

function useAppVeyorImage (targetBranch, options) {
  const validJobs = Object.keys(appVeyorJobs);
  if (options.job) {
    assert(validJobs.includes(options.job), `Unknown AppVeyor CI job name: ${options.job}.  Valid values are: ${validJobs}.`);
    callAppVeyorBuildJobs(targetBranch, options.job, options);
  } else {
    validJobs.forEach((job) => callAppVeyorBuildJobs(targetBranch, job, options));
  }
}

async function callAppVeyorBuildJobs (targetBranch, job, options) {
  console.log('Options: ', options);
  console.log(`Using AppVeyor image ${options.version} for ${job}...`);
  const environmentVariables = {
    APPVEYOR_BUILD_WORKER_CLOUD: options.buildCloudId,
    APPVEYOR_BUILD_WORKER_IMAGE: options.version
  };

  const requestOpts = {
    url: APPVEYOR_JOB_URL,
    auth: {
      bearer: process.env.APPVEYOR_CLOUD_TOKEN
    },
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      accountName: 'electron-bot',
      projectSlug: appVeyorJobs[job],
      branch: targetBranch,
      commitId: options.commit || undefined,
      environmentVariables
    }),
    method: 'POST'
  };

  try {
    const res = await makeRequest(requestOpts, true);
    console.log('Res: ', res);
    const version = res.version;
    const buildUrl = `https://ci.appveyor.com/project/electron-bot/${appVeyorJobs[job]}/build/${version}`;
    console.log(`AppVeyor CI request for ${job} successful.  Check status at ${buildUrl}`);
  } catch (err) {
    console.log('Could not call AppVeyor: ', err);
  }
}

async function bakeAppVeyorImage (targetBranch, options) {
  console.log(`Baking a new AppVeyor image for: ${options.version}, on build cloud ${options.buildCloudId}...`);

  const environmentVariables = {
    APPVEYOR_BUILD_WORKER_CLOUD: options.buildCloudId,
    APPVEYOR_BUILD_WORKER_IMAGE: DEFAULT_IMAGE,
    APPVEYOR_BAKE_IMAGE: options.version
  };

  const requestOpts = {
    url: APPVEYOR_JOB_URL,
    auth: {
      bearer: process.env.APPVEYOR_CLOUD_TOKEN
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
    const res = await makeRequest(requestOpts, true);
    console.log('Res: ', res);
    const version = res.version;
    const bakeUrl = `https://ci.appveyor.com/project/electron-bot/${appveyorBakeJob}/build/${version}`;
    console.log(`AppVeyor image bake request for ${options.version} successful.  Check build status at ${bakeUrl}`);
  } catch (err) {
    console.log('Could not call AppVeyor: ', err);
  }
}

async function prepareAppVeyorImage (opts) {
  // eslint-disable-next-line no-control-regex
  const versionRegex = new RegExp('chromium_version\':\n +\'(.+?)\',', 'm');
  const currentBranch = await handleGitCall(['rev-parse', '--abbrev-ref', 'HEAD'], ELECTRON_DIR);
  const deps = fs.readFileSync(path.resolve(__dirname, '../DEPS'), 'utf8');
  const [, CHROMIUM_VERSION] = versionRegex.exec(deps);

  const buildCloudId = opts.buildCloudId || '1424'; // BC: electron-16-core2
  const imageVersion = opts.imageVersion || `electron-${CHROMIUM_VERSION}`;
  const image = await checkAppVeyorImage({ buildCloudId, imageVersion });
  console.log('Branch: ', currentBranch);

  if (image && image.name) {
    console.log(`Image exists for ${image.name}. Continuing AppVeyor jobs using ${buildCloudId}`);
    useAppVeyorImage(currentBranch, { ...opts, version: image.name, buildCloudId });
  } else {
    console.log(`No AppVeyor image found for ${imageVersion} in ${buildCloudId}.
                 Creating new image for ${imageVersion}, using Chromium ${CHROMIUM_VERSION} - job will run after image is baked.`);
    // Bake new image, continue running jobs in parallel using the default image
    // await bakeAppVeyorImage(currentBranch, { ...opts, version: imageVersion, buildCloudId });
    // useAppVeyorImage(currentBranch, { ...opts, version: DEFAULT_IMAGE, buildCloudId });
  }
}

module.exports = prepareAppVeyorImage;

if (require.main === module) {
  const args = require('minimist')(process.argv.slice(2));
  console.log('ARGS: ', args);
  //   Load or bake AppVeyor images for Windows CI.
  //   Usage: prepare-appveyor.js [--buildCloudId=CLOUD_ID] [--appveyorJobId=xxx] [--imageVersion=xxx]
  //   [--commit=sha] [--branch=branch_name]
  prepareAppVeyorImage(args)
    .catch((err) => {
      console.error(err);
      process.exit(1);
    });
}

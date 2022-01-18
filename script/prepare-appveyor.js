if (!process.env.CI) require('dotenv-safe').load();

const assert = require('assert');
const fs = require('fs');
const got = require('got');
const path = require('path');

// Appveyor image constants | https://ci.appveyor.com/api/build-clouds/{buildCloudId}
const APPREVYOR_IMAGES_URL = 'https://ci.appveyor.com/api/build-clouds'; // GET
const BAKE_APPVEYOR_IMAGE_URL = 'https://ci.appveyor.com/api/builds'; // POST
const USE_APPVEYOR_IMAGE_URL = 'https://ci.appveyor.com/api/builds'; // POST

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
  const IMAGE_URL = `${APPREVYOR_IMAGES_URL}/${options.buildCloudId}`;
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

async function bakeAppVeyorImage (options) {
  console.log(`Baking a new AppVeyor image for: ${options.version}, on build cloud ${options.buildCloudId}...`);

  const environmentVariables = {
    ELECTRON_RELEASE: 0,
    APPVEYOR_BUILD_WORKER_CLOUD: options.buildCloudId,
    APPVEYOR_BUILD_WORKER_IMAGE: options.version,
    APPVEYOR_BAKE_IMAGE: options.version
  };

  const requestOpts = {
    url: BAKE_APPVEYOR_IMAGE_URL,
    auth: {
      bearer: process.env.APPVEYOR_CLOUD_TOKEN
    },
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      accountName: 'electron-bot',
      // projectSlug: appVeyorJobs[job],
      // branch: targetBranch,
      commitId: options.commit || undefined,
      environmentVariables
    }),
    method: 'POST'
  };

  // try {
  //   const { version } = await makeRequest(requestOpts, true);
  //   // const buildUrl = `https://ci.appveyor.com/project/electron-bot/${appVeyorJobs[job]}/build/${version}`;
  //   // console.log(`AppVeyor release build request for ${job} successful.  Check build status at ${buildUrl}`);
  // } catch (err) {
  //   console.log('Could not call AppVeyor: ', err);
  // }
}

// TODO: We'll need to replace the webhook that calls current AppVeyor builds with a call to this function
async function useAppVeyorImage (options) {
  console.log(`Using AppVeyor image ${options.version} on build cloud ${options.buildCloudId}...`);
  const environmentVariables = {
    ELECTRON_RELEASE: 1,
    APPVEYOR_BUILD_WORKER_CLOUD: options.buildCloudId,
    APPVEYOR_BUILD_WORKER_IMAGE: options.version
  };

  const requestOpts = {
    url: USE_APPVEYOR_IMAGE_URL,
    auth: {
      bearer: process.env.APPVEYOR_CLOUD_TOKEN
    },
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      accountName: 'electron-bot',
      // projectSlug: appVeyorJobs[job],
      // branch: targetBranch,
      commitId: options.commit || undefined,
      environmentVariables
    }),
    method: 'POST'
  };

  try {
    const { version } = await makeRequest(requestOpts, true);
    // const buildUrl = `https://ci.appveyor.com/project/electron-bot/${appVeyorJobs[job]}/build/${version}`;
    // console.log(`AppVeyor release build request for ${job} successful.  Check build status at ${buildUrl}`);
  } catch (err) {
    console.log('Could not call AppVeyor: ', err);
  }
}

async function prepareAppVeyorImage (targetBranch, opts) {
  // eslint-disable-next-line no-control-regex
  const versionRegex = new RegExp('chromium_version\':\n +\'(.+?)\',', 'm');
  const deps = fs.readFileSync(path.resolve(__dirname, '../DEPS'), 'utf8');
  const [, CHROMIUM_VERSION] = versionRegex.exec(deps);

  const imageVersion = opts.imageVersion || CHROMIUM_VERSION;
  const image = await checkAppVeyorImage({ buildCloudId: opts.buildCloudId, imageVersion });

  if (image) {
    console.log(`Image exists for ${image}. Continuing AppVeyor jobs using ${opts.buildCloudId}`);
    await useAppVeyorImage({ ...opts, version: image });
  } else {
    console.log(`No AppVeyor image found for ${imageVersion} in ${opts.buildCloudId}.`);
    console.log(`Creating new image for ${imageVersion} and falling back to default image in ${opts.buildCloudId}.`);
    // await bakeAppVeyorImage({ ...opts, version: CHROMIUM_VERSION });
    // continue with an existing image
    // await useAppVeyorImage({ ...opts });
  }
}

module.exports = prepareAppVeyorImage;

if (require.main === module) {
  const args = require('minimist')(process.argv.slice(2));
  console.log('ARGS: ', args);
  const targetBranch = args._[0];
  // if (args._.length < 1) {
  //   console.log(`Load or bake AppVeyor images for Windows CI.
  //   Usage: prepare-appveyor.js [--buildCloudId=CLOUD_ID] [--appveyorJobId=xxx] [--commit=sha] TARGET_BRANCH
  //   `);
  //   process.exit(0);
  // }
  prepareAppVeyorImage(targetBranch, args)
    .catch((err) => {
      console.error(err);
      process.exit(1);
    });
}

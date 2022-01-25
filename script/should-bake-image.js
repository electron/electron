if (!process.env.CI) require('dotenv-safe').load();

const assert = require('assert');
const fs = require('fs');
const got = require('got');
const path = require('path');

// Appveyor image constants | https://ci.appveyor.com/api/build-clouds/{buildCloudId}
const APPREVYOR_IMAGES_URL = 'https://ci.appveyor.com/api/build-clouds'; // GET
const BAKE_APPVEYOR_IMAGE_URL = 'https://ci.appveyor.com/api/builds'; // POST

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

async function prepareAppVeyorImage (opts) {
  // eslint-disable-next-line no-control-regex
  const versionRegex = new RegExp('chromium_version\':\n +\'(.+?)\',', 'm');
  const deps = fs.readFileSync(path.resolve(__dirname, '../DEPS'), 'utf8');
  const [, CHROMIUM_VERSION] = versionRegex.exec(deps);

  const buildCloudId = opts.buildCloudId || '1424'; // BC: electron-16-core2
  const imageVersion = opts.imageVersion || `electron-${CHROMIUM_VERSION}`;
  const image = await checkAppVeyorImage({ buildCloudId, imageVersion });

  if (image) {
    // console.log(`Image found for ${imageVersion} on cloud: ${buildCloudId}`);
    // console.log(`Image found for ${imageVersion} on cloud: ${buildCloudId}`);
    return `0, ${imageVersion}`;
  } else {
    // console.log(`Image not found! Baking new image for ${imageVersion} on cloud: ${buildCloudId}`);
    return `1, ${imageVersion}`;
  }
}

module.exports = prepareAppVeyorImage;

if (require.main === module) {
  const args = require('minimist')(process.argv.slice(2));
  prepareAppVeyorImage(args)
    .catch((err) => {
      console.error(err);
      process.exit(1);
    });
}

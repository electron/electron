const args = require('minimist')(process.argv.slice(2));
const fs = require('fs');
const got = require('got');
const stream = require('stream');
const { promisify } = require('util');

const pipeline = promisify(stream.pipeline);

async function downloadArtifact (name, buildNum, dest) {
  const circleArtifactUrl = `https://circleci.com/api/v1.1/project/github/electron/electron/${args.buildNum}/artifacts?circle-token=${process.env.CIRCLE_TOKEN}`;
  const responsePromise = got(circleArtifactUrl, {
    headers: {
      'Content-Type': 'application/json',
      Accept: 'application/json'
    }
  });
  const [response, artifacts] = await Promise.all([responsePromise, responsePromise.json()]);
  if (response.statusCode !== 200) {
    console.error('Could not fetch circleci artifact list, got status code:', response.statusCode);
  }
  const artifactToDownload = artifacts.find(artifact => {
    return (artifact.path === name);
  });
  if (!artifactToDownload) {
    console.log(`Could not find artifact called ${name} to download for build #${buildNum}.`);
    process.exit(1);
  } else {
    console.log(`Downloading ${artifactToDownload.url}.`);
    let downloadError = false;
    await downloadWithRetry(artifactToDownload.url, dest).catch(err => {
      if (args.verbose) {
        console.log(`${artifactToDownload.url} could not be successfully downloaded.  Error was:`, err);
      } else {
        console.log(`${artifactToDownload.url} could not be successfully downloaded.`);
      }
      downloadError = true;
    });
    if (!downloadError) {
      console.log(`Successfully downloaded ${name}.`);
    }
  }
}

async function downloadWithRetry (url, directory) {
  let lastError;
  const downloadURL = `${url}?circle-token=${process.env.CIRCLE_TOKEN}`;
  for (let i = 0; i < 5; i++) {
    console.log(`Attempting to download ${url} - attempt #${(i + 1)}`);
    try {
      return await downloadFile(downloadURL, directory);
    } catch (err) {
      lastError = err;
      await new Promise((resolve, reject) => setTimeout(resolve, 30000));
    }
  }
  throw lastError;
}

function downloadFile (url, directory) {
  return pipeline(
    got.stream(url),
    fs.createWriteStream(directory)
  );
}

if (!args.name || !args.buildNum || !args.dest) {
  console.log(`Download CircleCI artifacts.
    Usage: download-circleci-artifacts.js [--buildNum=CIRCLE_BUILD_NUMBER] [--name=artifactName] [--dest] [--verbose]`);
  process.exit(0);
} else {
  downloadArtifact(args.name, args.buildNum, args.dest);
}

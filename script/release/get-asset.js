const { Octokit } = require('@octokit/rest');
const got = require('got');

const octokit = new Octokit({
  userAgent: 'electron-asset-fetcher',
  auth: process.env.ELECTRON_GITHUB_TOKEN
});

async function getAssetContents (repo, assetId) {
  const requestOptions = octokit.repos.getReleaseAsset.endpoint({
    owner: 'electron',
    repo,
    asset_id: assetId,
    headers: {
      Accept: 'application/octet-stream'
    }
  });

  const { url, headers } = requestOptions;
  headers.authorization = `token ${process.env.ELECTRON_GITHUB_TOKEN}`;

  const response = await got(url, {
    followRedirect: false,
    method: 'HEAD',
    headers
  });
  if (!response.headers.location) {
    console.error(response.headers, `${response.body}`.slice(0, 300));
    throw new Error(`cannot find asset[${assetId}], asset download did not redirect`);
  }

  const fileResponse = await got(response.headers.location);
  if (fileResponse.statusCode !== 200) {
    console.error(fileResponse.headers, `${fileResponse.body}`.slice(0, 300));
    throw new Error(`cannot download asset[${assetId}] from ${response.headers.location}, got status: ${fileResponse.status}`);
  }

  return fileResponse.body;
}

module.exports = {
  getAssetContents
};

import { Octokit } from '@octokit/rest';

import { createGitHubTokenStrategy } from './github-token';
import { ELECTRON_ORG, ElectronReleaseRepo } from './types';

export async function getAssetContents(repo: ElectronReleaseRepo, assetId: number) {
  const octokit = new Octokit({
    userAgent: 'electron-asset-fetcher',
    authStrategy: createGitHubTokenStrategy(repo)
  });

  const requestOptions = octokit.repos.getReleaseAsset.endpoint({
    owner: ELECTRON_ORG,
    repo,
    asset_id: assetId,
    headers: {
      Accept: 'application/octet-stream'
    }
  });

  const { url, headers } = requestOptions;
  headers.authorization = `token ${((await octokit.auth()) as { token: string }).token}`;

  const response = await fetch(url, {
    redirect: 'manual',
    method: 'HEAD',
    headers: headers as Record<string, string>
  });

  if (response.status !== 302 && response.status !== 301) {
    console.error('Failed to HEAD github asset contents for redirect: ' + url);
    throw new Error("Unexpected status HEAD'ing github asset for redirect: " + response.status);
  }

  const location = response.headers.get('location');
  if (!location) {
    console.error(Object.fromEntries(response.headers), (await response.text()).slice(0, 300));
    throw new Error(`cannot find asset[${assetId}], asset download did not redirect`);
  }

  const fileResponse = await fetch(location);
  const body = await fileResponse.text();

  if (fileResponse.status !== 200) {
    console.error(Object.fromEntries(fileResponse.headers), body.slice(0, 300));
    throw new Error(`cannot download asset[${assetId}] from ${location}, got status: ${fileResponse.status}`);
  }

  return body;
}

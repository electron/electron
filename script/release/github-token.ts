import { createTokenAuth } from '@octokit/auth-token';
import got from 'got';

import { ElectronReleaseRepo } from './types';

const cachedTokens = Object.create(null);

async function ensureToken (repo: ElectronReleaseRepo) {
  if (!cachedTokens[repo]) {
    cachedTokens[repo] = await (async () => {
      const { ELECTRON_GITHUB_TOKEN, SUDOWOODO_EXCHANGE_URL, SUDOWOODO_EXCHANGE_TOKEN } = process.env;
      if (ELECTRON_GITHUB_TOKEN) {
        return ELECTRON_GITHUB_TOKEN;
      }

      if (SUDOWOODO_EXCHANGE_URL && SUDOWOODO_EXCHANGE_TOKEN) {
        const resp = await got.post(SUDOWOODO_EXCHANGE_URL + '?repo=' + repo, {
          headers: {
            Authorization: SUDOWOODO_EXCHANGE_TOKEN
          },
          throwHttpErrors: false
        });
        if (resp.statusCode !== 200) {
          console.error('bad sudowoodo exchange response code:', resp.statusCode);
          throw new Error('non-200 status code received from sudowoodo exchange function');
        }
        try {
          return JSON.parse(resp.body).token;
        } catch {
          // Swallow as the error could include the token
          throw new Error('Unexpected error parsing sudowoodo exchange response');
        }
      }

      throw new Error('Could not find or fetch a valid GitHub Auth Token');
    })();
  }
}

export const createGitHubTokenStrategy = (repo: ElectronReleaseRepo) => () => {
  let tokenAuth: ReturnType<typeof createTokenAuth> | null = null;

  async function ensureTokenAuth (): Promise<ReturnType<typeof createTokenAuth>> {
    if (!tokenAuth) {
      await ensureToken(repo);
      tokenAuth = createTokenAuth(cachedTokens[repo]);
    }
    return tokenAuth;
  }

  async function auth () {
    return await (await ensureTokenAuth())();
  }
  const hook: ReturnType<typeof createTokenAuth>['hook'] = async (...args) => {
    const a = (await ensureTokenAuth());
    return (a as any).hook(...args);
  };
  auth.hook = hook;
  return auth;
};

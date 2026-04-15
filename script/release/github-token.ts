import { createTokenAuth } from '@octokit/auth-token';
import got from 'got';

import { ElectronReleaseRepo } from './types';

const cachedTokens = Object.create(null);

const SUDOWOODO_OIDC_AUDIENCE = 'sudowoodo-broker';

async function getActionsIdToken (): Promise<string> {
  const { ACTIONS_ID_TOKEN_REQUEST_URL, ACTIONS_ID_TOKEN_REQUEST_TOKEN } = process.env;
  if (!ACTIONS_ID_TOKEN_REQUEST_URL || !ACTIONS_ID_TOKEN_REQUEST_TOKEN) {
    throw new Error(
      'ACTIONS_ID_TOKEN_REQUEST_URL/_TOKEN not set — the job needs `permissions: id-token: write` to mint an OIDC token for the sudowoodo exchange'
    );
  }
  const { value } = await got(ACTIONS_ID_TOKEN_REQUEST_URL + '&audience=' + SUDOWOODO_OIDC_AUDIENCE, {
    headers: {
      authorization: 'Bearer ' + ACTIONS_ID_TOKEN_REQUEST_TOKEN
    }
  }).json<{ value: string }>();
  return value;
}

async function ensureToken (repo: ElectronReleaseRepo) {
  if (!cachedTokens[repo]) {
    cachedTokens[repo] = await (async () => {
      const { ELECTRON_GITHUB_TOKEN, SUDOWOODO_EXCHANGE_URL } = process.env;
      if (ELECTRON_GITHUB_TOKEN) {
        return ELECTRON_GITHUB_TOKEN;
      }

      if (SUDOWOODO_EXCHANGE_URL) {
        const idToken = await getActionsIdToken();
        const resp = await got.post(SUDOWOODO_EXCHANGE_URL + '?repo=' + repo, {
          headers: {
            Authorization: 'Bearer ' + idToken
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

import { createTokenAuth } from '@octokit/auth-token';

import { ElectronReleaseRepo } from './types';

const cachedTokens = Object.create(null);

const SUDOWOODO_OIDC_AUDIENCE = 'sudowoodo-broker';

async function getActionsIdToken(): Promise<string> {
  const { ACTIONS_ID_TOKEN_REQUEST_URL, ACTIONS_ID_TOKEN_REQUEST_TOKEN } = process.env;
  if (!ACTIONS_ID_TOKEN_REQUEST_URL || !ACTIONS_ID_TOKEN_REQUEST_TOKEN) {
    throw new Error(
      'ACTIONS_ID_TOKEN_REQUEST_URL/_TOKEN not set — the job needs `permissions: id-token: write` to mint an OIDC token for the sudowoodo exchange'
    );
  }
  const resp = await fetch(ACTIONS_ID_TOKEN_REQUEST_URL + '&audience=' + SUDOWOODO_OIDC_AUDIENCE, {
    headers: {
      authorization: 'Bearer ' + ACTIONS_ID_TOKEN_REQUEST_TOKEN
    }
  });
  if (!resp.ok) {
    throw new Error(`Failed to request an Actions OIDC token, got status: ${resp.status}`);
  }
  const { value } = (await resp.json()) as { value: string };
  return value;
}

async function ensureToken(repo: ElectronReleaseRepo) {
  if (!cachedTokens[repo]) {
    cachedTokens[repo] = await (async () => {
      const { ELECTRON_GITHUB_TOKEN, SUDOWOODO_EXCHANGE_URL } = process.env;
      if (ELECTRON_GITHUB_TOKEN) {
        return ELECTRON_GITHUB_TOKEN;
      }

      if (SUDOWOODO_EXCHANGE_URL) {
        const idToken = await getActionsIdToken();
        const resp = await fetch(SUDOWOODO_EXCHANGE_URL + '?repo=' + repo, {
          method: 'POST',
          headers: {
            Authorization: 'Bearer ' + idToken
          }
        });
        if (resp.status !== 200) {
          console.error('bad sudowoodo exchange response code:', resp.status);
          throw new Error('non-200 status code received from sudowoodo exchange function');
        }
        try {
          return JSON.parse(await resp.text()).token;
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

  async function ensureTokenAuth(): Promise<ReturnType<typeof createTokenAuth>> {
    if (!tokenAuth) {
      await ensureToken(repo);
      tokenAuth = createTokenAuth(cachedTokens[repo]);
    }
    return tokenAuth;
  }

  async function auth() {
    return await (
      await ensureTokenAuth()
    )();
  }
  const hook: ReturnType<typeof createTokenAuth>['hook'] = async (...args) => {
    const a = await ensureTokenAuth();
    return (a as any).hook(...args);
  };
  auth.hook = hook;
  return auth;
};

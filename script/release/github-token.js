const { createTokenAuth } = require('@octokit/auth-token');
const got = require('got').default;

const cachedTokens = Object.create(null);

async function ensureToken (repo) {
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

module.exports.createGitHubTokenStrategy = (repo) => () => {
  let tokenAuth = null;

  async function ensureTokenAuth () {
    if (!tokenAuth) {
      await ensureToken(repo);
      tokenAuth = createTokenAuth(cachedTokens[repo]);
    }
  }

  async function auth () {
    await ensureTokenAuth();
    return await tokenAuth();
  }
  auth.hook = async (...args) => {
    await ensureTokenAuth();
    return await tokenAuth.hook(...args);
  };
  return auth;
};

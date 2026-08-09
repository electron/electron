import * as url from 'node:url';

const HASHER_FUNCTION_HOST = 'electron-hasher.azurewebsites.net';
const HASHER_FUNCTION_ROUTE = '/api/hashRemoteAsset';

export async function getUrlHash(targetUrl: string, algorithm = 'sha256', attempts = 3) {
  const options = {
    code: process.env.ELECTRON_HASHER_FUNCTION_KEY!,
    targetUrl,
    algorithm
  };
  const search = new url.URLSearchParams(options);
  const functionUrl = url.format({
    protocol: 'https:',
    hostname: HASHER_FUNCTION_HOST,
    pathname: HASHER_FUNCTION_ROUTE,
    search: search.toString()
  });
  try {
    const resp = await fetch(functionUrl);
    const body = await resp.text();
    if (resp.status !== 200) {
      console.error('bad hasher function response:', body.trim());
      throw new Error('non-200 status code received from hasher function');
    }
    if (!body) throw new Error('Successful lambda call but failed to get valid hash');

    // response shape should be { hash: 'xyz', invocationId: "abc"}
    const { hash } = JSON.parse(body.trim());
    return hash;
  } catch (err) {
    if (attempts > 1) {
      console.error(`Failed to get URL hash for ${targetUrl} - we will retry`, err);
      return getUrlHash(targetUrl, algorithm, attempts - 1);
    }
    throw err;
  }
}

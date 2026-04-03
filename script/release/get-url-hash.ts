import got from 'got';

import * as url from 'node:url';

const HASHER_FUNCTION_HOST = 'electron-hasher.azurewebsites.net';
const HASHER_FUNCTION_ROUTE = '/api/hashRemoteAsset';
const HASHER_URL_OVERRIDE = process.env.ELECTRON_HASHER_URL;

export async function getUrlHash (targetUrl: string, algorithm = 'sha256', attempts = 3) {
  const search = new url.URLSearchParams({ targetUrl, algorithm });
  let functionUrl: string;
  if (HASHER_URL_OVERRIDE) {
    functionUrl = `${HASHER_URL_OVERRIDE.replace(/\/$/, '')}${HASHER_FUNCTION_ROUTE}?${search.toString()}`;
  } else {
    search.set('code', process.env.ELECTRON_HASHER_FUNCTION_KEY!);
    functionUrl = url.format({
      protocol: 'https:',
      hostname: HASHER_FUNCTION_HOST,
      pathname: HASHER_FUNCTION_ROUTE,
      search: search.toString()
    });
  }
  try {
    const resp = await got(functionUrl, {
      throwHttpErrors: false
    });
    if (resp.statusCode !== 200) {
      console.error('bad hasher function response:', resp.body.trim());
      throw new Error('non-200 status code received from hasher function');
    }
    if (!resp.body) throw new Error('Successful lambda call but failed to get valid hash');

    // response shape should be { hash: 'xyz', invocationId: "abc"}
    const { hash } = JSON.parse(resp.body.trim());
    return hash;
  } catch (err) {
    if (attempts > 1) {
      const { response } = err as any;
      if (response?.body) {
        console.error(`Failed to get URL hash for ${targetUrl} - we will retry`, {
          statusCode: response.statusCode,
          body: JSON.parse(response.body)
        });
      } else {
        console.error(`Failed to get URL hash for ${targetUrl} - we will retry`, err);
      }
      return getUrlHash(targetUrl, algorithm, attempts - 1);
    }
    throw err;
  }
};

import got from 'got';

import * as url from 'node:url';

const HASHER_FUNCTION_HOST = 'electron-artifact-hasher.azurewebsites.net';
const HASHER_FUNCTION_ROUTE = '/api/HashArtifact';

export async function getUrlHash (targetUrl: string, algorithm = 'sha256', attempts = 3) {
  const options = {
    code: process.env.ELECTRON_ARTIFACT_HASHER_FUNCTION_KEY!,
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
    const resp = await got(functionUrl, {
      throwHttpErrors: false
    });
    if (resp.statusCode !== 200) {
      console.error('bad hasher function response:', resp.body.trim());
      throw new Error('non-200 status code received from hasher function');
    }
    if (!resp.body) throw new Error('Successful lambda call but failed to get valid hash');

    return resp.body.trim();
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

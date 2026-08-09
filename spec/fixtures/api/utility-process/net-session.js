const { net } = require('electron');

process.parentPort.on('message', async (e) => {
  const { type, url, options } = e.data;

  if (type === 'fetch') {
    try {
      const response = await net.fetch(url, options || {});
      const body = await response.text();
      const headers = {};
      response.headers.forEach((value, key) => {
        headers[key] = value;
      });
      process.parentPort.postMessage({
        ok: response.ok,
        status: response.status,
        body,
        headers
      });
    } catch (error) {
      process.parentPort.postMessage({
        ok: false,
        error: error.message
      });
    }
  } else if (type === 'fetch-cached') {
    try {
      const response1 = await net.fetch(url);
      const body1 = await response1.text();

      const fetchOpts = {};
      if (e.data.cacheMode) {
        fetchOpts.cache = e.data.cacheMode;
      }
      const response2 = await net.fetch(url, fetchOpts);
      const body2 = await response2.text();

      process.parentPort.postMessage({
        ok: true,
        first: { status: response1.status, body: body1 },
        second: { status: response2.status, body: body2 }
      });
    } catch (error) {
      process.parentPort.postMessage({
        ok: false,
        error: error.message
      });
    }
  } else if (type === 'net-request-login') {
    const request = net.request({ method: 'GET', url });
    request.on('login', (authInfo, callback) => {
      process.parentPort.postMessage({ event: 'login', authInfo });
      callback('user', 'pass');
    });
    request.on('response', (response) => {
      const data = [];
      response.on('data', (chunk) => data.push(chunk));
      response.on('end', () => {
        process.parentPort.postMessage({
          event: 'response',
          statusCode: response.statusCode
        });
      });
    });
    request.end();
  } else if (type === 'fetch-auth') {
    try {
      const response = await net.fetch(url);
      const body = await response.text();
      process.parentPort.postMessage({
        event: 'response',
        status: response.status,
        body
      });
    } catch (error) {
      process.parentPort.postMessage({
        event: 'error',
        error: error.message
      });
    }
  }
});

process.parentPort.postMessage({ type: 'ready' });

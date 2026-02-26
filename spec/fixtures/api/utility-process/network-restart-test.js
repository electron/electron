const { net } = require('electron');

process.parentPort.on('message', async (e) => {
  const { type, url } = e.data;

  if (type === 'fetch') {
    try {
      const response = await net.fetch(url);
      const body = await response.text();
      process.parentPort.postMessage({
        ok: response.ok,
        status: response.status,
        body
      });
    } catch (error) {
      process.parentPort.postMessage({
        ok: false,
        error: error.message
      });
    }
  }
});

process.parentPort.postMessage({ type: 'ready' });

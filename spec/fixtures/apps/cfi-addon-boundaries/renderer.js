const { ipcRenderer } = require('electron');

const report = (promise) => {
  promise
    .then(() => {
      ipcRenderer.send('cfi-addon-boundary-result', 'ok');
    })
    .catch((error) => {
      ipcRenderer.send('cfi-addon-boundary-result', error.stack);
    });
};

const mode = new URLSearchParams(location.search).get('mode');

if (mode === 'renderer-web-worker') {
  report(
    new Promise((resolve, reject) => {
      const worker = new Worker('web-worker.js');
      worker.onmessage = ({ data }) => {
        worker.terminate();
        data === 'ok' ? resolve() : reject(new Error(data));
      };
      worker.onerror = reject;
    })
  );
} else if (mode === 'renderer-main-world') {
  const runEmbedderCallbacks = require('../../module/run-v8-embedder-callbacks');
  report(runEmbedderCallbacks({ useCounter: false }));
}

const { ipcRenderer } = require('electron');
const runEmbedderCallbacks = require('../../module/run-v8-embedder-callbacks');

runEmbedderCallbacks({ useCounter: false })
  .then(() => {
    ipcRenderer.send('cfi-addon-boundary-result', 'ok');
  })
  .catch((error) => {
    ipcRenderer.send('cfi-addon-boundary-result', error.stack);
  });

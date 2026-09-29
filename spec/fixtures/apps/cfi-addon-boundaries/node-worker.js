const { parentPort } = require('node:worker_threads');
const runEmbedderCallbacks = require('../../module/run-v8-embedder-callbacks');

runEmbedderCallbacks()
  .then(() => {
    parentPort.postMessage('ok');
  })
  .catch((error) => {
    throw error;
  });

const runEmbedderCallbacks = require('../../module/run-v8-embedder-callbacks');

runEmbedderCallbacks()
  .then(() => {
    process.parentPort.postMessage('ok');
    setImmediate(() => process.exit(0));
  })
  .catch((error) => {
    throw error;
  });

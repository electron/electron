const runEmbedderCallbacks = require('../../module/run-v8-embedder-callbacks');

runEmbedderCallbacks({ useCounter: false })
  .then(() => {
    postMessage('ok');
  })
  .catch((error) => {
    postMessage(error.stack);
  });

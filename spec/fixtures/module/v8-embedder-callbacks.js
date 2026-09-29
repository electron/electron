const runEmbedderCallbacks = require('./run-v8-embedder-callbacks');

runEmbedderCallbacks()
  .then(() => {
    console.log('ok');
  })
  .catch((error) => {
    console.error(error);
    process.exitCode = 1;
  });

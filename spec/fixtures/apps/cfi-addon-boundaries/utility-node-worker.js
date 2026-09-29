const { once } = require('node:events');
const path = require('node:path');
const { Worker } = require('node:worker_threads');

(async () => {
  const worker = new Worker(path.join(__dirname, 'node-worker.js'));
  const messagePromise = once(worker, 'message');
  const exitPromise = once(worker, 'exit');
  const [message] = await messagePromise;
  const [code] = await exitPromise;
  if (message !== 'ok' || code !== 0) {
    throw new Error(`Worker failed: message=${message}, code=${code}`);
  }
  process.parentPort.postMessage('ok');
  setImmediate(() => process.exit(0));
})().catch((error) => {
  throw error;
});

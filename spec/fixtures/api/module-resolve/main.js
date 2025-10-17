const moduleExports = require('test-module-resolve');

const assert = require('node:assert');
const { Worker } = require('node:worker_threads');

assert.strictEqual(moduleExports, 'electron', 'Module should resolve to "electron" in the main process');

const workerCode = `
  const moduleExports = require('test-module-resolve');
  const { parentPort } = require('node:worker_threads');
  parentPort.postMessage(moduleExports);
`;

const worker = new Worker(workerCode, { eval: true });
worker.on('message', (message) => {
  assert.strictEqual(message, 'default', 'Module should resolve to "default" in a worker thread');
  process.exit(0);
});
worker.on('error', (err) => {
  console.error('Worker error:', err);
  process.exit(1);
});

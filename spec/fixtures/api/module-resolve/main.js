const moduleExports = require('test-module-resolve');

const assert = require('node:assert');
const { isMainThread } = require('node:worker_threads');

if (isMainThread) {
  assert.strictEqual(moduleExports, 'electron', 'Module should resolve to "electron" in the main process');
} else {
  assert.strictEqual(moduleExports, 'default', 'Module should resolve to "node" in a worker thread');
}

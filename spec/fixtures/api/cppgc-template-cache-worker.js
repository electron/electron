const assert = require('node:assert/strict');
const { parentPort } = require('node:worker_threads');

const testing = process._linkedBinding('electron_common_testing');
const v8Util = process._linkedBinding('electron_common_v8_util');

// A worker isolate has no gin::PerIsolateData, so templates are not cached:
// each call must still work and yields a distinct function.
const first = testing.getCachedCallbackTemplateForTesting();
const second = testing.getCachedCallbackTemplateForTesting();
assert.equal(first(), 42);
assert.equal(second(), 42);
assert.notEqual(first, second);

v8Util.requestGarbageCollectionForTesting();
assert.equal(first(), 42);

parentPort.postMessage('done');

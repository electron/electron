const assert = require('node:assert/strict');
const { once } = require('node:events');
const { setImmediate } = require('node:timers/promises');
const { parentPort, workerData } = require('node:worker_threads');

const testing = process._linkedBinding('electron_common_testing');
const v8Util = process._linkedBinding('electron_common_v8_util');

assert.deepEqual(testing.getGinDataForTesting(), {
  hasIsolateData: false,
  hasContextData: true
});

if (workerData === 'startup-failure') {
  assert.equal(testing.getCachedCallbackHolderProbeForTesting()(), 42);
  throw new Error('callback cache worker startup failure');
}

async function main() {
  const baseline = testing.getLiveCallbackHolderProbeCountForTesting();
  let probe = testing.createCallbackHolderProbeForTesting();
  const cached = testing.getCachedCallbackHolderProbeForTesting();
  assert.equal(cached, testing.getCachedCallbackHolderProbeForTesting());
  const weak = new WeakRef(probe);
  await setImmediate();
  await globalThis.gc({ execution: 'async', type: 'major' });
  assert.equal(probe.run(), 42);
  assert.equal(probe.value, 42);
  assert.equal(cached(), 42);
  assert.equal(cached, testing.getCachedCallbackHolderProbeForTesting());
  assert.equal(testing.getLiveCallbackHolderProbeCountForTesting(), baseline + 2);

  const command = once(parentPort, 'message');
  parentPort.postMessage('ready');
  const [mode] = await command;
  if (mode === 'gc') {
    probe = null;
    for (let i = 0; i < 20; i++) {
      await setImmediate();
      v8Util.requestGarbageCollectionForTesting();
      if (weak.deref() === undefined && testing.getLiveCallbackHolderProbeCountForTesting() === baseline + 1) {
        break;
      }
    }
    assert.equal(weak.deref(), undefined);
    assert.equal(testing.getLiveCallbackHolderProbeCountForTesting(), baseline + 1);
    assert.equal(cached(), 42);
  } else {
    assert.equal(mode, 'exit');
    // Leave callbacks rooted so only worker heap teardown can release them.
    globalThis.callbackHolderProbe = probe;
  }
  parentPort.close();
}

main().catch((error) => {
  throw error;
});

const assert = require('node:assert/strict');
const { once } = require('node:events');
const { setImmediate } = require('node:timers/promises');
const { parentPort } = require('node:worker_threads');

const testing = process._linkedBinding('electron_common_testing');
const v8Util = process._linkedBinding('electron_common_v8_util');

async function main() {
  const baseline = testing.getLiveCallbackHolderProbeCountForTesting();
  let probe = testing.createCallbackHolderProbeForTesting();
  const weak = new WeakRef(probe);
  await setImmediate();
  await globalThis.gc({ execution: 'async', type: 'major' });
  assert.equal(probe(), 42);
  assert.equal(testing.getLiveCallbackHolderProbeCountForTesting(), baseline + 1);

  const command = once(parentPort, 'message');
  parentPort.postMessage('ready');
  const [mode] = await command;
  if (mode === 'gc') {
    probe = null;
    for (let i = 0; i < 20; i++) {
      await setImmediate();
      v8Util.requestGarbageCollectionForTesting();
      if (weak.deref() === undefined && testing.getLiveCallbackHolderProbeCountForTesting() === baseline) {
        break;
      }
    }
    assert.equal(weak.deref(), undefined);
    assert.equal(testing.getLiveCallbackHolderProbeCountForTesting(), baseline);
  } else {
    assert.equal(mode, 'exit');
    // Leave the callback rooted so only worker heap teardown can release it.
    globalThis.callbackHolderProbe = probe;
  }
  parentPort.close();
}

main().catch((error) => {
  throw error;
});

'use strict';

const assert = require('assert');
const binding = require('@electron-ci/external-array-buffer/build/Release/binding.node');

function tick (x) {
  return new Promise((resolve) => {
    setImmediate(function ontick () {
      if (--x === 0) {
        resolve();
      } else {
        setImmediate(ontick);
      }
    });
  });
}

async function invokeGc () {
  // it seems calling window.gc once does not guarantee garbage will be
  // collected, so we repeat 10 times with interval of 100 ms
  for (let i = 0; i < 10; i++) {
    global.gc();
    await tick(100);
  }
}

{
  const test = binding.arraybuffer.createExternalBufferWithFinalize();
  binding.arraybuffer.checkBuffer(test);
  assert.strictEqual(0, binding.arraybuffer.getFinalizeCount());
};

(async () => {
  await invokeGc();
  assert.strictEqual(1, binding.arraybuffer.getFinalizeCount());
})();

process.exit(0);

// Runs test bodies from spec/api-net.spec.ts (and one from
// spec/api-app.spec.ts) inside a utility process. Each body arrives as source
// text, so everything it refers to from those spec files' imports has to be in
// scope here under the same name.
/* eslint-disable @typescript-eslint/no-unused-vars */
const { app, net, protocol, session, utilityProcess } = require('electron/main');

const { once } = require('node:events');
const fs = require('node:fs');
const http = require('node:http');
const http2 = require('node:http2');
const https = require('node:https');
const path = require('node:path');
const { setTimeout } = require('node:timers/promises');
const v8 = require('node:v8');

const {
  collectStreamBody,
  collectStreamBodyBuffer,
  getResponse,
  kOneKiloByte,
  kOneMegaByte,
  randomBuffer,
  randomString,
  respondNTimes,
  respondOnce
} = require('../../../lib/net-helpers.ts');
const { listen, defer, ifdescribe, isTestingBindingAvailable } = require('../../../lib/spec-helpers.ts');

const { expect } = require('../../../lib/vitest-cjs.cjs');
const electronNet = net;

v8.setFlagsFromString('--expose_gc');

// Posts |result| and exits with |code| once the parent has acknowledged it
// (see waitForUtilityResult in spec/lib/utility-process-helpers.ts).
// postMessage() is an asynchronous write and process.exit() does not wait for
// pending writes, so exiting right after posting can lose the result.
function finish(result, code) {
  // Leftover async work from the test body can still fail while this waits for the ack.
  // Log it rather than let it change the exit code of a test that has already reported its result.
  const logError = (err) => console.error('error after the test result was posted:', err);
  process.on('uncaughtException', logError);
  process.on('unhandledRejection', logError);
  process.parentPort.once('message', () => process.exit(code));
  process.parentPort.postMessage(result);
}

process.parentPort.once('message', async (e) => {
  // Equivalent of beforeEach in spec/api-net.spec.ts
  respondNTimes.routeFailure = false;

  try {
    if (e.data.args) {
      for (const [key, value] of Object.entries(e.data.args)) {
        // eslint-disable-next-line no-eval
        eval(`var ${key} = value;`);
      }
    }
    // eslint-disable-next-line no-eval
    await eval(e.data.fn);
  } catch (err) {
    finish({ ok: false, message: `${err}` }, 1);
    return;
  }

  // Equivalent of afterEach in spec/api-net.spec.ts
  if (respondNTimes.routeFailure) {
    finish(
      {
        ok: false,
        message:
          'Failing this test due an unhandled error in the respondOnce route handler, check the logs above for the actual error'
      },
      1
    );
    return;
  }

  // Test passed
  finish({ ok: true }, 0);
});

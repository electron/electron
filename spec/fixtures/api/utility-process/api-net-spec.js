// Runs test bodies from spec/api-net.spec.ts (and one from
// spec/api-app.spec.ts) inside a utility process. Each body arrives as source
// text, so everything it refers to from those spec files' imports has to be in
// scope here under the same name.
/* oxlint-disable @typescript-eslint/no-unused-vars */
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

function fail(message) {
  process.parentPort.postMessage({ ok: false, message });
}

process.parentPort.on('message', async (e) => {
  // Equivalent of beforeEach in spec/api-net.spec.ts
  respondNTimes.routeFailure = false;

  try {
    if (e.data.args) {
      for (const [key, value] of Object.entries(e.data.args)) {
        // oxlint-disable-next-line no-eval
        eval(`var ${key} = value;`);
      }
    }
    // oxlint-disable-next-line no-eval
    await eval(e.data.fn);
  } catch (err) {
    fail(`${err}`);
    process.exit(1);
  }

  // Equivalent of afterEach in spec/api-net.spec.ts
  if (respondNTimes.routeFailure) {
    fail(
      'Failing this test due an unhandled error in the respondOnce route handler, check the logs above for the actual error'
    );
    process.exit(1);
  }

  // Test passed
  process.parentPort.postMessage({ ok: true });
  process.exit(0);
});

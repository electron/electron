/* eslint-disable @typescript-eslint/no-unused-vars */
/* eslint-disable camelcase */
require('ts-node/register');

const main_1 = require('electron/main');

const chai_1 = require('chai');

const node_events_1 = require('node:events');
const http = require('node:http');
const promises_1 = require('node:timers/promises');
const url = require('node:url');
const v8 = require('node:v8');

const net_helpers_1 = require('../../../lib/net-helpers');

v8.setFlagsFromString('--expose_gc');
chai_1.use(require('chai-as-promised'));
chai_1.use(require('dirty-chai'));

function fail (message) {
  process.parentPort.postMessage({ ok: false, message });
}

process.parentPort.on('message', async (e) => {
  // Equivalent of beforeEach in spec/api-net-spec.ts
  net_helpers_1.respondNTimes.routeFailure = false;

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
    fail(`${err}`);
    process.exit(1);
  }

  // Equivalent of afterEach in spec/api-net-spec.ts
  if (net_helpers_1.respondNTimes.routeFailure) {
    fail('Failing this test due an unhandled error in the respondOnce route handler, check the logs above for the actual error');
    process.exit(1);
  }

  // Test passed
  process.parentPort.postMessage({ ok: true });
  process.exit(0);
});

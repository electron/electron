// Wraps node::ObjectWrap instances and garbage-collects them while no context
// is entered, as V8 does from platform tasks (idle-time GC, memory reducer).
// Since Node.js 24.19.0, ~ObjectWrap() calls RemoveEnvironmentCleanupHook()
// from V8's weak callback; without nodejs/node#63985 that hits
// CHECK_NOT_NULL(env) and aborts the process when no context is entered.
// See https://github.com/electron/electron/issues/53387.
const v8 = require('node:v8');

const { Wrapped, collectGarbageWithoutContext } = require('@electron-ci/object-wrap');

v8.setFlagsFromString('--expose-gc');

function churn() {
  for (let i = 0; i < 1000; i++) {
    // eslint-disable-next-line no-new
    new Wrapped();
  }
}

churn();
collectGarbageWithoutContext();

// Keep some instances alive so their cleanup hooks run at environment
// teardown, exercising CleanupHookThunkRun() (nodejs/node#65630).
globalThis.keepAlive = Array.from({ length: 10 }, () => new Wrapped());

setImmediate(() => {
  churn();
  collectGarbageWithoutContext();
  console.log('ok');
});

const waitMs = (msec) => new Promise((resolve) => setTimeout(resolve, msec));

const intervalMsec = 100;
const numIterations = 2;
let curIteration = 0;
let promise;

for (let i = 0; i < numIterations; i++) {
  promise = (promise || waitMs(intervalMsec)).then(() => {
    ++curIteration;
    return waitMs(intervalMsec);
  });
}

// https://github.com/electron/electron/issues/21515 was about electron
// exiting before promises finished. This test sets the pending exitCode
// to failure, then resets it to success only if all promises finish.
process.exitCode = 1;
promise.then(() => {
  if (curIteration === numIterations) {
    process.exitCode = 0;
  }
});

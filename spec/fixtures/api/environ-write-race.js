// A worker thread reads the environment while this script adds variables.
// os.homedir() reaches getenv() through libuv without taking Node's
// environment lock, the same way Chromium's threads and system libraries do.
//
// Everything runs synchronously in the main script and the app exits before
// it is ready, so nothing else in the process is running at exit.
const { app } = require('electron');

const { Worker } = require('node:worker_threads');

const STARTING = 0;
const READING = 1;
const STOP = 2;
const STOPPED = 3;

const state = new Int32Array(new SharedArrayBuffer(4));
const worker = new Worker(
  `
  const { workerData } = require('node:worker_threads');
  const os = require('node:os');
  const state = new Int32Array(workerData);
  Atomics.store(state, 0, ${READING});
  Atomics.notify(state, 0);
  while (Atomics.load(state, 0) !== ${STOP}) os.homedir();
  Atomics.store(state, 0, ${STOPPED});
  Atomics.notify(state, 0);
`,
  { eval: true, workerData: state.buffer }
);

// Waits while the state is `value`. Returns false after 30 seconds.
const waitWhile = (value) => Atomics.wait(state, 0, value, 30_000) !== 'timed-out';

if (!waitWhile(STARTING)) {
  console.log('the worker did not start');
} else {
  for (let i = 0; i < 2000; i++) {
    process.env[`ELECTRON_SPEC_ENVIRON_RACE_${i}`] = '1';
  }
  Atomics.store(state, 0, STOP);
  console.log(waitWhile(STOP) ? 'done' : 'the worker did not stop');
}
worker.unref();
app.exit(0);

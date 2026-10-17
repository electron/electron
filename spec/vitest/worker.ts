// Hands this Electron main process over to vitest's worker runtime. Loaded by
// spec/index.js once the app is ready when Electron was started by the pool in
// spec/vitest/electron-pool.ts. This is the Electron counterpart of vitest's
// own `workers/forks` entry: same protocol, but over a JSON IPC channel (see
// ipc-serialization.ts) and with app.quit() as the teardown.

import { app } from 'electron/main';

import { init, runBaseTests, setupEnvironment } from 'vitest/worker';

import { deserialize, serialize, WORKER_READY_MESSAGE } from './ipc-serialization.ts';

if (!process.send) {
  throw new Error('spec/vitest/worker.ts expects to be launched by the vitest Electron pool (no IPC channel)');
}

// Keep references in case a spec replaces them.
const processSend = process.send.bind(process);
const processOn = process.on.bind(process);
const processOff = process.off.bind(process);
const processExit = process.exit.bind(process);

processOn('error', (error: NodeJS.ErrnoException) => {
  // The CLI went away; nothing left to report to.
  if (error?.code === 'ERR_IPC_CHANNEL_CLOSED' || error?.code === 'EPIPE') processExit(1);
});
processOn('disconnect', () => processExit(1));

// The main process runs with unhandled rejections in 'warn' mode and the mocha
// runner left them as warnings, so several specs fire and forget promises that
// reject once their window is gone. vitest would report every one of them as
// an error (it only does so when nobody else listens), so keep warning.
processOn('unhandledRejection', (reason) => {
  console.warn('UnhandledPromiseRejectionWarning:', reason);
});

init({
  post: (message) => processSend(message),
  on: (callback) => processOn('message', callback),
  off: (callback) => processOff('message', callback),
  serialize,
  deserialize,
  teardown: () => {
    process.removeAllListeners('message');
    setImmediate(() => app.quit());
  },
  runTests: (state, traces) => runBaseTests('run', state, traces),
  collectTests: (state, traces) => runBaseTests('collect', state, traces),
  setup: setupEnvironment
});

processSend(WORKER_READY_MESSAGE);

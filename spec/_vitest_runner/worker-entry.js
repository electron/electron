// This file is the Electron main-process entry for each vitest pool worker.
// It is launched via `spawn(electron, [<spec-v2 dir>])` with an IPC channel,
// so `process.send` is available and vitest's fork-worker protocol works.

const { app } = require('electron');

const path = require('node:path');

process.on('uncaughtException', (err) => {
  console.error('Unhandled exception in vitest worker:', err);
  process.exit(1);
});

// The pool allocates (mkdtemp) and cleans up this directory; the worker just
// points Electron at it before app ready.
const userDataDir = process.env.ELECTRON_VITEST_USER_DATA_DIR;
if (!userDataDir) {
  throw new Error('ELECTRON_VITEST_USER_DATA_DIR was not provided by the pool');
}
app.setPath('userData', userDataDir);

app.on('window-all-closed', () => null);

app
  .whenReady()
  .then(async () => {
    const distPath = process.env.VITEST_DIST_PATH;
    if (!distPath) {
      throw new Error('VITEST_DIST_PATH was not provided by the pool');
    }
    // Importing this registers the process.on('message') handler that speaks
    // vitest's pool protocol and actually runs the test files.
    await import(path.join(distPath, 'workers/forks.js'));
  })
  .catch((err) => {
    console.error('Failed to bootstrap vitest worker:', err);
    process.exit(1);
  });

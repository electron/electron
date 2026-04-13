// This file is the Electron main-process entry for each vitest pool worker.
// It is launched via `spawn(electron, [<spec-v2 dir>])` with an IPC channel,
// so `process.send` is available and vitest's fork-worker protocol works.

const { app, protocol } = require('electron');

const fs = require('node:fs');
const path = require('node:path');
const v8 = require('node:v8');

// Catch-and-exit only while bootstrapping. Once vitest's fork worker is
// loaded it patches process.exit (to throw) and installs its own handlers,
// so calling process.exit from here would re-throw inside an uncaughtException
// handler and exit the process with Node's code 7, hiding the real error.
function bootstrapUncaughtHandler(err) {
  console.error('Unhandled exception in vitest worker bootstrap:', err);
  process.exit(1);
}
process.on('uncaughtException', bootstrapUncaughtHandler);

process.env.ELECTRON_DISABLE_SECURITY_WARNINGS = 'true';

if (process.env.ELECTRON_TEST_DISABLE_HARDWARE_ACCELERATION) {
  app.disableHardwareAcceleration();
}

// The pool allocates (mkdtemp) and cleans up the parent directory; the worker
// adds app.name so tests that assert getPath('userData') contains the app name
// still hold.
const userDataBase = process.env.ELECTRON_VITEST_USER_DATA_DIR;
if (!userDataBase) {
  throw new Error('ELECTRON_VITEST_USER_DATA_DIR was not provided by the pool');
}
const userDataDir = path.join(userDataBase, app.name);
fs.mkdirSync(userDataDir, { recursive: true });
app.setPath('userData', userDataDir);

v8.setFlagsFromString('--expose_gc');
app.commandLine.appendSwitch('js-flags', '--expose_gc');
app.on('window-all-closed', () => null);

// Use fake device for Media Stream to replace actual camera and microphone.
app.commandLine.appendSwitch('use-fake-device-for-media-stream');
app.commandLine.appendSwitch(
  'host-resolver-rules',
  [
    'MAP localhost2 127.0.0.1',
    'MAP ipv4.localhost2 10.0.0.1',
    'MAP ipv6.localhost2 [::1]',
    'MAP notfound.localhost2 ~NOTFOUND'
  ].join(', ')
);

// Enable features required by tests.
app.commandLine.appendSwitch(
  'enable-features',
  [
    // spec/api-web-frame-main.spec.ts
    'DocumentPolicyIncludeJSCallStacksInCrashReports',
    // spec/spellchecker.spec.ts
    'UnrestrictSpellingAndGrammarForTesting'
  ].join(',')
);

global.standardScheme = 'app';
global.zoomScheme = 'zoom';
global.serviceWorkerScheme = 'sw';
protocol.registerSchemesAsPrivileged([
  { scheme: global.standardScheme, privileges: { standard: true, secure: true, stream: false } },
  { scheme: global.zoomScheme, privileges: { standard: true, secure: true } },
  { scheme: global.serviceWorkerScheme, privileges: { allowServiceWorkers: true, standard: true, secure: true } },
  { scheme: 'http-like', privileges: { standard: true, secure: true, corsEnabled: true, supportFetchAPI: true } },
  { scheme: 'cors-blob', privileges: { corsEnabled: true, supportFetchAPI: true } },
  { scheme: 'cors', privileges: { corsEnabled: true, supportFetchAPI: true } },
  { scheme: 'no-cors', privileges: { supportFetchAPI: true } },
  { scheme: 'no-fetch', privileges: { corsEnabled: true } },
  { scheme: 'stream', privileges: { standard: true, stream: true } },
  { scheme: 'foo', privileges: { standard: true } },
  { scheme: 'bar', privileges: { standard: true } }
]);

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
    // vitest's worker patches process.exit and owns error handling from here.
    process.removeListener('uncaughtException', bootstrapUncaughtHandler);
  })
  .catch((err) => {
    console.error('Failed to bootstrap vitest worker:', err);
    process.exit(1);
  });

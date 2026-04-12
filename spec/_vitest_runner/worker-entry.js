// This file is the Electron main-process entry for each vitest pool worker.
// It is launched via `spawn(electron, [<spec-v2 dir>])` with an IPC channel,
// so `process.send` is available and vitest's fork-worker protocol works.

const { app, protocol } = require('electron');

const path = require('node:path');
const v8 = require('node:v8');

process.on('uncaughtException', (err) => {
  console.error('Unhandled exception in vitest worker:', err);
  process.exit(1);
});

process.env.ELECTRON_DISABLE_SECURITY_WARNINGS = 'true';

if (process.env.ELECTRON_TEST_DISABLE_HARDWARE_ACCELERATION) {
  app.disableHardwareAcceleration();
}

// The pool allocates (mkdtemp) and cleans up this directory; the worker just
// points Electron at it before app ready.
const userDataDir = process.env.ELECTRON_VITEST_USER_DATA_DIR;
if (!userDataDir) {
  throw new Error('ELECTRON_VITEST_USER_DATA_DIR was not provided by the pool');
}
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
  })
  .catch((err) => {
    console.error('Failed to bootstrap vitest worker:', err);
    process.exit(1);
  });

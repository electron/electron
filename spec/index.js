import { app, protocol } from 'electron';

import * as v8 from 'node:v8';

// This app is started by the vitest Electron pool (spec/vitest/electron-pool.ts),
// several instances at a time, each running one spec file after another in
// the main process. Everything up to app 'ready' configures the process the
// specs expect; spec/vitest/worker.ts then takes over.
const isVitestWorker = process.argv.includes('--vitest-worker');
if (isVitestWorker) {
  // Workers run concurrently; give each its own profile directory so they do
  // not fight over cookie/localStorage/cache databases. Keep the app name in
  // the path, some specs check for it.
  app.setPath('userData', `${app.getPath('userData')}-worker-${process.env.ELECTRON_SPEC_WORKER_ID || process.pid}`);
}

// Uncaught exceptions before the worker runtime is up are fatal; afterwards
// vitest reports them against the running test (and its listener keeps
// Electron's error dialog away).
let workerStarted = false;
process.on('uncaughtException', (err) => {
  if (workerStarted) return;
  console.error('Unhandled exception in the spec worker:', err);
  process.exit(1);
});

process.env.ELECTRON_DISABLE_SECURITY_WARNINGS = 'true';

// Some Linux machines have broken hardware acceleration support.
if (process.env.ELECTRON_TEST_DISABLE_HARDWARE_ACCELERATION) {
  app.disableHardwareAcceleration();
}

v8.setFlagsFromString('--expose_gc');
app.commandLine.appendSwitch('js-flags', '--expose_gc');
// Prevent the spec runner quitting when the first window closes
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
    // spec/spellchecker.spec.ts - allows spellcheck without user gesture
    // https://chromium-review.googlesource.com/c/chromium/src/+/7452579
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
  { scheme: 'no-cors-file', privileges: { supportFetchAPI: true } },
  { scheme: 'no-cors-http', privileges: { supportFetchAPI: true } },
  { scheme: 'no-cors-standard', privileges: { standard: true, supportFetchAPI: true } },
  { scheme: 'no-fetch', privileges: { corsEnabled: true } },
  { scheme: 'stream', privileges: { standard: true, stream: true } },
  { scheme: 'foo', privileges: { standard: true } },
  { scheme: 'bar', privileges: { standard: true } }
]);

app
  .whenReady()
  .then(async () => {
    if (!isVitestWorker) {
      console.error(
        'This is the Electron spec app; it is started by the test runner. Run `npm test` ' +
          '(node script/spec-runner.js) from the repository root instead, optionally with ' +
          '--files spec/some.spec.ts or -g <pattern>.'
      );
      return process.exit(1);
    }
    const { isTestingBindingAvailable } = await import('./lib/spec-helpers.ts');
    if (process.env.ELECTRON_REQUIRE_TESTING_BINDINGS === '1' && !isTestingBindingAvailable()) {
      throw new Error('Testing build expected, but testing bindings are unavailable');
    }
    workerStarted = true;
    await import('./vitest/worker.ts');
  })
  .catch((err) => {
    console.error('An error occurred while starting the spec worker');
    console.error(err);
    process.exit(1);
  });

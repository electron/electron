const { app, protocol } = require('electron');

const childProcess = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');
const v8 = require('node:v8');

const FAILURE_STATUS_KEY = 'Electron_Spec_Runner_Failures';

// We want to terminate on errors, not throw up a dialog
process.on('uncaughtException', (err) => {
  console.error('Unhandled exception in main spec runner:', err);
  process.exit(1);
});

// Tell ts-node which tsconfig to use
process.env.TS_NODE_PROJECT = path.resolve(__dirname, '../tsconfig.spec.json');
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
    // spec/api-web-frame-main-spec.ts
    'DocumentPolicyIncludeJSCallStacksInCrashReports',
    // spec/spellchecker-spec.ts - allows spellcheck without user gesture
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
  { scheme: 'no-cors-standard', privileges: { standard: true, supportFetchAPI: true } },
  { scheme: 'no-fetch', privileges: { corsEnabled: true } },
  { scheme: 'stream', privileges: { standard: true, stream: true } },
  { scheme: 'foo', privileges: { standard: true } },
  { scheme: 'bar', privileges: { standard: true } }
]);

// Walk up the PPid chain in /proc to determine if `pid` is a descendant
// of the current process.  Used on Linux to avoid killing the current Electron
// instance's own helper processes (GPU, renderer, zygote, crashpad, etc.)
// which share the same executable path but are spawned by us.
function isDescendantOfCurrentProcess(pid) {
  let current = pid;
  while (current > 1) {
    try {
      const status = fs.readFileSync(`/proc/${current}/status`, 'utf8');
      const match = status.match(/^PPid:\s+(\d+)/m);
      if (!match) return false;
      const ppid = parseInt(match[1], 10);
      if (ppid === process.pid) return true;
      if (ppid === current) return false; // guard against cycles
      current = ppid;
    } catch {
      return false;
    }
  }
  return false;
}

async function killOrphanedElectronProcesses(suiteName) {
  let killed = 0;
  try {
    if (process.platform === 'win32') {
      // Enumerate by executable path, filter current PID, force-kill each survivor
      const escapedPath = process.execPath.replace(/\\/g, '\\\\');
      const result = childProcess.spawnSync(
        'wmic',
        ['process', 'where', `ExecutablePath='${escapedPath}'`, 'get', 'ProcessId', '/format:value'],
        { encoding: 'utf8' }
      );
      const pids = (result.stdout || '')
        .split(/\r?\n/)
        .map((line) => line.match(/^ProcessId=(\d+)/)?.[1])
        .filter(Boolean)
        .map(Number)
        .filter((pid) => pid !== process.pid);
      for (const pid of pids) {
        try {
          childProcess.spawnSync('taskkill', ['/F', '/PID', String(pid), '/T']);
          killed++;
        } catch {
          // process may have already exited
        }
      }
    } else {
      let pids;
      if (process.platform === 'linux') {
        // On Linux, pgrep -f matches the full command line, which would also
        // match wrapper scripts like `python3 dbus_mock.py /path/to/electron`.
        // Killing those wrappers hangs the test run.  Instead, scan /proc
        // directly and compare the /proc/<pid>/exe symlink (the real executable)
        // against process.execPath so we only find actual Electron processes.
        pids = [];
        try {
          for (const entry of fs.readdirSync('/proc')) {
            const pid = parseInt(entry, 10);
            if (isNaN(pid)) continue;
            try {
              if (fs.readlinkSync(`/proc/${pid}/exe`) === process.execPath) {
                pids.push(pid);
              }
            } catch {
              // no permission or process already exited
            }
          }
        } catch {
          // /proc unavailable — ignore
        }
      } else {
        // macOS: pgrep -f is safe here (no wrapper scripts, and helpers use
        // different .app bundles with different executable paths).
        const result = childProcess.spawnSync('pgrep', ['-f', process.execPath], { encoding: 'utf8' });
        pids = (result.stdout || '')
          .split('\n')
          .map((s) => parseInt(s, 10))
          .filter((pid) => !isNaN(pid));
      }

      for (const pid of pids.filter((pid) => pid !== process.pid)) {
        try {
          // On Linux, skip any process that is a descendant of the current
          // Electron instance (GPU, renderer, zygote, crashpad, etc.).
          if (process.platform === 'linux' && isDescendantOfCurrentProcess(pid)) continue;
          process.kill(pid, 'SIGKILL');
          killed++;
        } catch {
          // process may have already exited
        }
      }
    }
  } catch {
    // pgrep / wmic not available or returned an error — ignore
  }
  if (killed > 0) {
    console.log(`Killed ${killed} orphaned Electron process${killed === 1 ? '' : 'es'} before suite: ${suiteName}`);
  }
}

app
  .whenReady()
  .then(async () => {
    require('ts-node/register');

    const argv = require('yargs')
      .boolean('ci')
      .array('files')
      .string('g')
      .alias('g', 'grep')
      .boolean('i')
      .alias('i', 'invert').argv;

    const Mocha = require('mocha');
    const mochaOptions = {
      forbidOnly: process.env.CI
    };
    if (process.env.CI) {
      mochaOptions.retries = 3;
    }
    if (process.env.MOCHA_REPORTER) {
      mochaOptions.reporter = process.env.MOCHA_REPORTER;
    }
    if (process.env.MOCHA_MULTI_REPORTERS) {
      mochaOptions.reporterOptions = {
        reporterEnabled: process.env.MOCHA_MULTI_REPORTERS
      };
    }
    // The MOCHA_GREP and MOCHA_INVERT are used in some vendor builds for sharding
    // tests.
    if (process.env.MOCHA_GREP) {
      mochaOptions.grep = process.env.MOCHA_GREP;
    }
    if (process.env.MOCHA_INVERT) {
      mochaOptions.invert = process.env.MOCHA_INVERT === 'true';
    }
    const mocha = new Mocha(mochaOptions);

    // Add a root hook on mocha to skip any tests that are disabled
    const disabledTests = new Set(JSON.parse(fs.readFileSync(path.join(__dirname, 'disabled-tests.json'), 'utf8')));
    mocha.suite.beforeEach(function () {
      // TODO(clavin): add support for disabling *suites* by title, not just tests
      if (disabledTests.has(this.currentTest?.fullTitle())) {
        this.skip();
      }
    });

    // The cleanup method is registered this way rather than through an
    // `afterEach` at the top level so that it can run before other `afterEach`
    // methods.
    //
    // The order of events is:
    // 1. test completes,
    // 2. `defer()`-ed methods run, in reverse order,
    // 3. regular `afterEach` hooks run.
    const { runCleanupFunctions } = require('./lib/spec-helpers');
    mocha.suite.on('suite', function attach(suite) {
      suite.afterEach('cleanup', runCleanupFunctions);
      suite.on('suite', attach);
    });

    // Kill any Electron processes left over from the previous spec file before
    // starting the next one.  The listener is intentionally non-recursive so it
    // only fires for top-level suites (one per file), not nested describes.
    mocha.suite.on('suite', (suite) => {
      console.log(`Adding kill orphaned process for test suite: ${suite.title}`);
      suite.beforeAll('kill orphaned electron processes', () => killOrphanedElectronProcesses(suite.title));
    });

    if (!process.env.MOCHA_REPORTER) {
      mocha.ui('bdd').reporter('tap');
    }
    const mochaTimeout = process.env.MOCHA_TIMEOUT || 30000;
    mocha.timeout(mochaTimeout);

    if (argv.grep) mocha.grep(argv.grep);
    if (argv.invert) mocha.invert();

    const baseElectronDir = path.resolve(__dirname, '..');
    const validTestPaths =
      argv.files &&
      argv.files.map((file) => (path.isAbsolute(file) ? path.relative(baseElectronDir, file) : path.normalize(file)));
    const filter = (file) => {
      if (!/-spec\.[tj]s$/.test(file)) {
        return false;
      }

      // This allows you to run specific modules only:
      // npm run test -match=menu
      const moduleMatch = process.env.npm_config_match ? new RegExp(process.env.npm_config_match, 'g') : null;
      if (moduleMatch && !moduleMatch.test(file)) {
        return false;
      }

      if (validTestPaths && !validTestPaths.includes(path.relative(baseElectronDir, file))) {
        return false;
      }

      return true;
    };

    const { getFiles } = require('./get-files');
    const testFiles = await getFiles(__dirname, filter);
    for (const file of testFiles.sort()) {
      mocha.addFile(file);
    }

    if (validTestPaths && validTestPaths.length > 0 && testFiles.length === 0) {
      console.error('Test files were provided, but they did not match any searched files');
      console.error('provided file paths (relative to electron/):', validTestPaths);
      process.exit(1);
    }

    const cb = () => {
      // Ensure the callback is called after runner is defined
      process.nextTick(() => {
        if (process.env.ELECTRON_FORCE_TEST_SUITE_EXIT === 'true') {
          console.log(`${FAILURE_STATUS_KEY}: ${runner.failures}`);
          process.kill(process.pid);
        } else {
          process.exit(runner.failures);
        }
      });
    };

    // Set up chai in the correct order
    const chai = require('chai');
    chai.use(require('chai-as-promised'));
    chai.use(require('dirty-chai'));

    // Show full object diff
    // https://github.com/chaijs/chai/issues/469
    chai.config.truncateThreshold = 0;

    const runner = mocha.run(cb);

    const RETRY_EVENT = Mocha?.Runner?.constants?.EVENT_TEST_RETRY || 'retry';

    runner.on(RETRY_EVENT, (test, err) => {
      console.log(`Failure in test: "${test.fullTitle()}"`);
      if (err?.stack) console.log(err.stack.split('\n').slice(0, 3).join('\n'));
      console.log(`Retrying test (${test.currentRetry() + 1}/${test.retries()})...`);
    });
  })
  .catch((err) => {
    console.error('An error occurred while running the spec runner');
    console.error(err);
    process.exit(1);
  });

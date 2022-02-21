const path = require('path');
const v8 = require('v8');

module.paths.push(path.resolve(__dirname, '../spec/node_modules'));

// Extra module paths which can be used to load Mocha reporters
if (process.env.ELECTRON_TEST_EXTRA_MODULE_PATHS) {
  for (const modulePath of process.env.ELECTRON_TEST_EXTRA_MODULE_PATHS.split(':')) {
    module.paths.push(modulePath);
  }
}

// Add search paths for loaded spec files
require('../spec/global-paths')(module.paths);

// We want to terminate on errors, not throw up a dialog
process.on('uncaughtException', (err) => {
  console.error('Unhandled exception in main spec runner:', err);
  process.exit(1);
});

// Tell ts-node which tsconfig to use
process.env.TS_NODE_PROJECT = path.resolve(__dirname, '../tsconfig.spec.json');
process.env.ELECTRON_DISABLE_SECURITY_WARNINGS = 'true';

const { app, protocol } = require('electron');

v8.setFlagsFromString('--expose_gc');
app.commandLine.appendSwitch('js-flags', '--expose_gc');
// Prevent the spec runner quitting when the first window closes
app.on('window-all-closed', () => null);

// Use fake device for Media Stream to replace actual camera and microphone.
app.commandLine.appendSwitch('use-fake-device-for-media-stream');
app.commandLine.appendSwitch('host-rules', 'MAP localhost2 127.0.0.1');

global.standardScheme = 'app';
global.zoomScheme = 'zoom';
global.serviceWorkerScheme = 'sw';
protocol.registerSchemesAsPrivileged([
  { scheme: global.standardScheme, privileges: { standard: true, secure: true, stream: false } },
  { scheme: global.zoomScheme, privileges: { standard: true, secure: true } },
  { scheme: global.serviceWorkerScheme, privileges: { allowServiceWorkers: true, standard: true, secure: true } },
  { scheme: 'cors-blob', privileges: { corsEnabled: true, supportFetchAPI: true } },
  { scheme: 'cors', privileges: { corsEnabled: true, supportFetchAPI: true } },
  { scheme: 'no-cors', privileges: { supportFetchAPI: true } },
  { scheme: 'no-fetch', privileges: { corsEnabled: true } },
  { scheme: 'stream', privileges: { standard: true, stream: true } },
  { scheme: 'foo', privileges: { standard: true } },
  { scheme: 'bar', privileges: { standard: true } }
]);

app.whenReady().then(async () => {
  require('ts-node/register');

  const argv = require('yargs')
    .boolean('ci')
    .array('files')
    .string('g').alias('g', 'grep')
    .boolean('i').alias('i', 'invert')
    .argv;

  const Mocha = require('mocha');
  const mochaOptions = {};
  if (process.env.MOCHA_REPORTER) {
    mochaOptions.reporter = process.env.MOCHA_REPORTER;
  }
  if (process.env.MOCHA_MULTI_REPORTERS) {
    mochaOptions.reporterOptions = {
      reporterEnabled: process.env.MOCHA_MULTI_REPORTERS
    };
  }
  const mocha = new Mocha(mochaOptions);

  // The cleanup method is registered this way rather than through an
  // `afterEach` at the top level so that it can run before other `afterEach`
  // methods.
  //
  // The order of events is:
  // 1. test completes,
  // 2. `defer()`-ed methods run, in reverse order,
  // 3. regular `afterEach` hooks run.
  const { runCleanupFunctions } = require('./spec-helpers');
  mocha.suite.on('suite', function attach (suite) {
    suite.afterEach('cleanup', runCleanupFunctions);
    suite.on('suite', attach);
  });

  if (!process.env.MOCHA_REPORTER) {
    mocha.ui('bdd').reporter('tap');
  }
  const mochaTimeout = process.env.MOCHA_TIMEOUT || 30000;
  mocha.timeout(mochaTimeout);

  if (argv.grep) mocha.grep(argv.grep);
  if (argv.invert) mocha.invert();

  const filter = (file) => {
    if (!/-spec\.[tj]s$/.test(file)) {
      return false;
    }

    // This allows you to run specific modules only:
    // npm run test -match=menu
    const moduleMatch = process.env.npm_config_match
      ? new RegExp(process.env.npm_config_match, 'g')
      : null;
    if (moduleMatch && !moduleMatch.test(file)) {
      return false;
    }

    const baseElectronDir = path.resolve(__dirname, '..');
    if (argv.files && !argv.files.includes(path.relative(baseElectronDir, file))) {
      return false;
    }

    return true;
  };

  const getFiles = require('../spec/static/get-files');
  const testFiles = await getFiles(__dirname, { filter });
  testFiles.sort().forEach((file) => {
    mocha.addFile(file);
  });

  const cb = () => {
    // Ensure the callback is called after runner is defined
    process.nextTick(() => {
      process.exit(runner.failures);
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
}).catch((err) => {
  console.error('An error occurred while running the spec-main spec runner');
  console.error(err);
  process.exit(1);
});

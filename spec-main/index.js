const Module = require('module');
const path = require('path');
const v8 = require('v8');

Module.globalPaths.push(path.resolve(__dirname, '../spec/node_modules'));

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
// Prevent the spec runner quiting when the first window closes
app.on('window-all-closed', () => null);

// Use fake device for Media Stream to replace actual camera and microphone.
app.commandLine.appendSwitch('use-fake-device-for-media-stream');

global.standardScheme = 'app';
global.zoomScheme = 'zoom';
protocol.registerSchemesAsPrivileged([
  { scheme: global.standardScheme, privileges: { standard: true, secure: true } },
  { scheme: global.zoomScheme, privileges: { standard: true, secure: true } },
  { scheme: 'cors-blob', privileges: { corsEnabled: true, supportFetchAPI: true } },
  { scheme: 'cors', privileges: { corsEnabled: true, supportFetchAPI: true } },
  { scheme: 'no-cors', privileges: { supportFetchAPI: true } },
  { scheme: 'no-fetch', privileges: { corsEnabled: true } },
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

  const runner = mocha.run(cb);
});

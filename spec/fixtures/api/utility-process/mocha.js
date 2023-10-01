const path = require('node:path');
const v8 = require('node:v8');

require('ts-node/register');

v8.setFlagsFromString('--expose_gc');

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
const mocha = new Mocha(mochaOptions);

const { runCleanupFunctions } = require('../../../lib/spec-helpers');
mocha.suite.on('suite', function attach (suite) {
  suite.afterEach('cleanup', runCleanupFunctions);
  suite.on('suite', attach);
});

if (!process.env.MOCHA_REPORTER) {
  mocha.ui('bdd').reporter('tap');
}

const mochaTimeout = process.env.MOCHA_TIMEOUT || 30000;
mocha.timeout(mochaTimeout);

const baseElectronDir = path.resolve(__dirname, '../../../..');

process.parentPort.on('message', (e) => {
  console.log('child received', e);
  console.log('path', path.join(baseElectronDir, e.data));

  mocha.addFile(path.join(baseElectronDir, e.data));

  // Set up chai in the correct order
  const chai = require('chai');
  chai.use(require('chai-as-promised'));
  chai.use(require('dirty-chai'));

  // Show full object diff
  // https://github.com/chaijs/chai/issues/469
  chai.config.truncateThreshold = 0;

  mocha.run((failures) => {
    // Ensure the callback is called after runner is defined
    process.nextTick(() => {
      process.exit(failures);
    });
  });
});

const cp = require('child_process');
const fs = require('fs');
const path = require('path');

const BASE = path.resolve(__dirname, '../..');
const NAN_DIR = path.resolve(BASE, 'third_party', 'nan');
const NPX_CMD = process.platform === 'win32' ? 'npx.cmd' : 'npx';

const utils = require('./lib/utils');
const { YARN_VERSION } = require('./yarn');

if (!process.mainModule) {
  throw new Error('Must call the nan spec runner directly');
}

async function main () {
  const nodeDir = path.resolve(BASE, `out/${utils.getOutDir({ shouldLog: true })}/gen/node_headers`);
  const env = Object.assign({}, process.env, {
    npm_config_nodedir: nodeDir,
    npm_config_msvs_version: '2019',
    npm_config_arch: process.env.NPM_CONFIG_ARCH
  });
  const { status: buildStatus } = cp.spawnSync(NPX_CMD, ['node-gyp', 'rebuild', '--directory', 'test'], {
    env,
    cwd: NAN_DIR,
    stdio: 'inherit'
  });
  if (buildStatus !== 0) {
    console.error('Failed to build nan test modules');
    return process.exit(buildStatus);
  }

  const { status: installStatus } = cp.spawnSync(NPX_CMD, [`yarn@${YARN_VERSION}`, 'install'], {
    env,
    cwd: NAN_DIR,
    stdio: 'inherit'
  });
  if (installStatus !== 0) {
    console.error('Failed to install nan node_modules');
    return process.exit(installStatus);
  }

  const DISABLED_TESTS = ['nannew-test.js'];
  const testsToRun = fs.readdirSync(path.resolve(NAN_DIR, 'test', 'js'))
    .filter(test => !DISABLED_TESTS.includes(test))
    .map(test => `test/js/${test}`);

  const testChild = cp.spawn(utils.getAbsoluteElectronExec(), ['node_modules/.bin/tap', ...testsToRun], {
    env: {
      ...process.env,
      ELECTRON_RUN_AS_NODE: 'true'
    },
    cwd: NAN_DIR,
    stdio: 'inherit'
  });
  testChild.on('exit', (testCode) => {
    process.exit(testCode);
  });
}

main().catch((err) => {
  console.error('An unhandled error occurred in the nan spec runner', err);
  process.exit(1);
});

const cp = require('child_process');
const fs = require('fs');
const path = require('path');

const args = require('minimist')(process.argv.slice(2), {
  boolean: ['default', 'validateDisabled'],
  string: ['jUnitDir']
});

const BASE = path.resolve(__dirname, '../..');
const DISABLED_TESTS = require('./node-disabled-tests.json');
const NODE_DIR = path.resolve(BASE, 'third_party', 'electron_node');
const JUNIT_DIR = args.jUnitDir ? path.resolve(args.jUnitDir) : null;
const TAP_FILE_NAME = 'test.tap';

const utils = require('./lib/utils');

if (!process.mainModule) {
  throw new Error('Must call the node spec runner directly');
}

const defaultOptions = [
  'tools/test.py',
  '-p',
  'tap',
  '--logfile',
  TAP_FILE_NAME,
  '--mode=debug',
  'default',
  `--skip-tests=${DISABLED_TESTS.join(',')}`,
  '--flaky-tests=dontcare',
  '--shell',
  utils.getAbsoluteElectronExec(),
  '-J'
];

const getCustomOptions = () => {
  let customOptions = ['tools/test.py'];

  // Add all custom arguments.
  const extra = process.argv.slice(2);
  if (extra) {
    customOptions = customOptions.concat(extra);
  }

  // Necessary or Node.js will try to run from out/Release/node.
  customOptions = customOptions.concat([
    '--shell',
    utils.getAbsoluteElectronExec()
  ]);

  return customOptions;
};

async function main () {
  // Optionally validate that all disabled specs still exist.
  if (args.validateDisabled) {
    const missing = [];
    for (const test of DISABLED_TESTS) {
      const testName = test.endsWith('.js') ? test : `${test}.js`;
      if (!fs.existsSync(path.join(NODE_DIR, 'test', testName))) {
        missing.push(test);
      }
    }

    if (missing.length > 0) {
      console.error(`Found ${missing.length} missing disabled specs: \n${missing.join('\n')}`);
      process.exit(1);
    }
  }

  const options = args.default ? defaultOptions : getCustomOptions();

  const testChild = cp.spawn('python3', options, {
    env: {
      ...process.env,
      ELECTRON_RUN_AS_NODE: 'true',
      ELECTRON_EAGER_ASAR_HOOK_FOR_TESTING: 'true'
    },
    cwd: NODE_DIR,
    stdio: 'inherit'
  });
  testChild.on('exit', (testCode) => {
    if (JUNIT_DIR) {
      fs.mkdirSync(JUNIT_DIR);
      const converterStream = require('tap-xunit')();
      fs.createReadStream(
        path.resolve(NODE_DIR, TAP_FILE_NAME)
      ).pipe(converterStream).pipe(
        fs.createWriteStream(path.resolve(JUNIT_DIR, 'nodejs.xml'))
      ).on('close', () => {
        process.exit(testCode);
      });
    }
  });
}

main().catch((err) => {
  console.error('An unhandled error occurred in the node spec runner', err);
  process.exit(1);
});

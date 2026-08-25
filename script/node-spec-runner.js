const minimist = require('minimist');

const cp = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');

const utils = require('./lib/utils');
const DISABLED_TESTS = require('./node-disabled-tests.json');

const args = minimist(process.argv.slice(2), {
  boolean: ['default', 'validateDisabled'],
  string: ['jUnitDir']
});

const BASE = path.resolve(__dirname, '../..');

const ROOT_PACKAGE_JSON = path.resolve(BASE, 'package.json');
const NODE_DIR = path.resolve(BASE, 'third_party', 'electron_node');
const JUNIT_DIR = args.jUnitDir ? path.resolve(args.jUnitDir) : null;
const TAP_FILE_NAME = 'test.tap';

if (!require.main) {
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
  '--measure-flakiness=9',
  '--shell',
  utils.getAbsoluteElectronExec(),
  '-J'
];

// The upstream Node.js test suite assumes there is no package.json above the
// test tree. In Electron, third_party/electron_node lives under Chromium's
// src/, whose package.json ("type": "module") is always an ancestor. That
// changes how Node resolves the module type of test files and fixtures: it
// disables module-syntax detection (breaking e.g.
// test-compile-cache-typescript-esm) and emits MODULE_TYPELESS_PACKAGE_JSON
// warnings that break tests asserting clean stderr (e.g. test-esm-detect-
// ambiguous, test-esm-import-meta-main-eval, test-output-coverage-with-mock).
//
// While the suite runs we move src/package.json aside so the environment
// matches upstream exactly, then restore it when done. The original contents
// are kept in a sibling backup file so an interrupted/killed run self-heals on
// the next invocation rather than leaving src/package.json missing.
const ROOT_PACKAGE_JSON_BACKUP = `${ROOT_PACKAGE_JSON}.spec-runner-backup`;

const stashPackageJson = () => {
  // This won't always exist in CI.
  if (!fs.existsSync(ROOT_PACKAGE_JSON)) {
    return;
  }
  fs.copyFileSync(ROOT_PACKAGE_JSON, ROOT_PACKAGE_JSON_BACKUP);
  fs.rmSync(ROOT_PACKAGE_JSON);
};

const restorePackageJson = () => {
  if (!fs.existsSync(ROOT_PACKAGE_JSON_BACKUP)) {
    return;
  }
  fs.copyFileSync(ROOT_PACKAGE_JSON_BACKUP, ROOT_PACKAGE_JSON);
  fs.rmSync(ROOT_PACKAGE_JSON_BACKUP);
};

const getCustomOptions = () => {
  let customOptions = ['tools/test.py'];

  // Add all custom arguments.
  const extra = process.argv.slice(2);
  if (extra) {
    customOptions = customOptions.concat(extra);
  }

  // Necessary or Node.js will try to run from out/Release/node.
  customOptions = customOptions.concat(['--shell', utils.getAbsoluteElectronExec()]);

  return customOptions;
};

async function main() {
  // Optionally validate that all disabled specs still exist.
  if (args.validateDisabled) {
    const missing = [];
    for (const test of DISABLED_TESTS) {
      const js = path.join(NODE_DIR, 'test', `${test}.js`);
      const mjs = path.join(NODE_DIR, 'test', `${test}.mjs`);
      if (!fs.existsSync(js) && !fs.existsSync(mjs)) {
        missing.push(test);
      }
    }

    if (missing.length > 0) {
      console.error(`Found ${missing.length} missing disabled specs: \n${missing.join('\n')}`);
      process.exit(1);
    }

    console.log(`All ${DISABLED_TESTS.length} disabled specs exist.`);
    process.exit(0);
  }

  const options = args.default ? defaultOptions : getCustomOptions();

  // Recover src/package.json if a previous run was interrupted, then move it
  // aside for the duration of this run.
  restorePackageJson();
  stashPackageJson();

  // Make sure src/package.json is put back even if we exit abnormally.
  process.on('exit', restorePackageJson);
  process.on('SIGINT', () => process.exit(130));
  process.on('SIGTERM', () => process.exit(143));

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
    restorePackageJson();

    if (JUNIT_DIR) {
      fs.mkdirSync(JUNIT_DIR);
      const converterStream = require('tap-xunit')();
      fs.createReadStream(path.resolve(NODE_DIR, TAP_FILE_NAME))
        .pipe(converterStream)
        .pipe(fs.createWriteStream(path.resolve(JUNIT_DIR, 'nodejs.xml')))
        .on('close', () => {
          process.exit(testCode);
        });
    }
  });
}

main().catch((err) => {
  console.error('An unhandled error occurred in the node spec runner', err);
  process.exit(1);
});

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
const OUT_PACKAGE_JSON = path.resolve(path.dirname(utils.getAbsoluteElectronExec()), 'package.json');
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

// The upstream Node.js test suite assumes there is no package.json above its
// test files or virtual files rooted at process.execPath. Electron has
// "type": "module" package files at both Chromium's src/ root and in the
// selected output directory. They change how Node resolves the module type of
// test files and fixtures: they disable module-syntax detection (breaking e.g.
// test-compile-cache-typescript-esm) and emit MODULE_TYPELESS_PACKAGE_JSON
// warnings that break tests asserting clean stderr (e.g. test-esm-detect-
// ambiguous, test-esm-import-meta-main-eval, test-output-coverage-with-mock),
// and make virtual CommonJS files under process.execPath load as ESM.
//
// While the suite runs we move both package files aside so the environment
// matches upstream exactly, then restore them when done. Original contents are
// kept in sibling backup files so an interrupted run self-heals next time.
const PACKAGE_JSON_PATHS = [ROOT_PACKAGE_JSON, OUT_PACKAGE_JSON];

const stashPackageJson = () => {
  for (const packageJson of PACKAGE_JSON_PATHS) {
    // These won't always exist in CI.
    if (!fs.existsSync(packageJson)) {
      continue;
    }
    fs.copyFileSync(packageJson, `${packageJson}.spec-runner-backup`);
    fs.rmSync(packageJson);
  }
};

const restorePackageJson = () => {
  for (const packageJson of PACKAGE_JSON_PATHS) {
    const backup = `${packageJson}.spec-runner-backup`;
    if (!fs.existsSync(backup)) {
      continue;
    }
    fs.copyFileSync(backup, packageJson);
    fs.rmSync(backup);
  }
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

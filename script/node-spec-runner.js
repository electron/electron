const minimist = require('minimist');

const cp = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');

const utils = require('./lib/utils');
const DISABLED_TESTS = require('./node-disabled-tests.json');
const FLAKY_TESTS = require('./node-flaky-tests.json');

const args = minimist(process.argv.slice(2), {
  boolean: ['default', 'validateDisabled'],
  string: ['jUnitDir']
});

const BASE = path.resolve(__dirname, '../..');

const ROOT_PACKAGE_JSON = path.resolve(BASE, 'package.json');
const OUT_PACKAGE_JSON = path.resolve(path.dirname(utils.getAbsoluteElectronExec()), 'package.json');
const NODE_DIR = path.resolve(BASE, 'third_party', 'electron_node');
const ROOT_STATUS = path.resolve(NODE_DIR, 'test', 'root.status');
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

// Files outside Electron that are rewritten for the duration of a run and put
// back afterwards. Originals are kept in sibling backup files so an interrupted
// run self-heals next time.
//
// The upstream suite assumes there is no package.json above its test files or
// rooted at process.execPath; Chromium's src/ root and the output directory
// both have a "type": "module" one, which changes module-type detection for
// test files and fixtures and emits MODULE_TYPELESS_PACKAGE_JSON warnings that
// break tests asserting clean stderr. They are removed while the suite runs.
//
// Tests that flake under Electron but should keep running are appended to the
// suite's root.status as PASS,FLAKY, which --flaky-tests=dontcare then reports
// without failing the run.
const REWRITES = [
  { file: ROOT_PACKAGE_JSON, rewrite: () => null },
  { file: OUT_PACKAGE_JSON, rewrite: () => null },
  {
    file: ROOT_STATUS,
    rewrite: (original) => `${original}\n[true]\n${FLAKY_TESTS.map((test) => `${test}: PASS,FLAKY`).join('\n')}\n`
  }
];

const backupPath = (file) => `${file}.spec-runner-backup`;

const applyRewrites = () => {
  for (const { file, rewrite } of REWRITES) {
    // The package.json files won't always exist in CI.
    if (!fs.existsSync(file)) {
      continue;
    }
    fs.copyFileSync(file, backupPath(file));
    const rewritten = rewrite(fs.readFileSync(file, 'utf8'));
    if (rewritten === null) {
      fs.rmSync(file);
    } else {
      fs.writeFileSync(file, rewritten);
    }
  }
};

const restoreRewrites = () => {
  for (const { file } of REWRITES) {
    if (!fs.existsSync(backupPath(file))) {
      continue;
    }
    fs.copyFileSync(backupPath(file), file);
    fs.rmSync(backupPath(file));
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
    for (const test of [...DISABLED_TESTS, ...FLAKY_TESTS]) {
      const js = path.join(NODE_DIR, 'test', `${test}.js`);
      const mjs = path.join(NODE_DIR, 'test', `${test}.mjs`);
      if (!fs.existsSync(js) && !fs.existsSync(mjs)) {
        missing.push(test);
      }
    }

    if (missing.length > 0) {
      console.error(`Found ${missing.length} missing disabled/flaky specs: \n${missing.join('\n')}`);
      process.exit(1);
    }

    console.log(`All ${DISABLED_TESTS.length} disabled and ${FLAKY_TESTS.length} flaky specs exist.`);
    process.exit(0);
  }

  const options = args.default ? defaultOptions : getCustomOptions();

  // Undo a previous interrupted run first, then rewrite for this one and make
  // sure everything is put back even if we exit abnormally.
  restoreRewrites();
  applyRewrites();
  process.on('exit', restoreRewrites);
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
    restoreRewrites();

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

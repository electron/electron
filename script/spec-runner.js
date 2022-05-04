#!/usr/bin/env node

const childProcess = require('child_process');
const crypto = require('crypto');
const fs = require('fs-extra');
const { hashElement } = require('folder-hash');
const path = require('path');
const unknownFlags = [];

require('colors');
const pass = '✓'.green;
const fail = '✗'.red;

const args = require('minimist')(process.argv, {
  string: ['runners', 'target'],
  boolean: ['buildNativeTests', 'runTestFilesSeperately'],
  unknown: arg => unknownFlags.push(arg)
});

const unknownArgs = [];
for (const flag of unknownFlags) {
  unknownArgs.push(flag);
  const onlyFlag = flag.replace(/^-+/, '');
  if (args[onlyFlag]) {
    unknownArgs.push(args[onlyFlag]);
  }
}

const utils = require('./lib/utils');
const { YARN_VERSION } = require('./yarn');

const BASE = path.resolve(__dirname, '../..');
const NPX_CMD = process.platform === 'win32' ? 'npx.cmd' : 'npx';

const runners = new Map([
  ['main', { description: 'Main process specs', run: runMainProcessElectronTests }],
  ['remote', { description: 'Remote based specs', run: runRemoteBasedElectronTests }],
  ['native', { description: 'Native specs', run: runNativeElectronTests }]
]);

const specHashPath = path.resolve(__dirname, '../spec/.hash');

let runnersToRun = null;
if (args.runners !== undefined) {
  runnersToRun = args.runners.split(',').filter(value => value);
  if (!runnersToRun.every(r => [...runners.keys()].includes(r))) {
    console.log(`${fail} ${runnersToRun} must be a subset of [${[...runners.keys()].join(' | ')}]`);
    process.exit(1);
  }
  console.log('Only running:', runnersToRun);
} else {
  console.log(`Triggering runners: ${[...runners.keys()].join(', ')}`);
}

async function main () {
  const [lastSpecHash, lastSpecInstallHash] = loadLastSpecHash();
  const [currentSpecHash, currentSpecInstallHash] = await getSpecHash();
  const somethingChanged = (currentSpecHash !== lastSpecHash) ||
      (lastSpecInstallHash !== currentSpecInstallHash);

  if (somethingChanged) {
    await installSpecModules(path.resolve(__dirname, '..', 'spec'));
    await installSpecModules(path.resolve(__dirname, '..', 'spec-main'));
    await getSpecHash().then(saveSpecHash);
  }

  if (!fs.existsSync(path.resolve(__dirname, '../electron.d.ts'))) {
    console.log('Generating electron.d.ts as it is missing');
    generateTypeDefinitions();
  }

  await runElectronTests();
}

function generateTypeDefinitions () {
  const { status } = childProcess.spawnSync('npm', ['run', 'create-typescript-definitions'], {
    cwd: path.resolve(__dirname, '..'),
    stdio: 'inherit',
    shell: true
  });
  if (status !== 0) {
    throw new Error(`Electron typescript definition generation failed with exit code: ${status}.`);
  }
}

function loadLastSpecHash () {
  return fs.existsSync(specHashPath)
    ? fs.readFileSync(specHashPath, 'utf8').split('\n')
    : [null, null];
}

function saveSpecHash ([newSpecHash, newSpecInstallHash]) {
  fs.writeFileSync(specHashPath, `${newSpecHash}\n${newSpecInstallHash}`);
}

async function runElectronTests () {
  const errors = [];

  const testResultsDir = process.env.ELECTRON_TEST_RESULTS_DIR;
  for (const [runnerId, { description, run }] of runners) {
    if (runnersToRun && !runnersToRun.includes(runnerId)) {
      console.info('\nSkipping:', description);
      continue;
    }
    try {
      console.info('\nRunning:', description);
      if (testResultsDir) {
        process.env.MOCHA_FILE = path.join(testResultsDir, `test-results-${runnerId}.xml`);
      }
      await run();
    } catch (err) {
      errors.push([runnerId, err]);
    }
  }

  if (errors.length !== 0) {
    for (const err of errors) {
      console.error('\n\nRunner Failed:', err[0]);
      console.error(err[1]);
    }
    console.log(`${fail} Electron test runners have failed`);
    process.exit(1);
  }
}

async function runTestUsingElectron (specDir, testName) {
  let exe = path.resolve(BASE, utils.getElectronExec());
  const runnerArgs = [`electron/${specDir}`, ...unknownArgs.slice(2)];
  if (process.platform === 'linux') {
    runnerArgs.unshift(path.resolve(__dirname, 'dbus_mock.py'), exe);
    exe = 'python3';
  }
  const { status, signal } = childProcess.spawnSync(exe, runnerArgs, {
    cwd: path.resolve(__dirname, '../..'),
    stdio: 'inherit'
  });
  if (status !== 0) {
    if (status) {
      const textStatus = process.platform === 'win32' ? `0x${status.toString(16)}` : status.toString();
      console.log(`${fail} Electron tests failed with code ${textStatus}.`);
    } else {
      console.log(`${fail} Electron tests failed with kill signal ${signal}.`);
    }
    process.exit(1);
  }
  console.log(`${pass} Electron ${testName} process tests passed.`);
}

const specFilter = (file) => {
  if (!/-spec\.[tj]s$/.test(file)) {
    return false;
  } else {
    return true;
  }
};

async function runTests (specDir, testName) {
  if (args.runTestFilesSeperately) {
    const getFiles = require('../spec/static/get-files');
    const testFiles = await getFiles(path.resolve(__dirname, `../${specDir}`), { filter: specFilter });
    const baseElectronDir = path.resolve(__dirname, '..');
    unknownArgs.splice(unknownArgs.length, 0, '--files', '');
    testFiles.sort().forEach(async (file) => {
      unknownArgs.splice((unknownArgs.length - 1), 1, path.relative(baseElectronDir, file));
      console.log(`Running tests for ${unknownArgs[unknownArgs.length - 1]}`);
      await runTestUsingElectron(specDir, testName);
    });
  } else {
    await runTestUsingElectron(specDir, testName);
  }
}

async function runRemoteBasedElectronTests () {
  await runTests('spec', 'remote');
}

async function runNativeElectronTests () {
  let testTargets = require('./native-test-targets.json');
  const outDir = `out/${utils.getOutDir()}`;

  // If native tests are being run, only one arg would be relevant
  if (args.target && !testTargets.includes(args.target)) {
    console.log(`${fail} ${args.target} must be a subset of [${[testTargets].join(', ')}]`);
    process.exit(1);
  }

  // Optionally build all native test targets
  if (args.buildNativeTests) {
    for (const target of testTargets) {
      const build = childProcess.spawnSync('ninja', ['-C', outDir, target], {
        cwd: path.resolve(__dirname, '../..'),
        stdio: 'inherit'
      });

      // Exit if test target failed to build
      if (build.status !== 0) {
        console.log(`${fail} ${target} failed to build.`);
        process.exit(1);
      }
    }
  }

  // If a specific target was passed, only build and run that target
  if (args.target) testTargets = [args.target];

  // Run test targets
  const failures = [];
  for (const target of testTargets) {
    console.info('\nRunning native test for target:', target);
    const testRun = childProcess.spawnSync(`./${outDir}/${target}`, {
      cwd: path.resolve(__dirname, '../..'),
      stdio: 'inherit'
    });

    // Collect failures and log at end
    if (testRun.status !== 0) failures.push({ target });
  }

  // Exit if any failures
  if (failures.length > 0) {
    console.log(`${fail} Electron native tests failed for the following targets: `, failures);
    process.exit(1);
  }

  console.log(`${pass} Electron native tests passed.`);
}

async function runMainProcessElectronTests () {
  await runTests('spec-main', 'main');
}

async function installSpecModules (dir) {
  const nodeDir = path.resolve(BASE, `out/${utils.getOutDir({ shouldLog: true })}/gen/node_headers`);
  const env = Object.assign({}, process.env, {
    npm_config_nodedir: nodeDir,
    npm_config_msvs_version: '2019',
    npm_config_yes: 'true'
  });
  if (fs.existsSync(path.resolve(dir, 'node_modules'))) {
    await fs.remove(path.resolve(dir, 'node_modules'));
  }
  const { status } = childProcess.spawnSync(NPX_CMD, [`yarn@${YARN_VERSION}`, 'install', '--frozen-lockfile'], {
    env,
    cwd: dir,
    stdio: 'inherit'
  });
  if (status !== 0 && !process.env.IGNORE_YARN_INSTALL_ERROR) {
    console.log(`${fail} Failed to yarn install in '${dir}'`);
    process.exit(1);
  }
}

function getSpecHash () {
  return Promise.all([
    (async () => {
      const hasher = crypto.createHash('SHA256');
      hasher.update(fs.readFileSync(path.resolve(__dirname, '../spec/package.json')));
      hasher.update(fs.readFileSync(path.resolve(__dirname, '../spec-main/package.json')));
      hasher.update(fs.readFileSync(path.resolve(__dirname, '../spec/yarn.lock')));
      hasher.update(fs.readFileSync(path.resolve(__dirname, '../spec-main/yarn.lock')));
      hasher.update(fs.readFileSync(path.resolve(__dirname, '../script/spec-runner.js')));
      return hasher.digest('hex');
    })(),
    (async () => {
      const specNodeModulesPath = path.resolve(__dirname, '../spec/node_modules');
      if (!fs.existsSync(specNodeModulesPath)) {
        return null;
      }
      const { hash } = await hashElement(specNodeModulesPath, {
        folders: {
          exclude: ['.bin']
        }
      });
      return hash;
    })()
  ]);
}

main().catch((error) => {
  console.error('An error occurred inside the spec runner:', error);
  process.exit(1);
});

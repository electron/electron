#!/usr/bin/env node

const { ElectronVersions, Installer } = require('@electron/fiddle-core');

const { DOMParser } = require('@xmldom/xmldom');
const chalk = require('chalk');
const { hashElement } = require('folder-hash');
const minimist = require('minimist');

const childProcess = require('node:child_process');
const crypto = require('node:crypto');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');

const unknownFlags = [];

const pass = chalk.green('✓');
const fail = chalk.red('✗');

const FAILURE_STATUS_KEY = 'Electron_Spec_Runner_Failures';

const args = minimist(process.argv, {
  boolean: ['skipYarnInstall'],
  string: ['runners', 'target', 'electronVersion'],
  number: ['enableRerun'],
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
const { YARN_SCRIPT_PATH } = require('./yarn');

const BASE = path.resolve(__dirname, '../..');

const runners = new Map([
  ['main', { description: 'Main process specs', run: runMainProcessElectronTests }]
]);

const specHashPath = path.resolve(__dirname, '../spec/.hash');

if (args.electronVersion) {
  if (args.runners && args.runners !== 'main') {
    console.log(`${fail} only 'main' runner can be used with --electronVersion`);
    process.exit(1);
  }

  args.runners = 'main';
}

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
  if (args.electronVersion) {
    const versions = await ElectronVersions.create();
    if (args.electronVersion === 'latest') {
      args.electronVersion = versions.latest.version;
    } else if (args.electronVersion.startsWith('latest@')) {
      const majorVersion = parseInt(args.electronVersion.slice('latest@'.length));
      const ver = versions.inMajor(majorVersion).slice(-1)[0];
      if (ver) {
        args.electronVersion = ver.version;
      } else {
        console.log(`${fail} '${majorVersion}' is not a recognized Electron major version`);
        process.exit(1);
      }
    } else if (!versions.isVersion(args.electronVersion)) {
      console.log(`${fail} '${args.electronVersion}' is not a recognized Electron version`);
      process.exit(1);
    }

    const versionString = `v${args.electronVersion}`;
    console.log(`Running against Electron ${chalk.green(versionString)}`);
  }

  const [lastSpecHash, lastSpecInstallHash] = loadLastSpecHash();
  const [currentSpecHash, currentSpecInstallHash] = await getSpecHash();
  const somethingChanged = (currentSpecHash !== lastSpecHash) ||
      (lastSpecInstallHash !== currentSpecInstallHash);

  if (somethingChanged && !args.skipYarnInstall) {
    await installSpecModules(path.resolve(__dirname, '..', 'spec'));
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

async function asyncSpawn (exe, runnerArgs) {
  return new Promise((resolve, reject) => {
    let forceExitResult = 0;
    const child = childProcess.spawn(exe, runnerArgs, {
      cwd: path.resolve(__dirname, '../..')
    });
    if (process.env.ELECTRON_TEST_PID_DUMP_PATH && child.pid) {
      fs.writeFileSync(process.env.ELECTRON_TEST_PID_DUMP_PATH, child.pid.toString());
    }
    child.stdout.pipe(process.stdout);
    child.stderr.pipe(process.stderr);
    if (process.env.ELECTRON_FORCE_TEST_SUITE_EXIT) {
      child.stdout.on('data', data => {
        const failureRE = RegExp(`${FAILURE_STATUS_KEY}: (\\d.*)`);
        const failures = data.toString().match(failureRE);
        if (failures) {
          forceExitResult = parseInt(failures[1], 10);
        }
      });
    }
    child.on('error', error => reject(error));
    child.on('close', (status, signal) => {
      let returnStatus = 0;
      if (process.env.ELECTRON_FORCE_TEST_SUITE_EXIT) {
        returnStatus = forceExitResult;
      } else {
        returnStatus = status;
      }
      resolve({ status: returnStatus, signal });
    });
  });
}

function parseJUnitXML (specDir) {
  if (!fs.existsSync(process.env.MOCHA_FILE)) {
    console.error('JUnit XML file not found:', process.env.MOCHA_FILE);
    return [];
  }

  const xmlContent = fs.readFileSync(process.env.MOCHA_FILE, 'utf8');
  const parser = new DOMParser();
  const xmlDoc = parser.parseFromString(xmlContent, 'text/xml');

  const failedTests = [];
  // find failed tests by looking for all testsuite nodes with failure > 0
  const testSuites = xmlDoc.getElementsByTagName('testsuite');
  for (let i = 0; i < testSuites.length; i++) {
    const testSuite = testSuites[i];
    const failures = testSuite.getAttribute('failures');
    if (failures > 0) {
      const testcases = testSuite.getElementsByTagName('testcase');

      for (let i = 0; i < testcases.length; i++) {
        const testcase = testcases[i];
        const failures = testcase.getElementsByTagName('failure');
        const errors = testcase.getElementsByTagName('error');

        if (failures.length > 0 || errors.length > 0) {
          const testName = testcase.getAttribute('name');
          const filePath = testSuite.getAttribute('file');
          const fileName = filePath ? path.relative(specDir, filePath) : 'unknown file';
          const failureInfo = {
            name: testName,
            file: fileName,
            filePath
          };
          if (failures.length > 0) {
            failureInfo.failure = failures[0].textContent || failures[0].nodeValue || 'No failure message';
          }

          if (errors.length > 0) {
            failureInfo.error = errors[0].textContent || errors[0].nodeValue || 'No error message';
          }

          failedTests.push(failureInfo);
        }
      }
    }
  }

  return failedTests;
}

async function rerunFailedTest (specDir, testName, testInfo) {
  console.log('\n========================================');
  console.log(`Rerunning failed test: ${testInfo.name} (${testInfo.file})`);
  console.log('========================================');

  let grepPattern = testInfo.name;

  // Escape special regex characters in test name
  grepPattern = grepPattern.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');

  const args = [];
  if (testInfo.filePath) {
    args.push('--files', testInfo.filePath);
  }
  args.push('-g', grepPattern);

  const success = await runTestUsingElectron(specDir, testName, false, args);

  if (success) {
    console.log(`✅ Test passed: ${testInfo.name}`);
    return true;
  } else {
    console.log(`❌ Test failed again: ${testInfo.name}`);
    return false;
  }
}

async function rerunFailedTests (specDir, testName) {
  console.log('\n📋 Parsing JUnit XML for failed tests...');
  const failedTests = parseJUnitXML(specDir);

  if (failedTests.length === 0) {
    console.log('No failed tests could be found.');
    process.exit(1);
    return;
  }

  // Save off the original junit xml file
  if (fs.existsSync(process.env.MOCHA_FILE)) {
    fs.copyFileSync(process.env.MOCHA_FILE, `${process.env.MOCHA_FILE}.save`);
  }

  console.log(`\n📊 Found ${failedTests.length} failed test(s):`);
  failedTests.forEach((test, index) => {
    console.log(`  ${index + 1}. ${test.name} (${test.file})`);
  });

  // Step 3: Rerun each failed test individually
  console.log('\n🔄 Rerunning failed tests individually...\n');

  const results = {
    total: failedTests.length,
    passed: 0,
    failed: 0
  };

  let index = 0;
  for (const testInfo of failedTests) {
    let runCount = 0;
    let success = false;
    let retryTest = false;
    while (!success && (runCount < args.enableRerun)) {
      success = await rerunFailedTest(specDir, testName, testInfo);
      if (success) {
        results.passed++;
      } else {
        if (runCount === args.enableRerun - 1) {
          results.failed++;
        } else {
          retryTest = true;
          console.log(`Retrying test (${runCount + 1}/${args.enableRerun})...`);
        }
      }

      // Add a small delay between tests
      if (retryTest || index < failedTests.length - 1) {
        console.log('\nWaiting 2 seconds before next test...');
        await new Promise(resolve => setTimeout(resolve, 2000));
      }
      runCount++;
    }
    index++;
  };

  // Step 4: Summary
  console.log('\n📈 Summary:');
  console.log(`Total failed tests: ${results.total}`);
  console.log(`Passed on rerun: ${results.passed}`);
  console.log(`Still failing: ${results.failed}`);

  // Restore the original junit xml file
  if (fs.existsSync(`${process.env.MOCHA_FILE}.save`)) {
    fs.renameSync(`${process.env.MOCHA_FILE}.save`, process.env.MOCHA_FILE);
  }

  if (results.failed === 0) {
    console.log('🎉 All previously failed tests now pass!');
  } else {
    console.log(`⚠️  ${results.failed} test(s) are still failing`);
    process.exit(1);
  }
}

async function runTestUsingElectron (specDir, testName, shouldRerun, additionalArgs = []) {
  let exe;
  if (args.electronVersion) {
    const installer = new Installer();
    exe = await installer.install(args.electronVersion);
  } else {
    exe = path.resolve(BASE, utils.getElectronExec());
  }
  let argsToPass = unknownArgs.slice(2);
  if (additionalArgs.includes('--files')) {
    argsToPass = argsToPass.filter(arg => (arg.toString().indexOf('--files') === -1 && arg.toString().indexOf('spec/') === -1));
  }
  const runnerArgs = [`electron/${specDir}`, ...argsToPass, ...additionalArgs];
  if (process.platform === 'linux') {
    runnerArgs.unshift(path.resolve(__dirname, 'dbus_mock.py'), exe);
    exe = 'python3';
  }
  console.log(`Running: ${exe} ${runnerArgs.join(' ')}`);
  const { status, signal } = await asyncSpawn(exe, runnerArgs);
  if (status !== 0) {
    if (status) {
      const textStatus = process.platform === 'win32' ? `0x${status.toString(16)}` : status.toString();
      console.log(`${fail} Electron tests failed with code ${textStatus}.`);
    } else {
      console.log(`${fail} Electron tests failed with kill signal ${signal}.`);
    }
    if (shouldRerun) {
      await rerunFailedTests(specDir, testName);
    } else {
      process.exit(1);
    }
  }
  console.log(`${pass} Electron ${testName} process tests passed.`);
  return true;
}

async function runMainProcessElectronTests () {
  let shouldRerun = false;
  if (args.enableRerun && args.enableRerun > 0) {
    shouldRerun = true;
  }
  await runTestUsingElectron('spec', 'main', shouldRerun);
}

async function installSpecModules (dir) {
  const env = {
    npm_config_msvs_version: '2022',
    ...process.env,
    CXXFLAGS: process.env.CXXFLAGS,
    npm_config_yes: 'true'
  };
  if (args.electronVersion) {
    env.npm_config_target = args.electronVersion;
    env.npm_config_disturl = 'https://electronjs.org/headers';
    env.npm_config_runtime = 'electron';
    env.npm_config_devdir = path.join(os.homedir(), '.electron-gyp');
    env.npm_config_build_from_source = 'true';
    const { status } = childProcess.spawnSync('npm', ['run', 'node-gyp-install', '--ensure'], {
      env,
      cwd: dir,
      stdio: 'inherit',
      shell: true
    });
    if (status !== 0) {
      console.log(`${fail} Failed to "npm run node-gyp-install" install in '${dir}'`);
      process.exit(1);
    }
  } else {
    env.npm_config_nodedir = path.resolve(BASE, `out/${utils.getOutDir({ shouldLog: true })}/gen/node_headers`);
  }
  if (fs.existsSync(path.resolve(dir, 'node_modules'))) {
    await fs.promises.rm(path.resolve(dir, 'node_modules'), { force: true, recursive: true });
  }
  const yarnArgs = [YARN_SCRIPT_PATH, 'install', '--immutable'];
  const { status } = childProcess.spawnSync(process.execPath, yarnArgs, {
    env,
    cwd: dir,
    stdio: 'inherit',
    shell: process.platform === 'win32'
  });
  if (status !== 0 && !process.env.IGNORE_YARN_INSTALL_ERROR) {
    console.log(`${fail} Failed to yarn install in '${dir}'`);
    process.exit(1);
  }

  if (process.platform === 'linux') {
    const { status: rebuildStatus } = childProcess.spawnSync('npm', ['rebuild', 'abstract-socket'], {
      env,
      cwd: dir,
      stdio: 'inherit',
      shell: process.platform === 'win32'
    });
    if (rebuildStatus !== 0) {
      console.log(`${fail} Failed to rebuild abstract-socket native module`);
      process.exit(1);
    }
  }
}

function getSpecHash () {
  return Promise.all([
    (async () => {
      const hasher = crypto.createHash('SHA256');
      hasher.update(fs.readFileSync(path.resolve(__dirname, '../yarn.lock')));
      hasher.update(fs.readFileSync(path.resolve(__dirname, '../spec/package.json')));
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

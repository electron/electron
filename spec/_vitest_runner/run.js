#!/usr/bin/env node

const childProcess = require('node:child_process');
const crypto = require('node:crypto');
const fs = require('node:fs');
const path = require('node:path');

const { getAbsoluteElectronExec, getOutDir } = require('../../script/lib/utils');
const { YARN_SCRIPT_PATH } = require('../../script/yarn');

const ROOT = path.resolve(__dirname, '..', '..');
const SPEC_DIR = path.resolve(ROOT, 'spec');
const SPEC_HASH_PATH = path.resolve(SPEC_DIR, '.hash');
const VITEST_BIN = path.join(ROOT, 'node_modules', '.bin', 'vitest');

const rawArgs = process.argv.slice(2);
const skipYarnInstall = rawArgs.includes('--skipYarnInstall');
const vitestArgs = rawArgs.filter((a) => a !== '--skipYarnInstall');

function getSpecHash() {
  const hasher = crypto.createHash('SHA256');
  hasher.update(fs.readFileSync(path.resolve(ROOT, 'yarn.lock')));
  hasher.update(fs.readFileSync(path.resolve(SPEC_DIR, 'package.json')));
  hasher.update(fs.readFileSync(__filename));
  return hasher.digest('hex');
}

function installSpecModules() {
  const env = {
    npm_config_msvs_version: '2022',
    ...process.env,
    npm_config_nodedir: path.resolve(ROOT, '..', `out/${getOutDir({ shouldLog: true })}/gen/node_headers`),
    npm_config_yes: 'true'
  };
  const nodeModules = path.resolve(SPEC_DIR, 'node_modules');
  if (fs.existsSync(nodeModules)) {
    fs.rmSync(nodeModules, { force: true, recursive: true });
  }
  const { status } = childProcess.spawnSync(process.execPath, [YARN_SCRIPT_PATH, 'install', '--immutable'], {
    env,
    cwd: SPEC_DIR,
    stdio: 'inherit',
    shell: process.platform === 'win32'
  });
  if (status !== 0 && !process.env.IGNORE_YARN_INSTALL_ERROR) {
    console.error(`Failed to yarn install in '${SPEC_DIR}'`);
    process.exit(1);
  }
  if (process.platform === 'linux') {
    const { status: rebuildStatus } = childProcess.spawnSync('npm', ['rebuild', 'abstract-socket'], {
      env,
      cwd: SPEC_DIR,
      stdio: 'inherit'
    });
    if (rebuildStatus !== 0) {
      console.error('Failed to rebuild abstract-socket native module');
      process.exit(1);
    }
  }
}

function generateTypeDefinitions() {
  const { status } = childProcess.spawnSync('npm', ['run', 'create-typescript-definitions'], {
    cwd: ROOT,
    stdio: 'inherit',
    shell: true
  });
  if (status !== 0) {
    throw new Error(`Electron typescript definition generation failed with exit code: ${status}.`);
  }
}

if (!skipYarnInstall) {
  const currentHash = getSpecHash();
  const lastHash = fs.existsSync(SPEC_HASH_PATH) ? fs.readFileSync(SPEC_HASH_PATH, 'utf8') : null;
  if (currentHash !== lastHash || !fs.existsSync(path.resolve(SPEC_DIR, 'node_modules'))) {
    installSpecModules();
    fs.writeFileSync(SPEC_HASH_PATH, getSpecHash());
  }
}

if (!fs.existsSync(path.resolve(ROOT, 'electron.d.ts'))) {
  console.log('Generating electron.d.ts as it is missing');
  generateTypeDefinitions();
}

const exe = process.env.ELECTRON_TESTS_EXECUTABLE || getAbsoluteElectronExec();
console.log(`Electron binary: ${exe}`);

const env = {
  ...process.env,
  ELECTRON_TESTS_EXECUTABLE: exe
};

const configPath = path.join(__dirname, 'vitest.config.ts');

function runVitest(extraArgs) {
  let command = VITEST_BIN;
  let args = ['run', '--config', configPath, ...extraArgs];
  // On Linux, run vitest under a mocked D-Bus so every spawned Electron worker
  // inherits the session/system bus addresses.
  if (process.platform === 'linux') {
    args = [path.resolve(ROOT, 'script', 'dbus_mock.py'), command, ...args];
    command = 'python3';
  }
  const result = childProcess.spawnSync(command, args, { cwd: ROOT, stdio: 'inherit', env });
  return result.status ?? 1;
}

const SERIAL_DIR = path.join('spec', 'serial');

function isSerialPath(a) {
  const rel = path.relative(ROOT, path.resolve(ROOT, a));
  return rel === SERIAL_DIR || rel.startsWith(SERIAL_DIR + path.sep);
}

function isTestPath(a) {
  return !a.startsWith('-') && (a.includes('/') || a.includes(path.sep) || /\.spec\.[cm]?[tj]s$/.test(a));
}

function serialArgs(args) {
  const out = [];
  let hasPositional = false;
  for (let i = 0; i < args.length; i++) {
    const a = args[i];
    if (a.startsWith('--outputFile.junit=')) {
      out.push(a.replace(/\.xml$/, '-serial.xml'));
    } else if (a === '--outputFile.junit') {
      out.push(a, args[++i].replace(/\.xml$/, '-serial.xml'));
    } else if (!isTestPath(a)) {
      out.push(a);
    } else {
      hasPositional = true;
      if (isSerialPath(a)) out.push(a);
    }
  }
  return { args: out, hasPositional };
}

const { args: sArgs, hasPositional } = serialArgs(vitestArgs);
const positionalSerial = hasPositional && sArgs.some((a) => isTestPath(a));
const positionalParallel = hasPositional && vitestArgs.some((a) => isTestPath(a) && !isSerialPath(a));

let parallelStatus = 0;
if (!hasPositional || positionalParallel) {
  parallelStatus = runVitest(['--exclude', 'spec/serial/**', ...vitestArgs]);
}

let serialStatus = 0;
if (!hasPositional || positionalSerial) {
  console.log('\nRunning spec/serial/** without file parallelism...');
  serialStatus = runVitest(['--no-file-parallelism', ...sArgs, ...(hasPositional ? [] : ['spec/serial/**/*.spec.ts'])]);
}

process.exit(parallelStatus || serialStatus);

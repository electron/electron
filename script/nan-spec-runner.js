const minimist = require('minimist');

const cp = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');

const BASE = path.resolve(__dirname, '../..');
const NAN_DIR = path.resolve(BASE, 'third_party', 'nan');
const NPX_CMD = process.platform === 'win32' ? 'npx.cmd' : 'npx';

const utils = require('./lib/utils');
const { YARN_SCRIPT_PATH } = require('./yarn');

if (!require.main) {
  throw new Error('Must call the nan spec runner directly');
}

const args = minimist(process.argv.slice(2), {
  string: ['only']
});

const getNodeGypVersion = () => {
  const nanPackageJSONPath = path.join(NAN_DIR, 'package.json');
  const nanPackageJSON = JSON.parse(fs.readFileSync(nanPackageJSONPath, 'utf8'));
  const { devDependencies } = nanPackageJSON;
  const nodeGypVersion = devDependencies['node-gyp'];
  return nodeGypVersion || 'latest';
};

async function main () {
  const outDir = utils.getOutDir({ shouldLog: true });
  const nodeDir = path.resolve(BASE, 'out', outDir, 'gen', 'node_headers');
  const env = {
    npm_config_msvs_version: '2022',
    ...process.env,
    npm_config_nodedir: nodeDir,
    npm_config_arch: process.env.NPM_CONFIG_ARCH,
    npm_config_yes: 'true'
  };

  const clangDir = path.resolve(BASE, 'third_party', 'llvm-build', 'Release+Asserts', 'bin');
  const cc = path.resolve(clangDir, 'clang');
  const cxx = path.resolve(clangDir, 'clang++');
  const ld = path.resolve(clangDir, 'lld');

  const platformFlags = [];
  if (process.platform === 'darwin') {
    const sdkPath = path.resolve(BASE, 'out', outDir, 'sdk', 'xcode_links');
    const sdks = (await fs.promises.readdir(sdkPath)).filter(f => f.endsWith('.sdk'));

    if (!sdks.length) {
      console.error('Could not find an SDK to use for the NAN tests');
      process.exit(1);
    }

    const sdkToUse = sdks.sort((a, b) => {
      const getVer = s => s.match(/(\d+)\.?(\d*)/)?.[0] || '0';
      return getVer(b).localeCompare(getVer(a), undefined, { numeric: true });
    })[0];

    if (sdks.length > 1) {
      console.warn(`Multiple SDKs found - using ${sdkToUse}`);
    }

    platformFlags.push(`-isysroot ${path.resolve(sdkPath, sdkToUse)}`);
  }

  const cflags = [
    '-Wno-trigraphs',
    '-fPIC',
    ...platformFlags
  ].join(' ');

  const cxxflags = [
    '-std=c++20',
    '-Wno-trigraphs',
    '-fno-exceptions',
    '-fno-rtti',
    '-nostdinc++',
    `-isystem "${path.resolve(BASE, 'buildtools', 'third_party', 'libc++')}"`,
    `-isystem "${path.resolve(BASE, 'third_party', 'libc++', 'src', 'include')}"`,
    `-isystem "${path.resolve(BASE, 'third_party', 'libc++abi', 'src', 'include')}"`,
    ' -fvisibility-inlines-hidden',
    '-fPIC',
    '-D_LIBCPP_ABI_NAMESPACE=Cr',
    '-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE',
    ...platformFlags
  ].join(' ');

  const ldflags = [
    '-stdlib=libc++',
    '-fuse-ld=lld',
    `-L"${path.resolve(BASE, 'out', outDir, 'obj', 'buildtools', 'third_party', 'libc++abi')}"`,
    `-L"${path.resolve(BASE, 'out', outDir, 'obj', 'buildtools', 'third_party', 'libc++')}"`,
    '-lc++abi',
    ...platformFlags
  ].join(' ');

  if (process.platform !== 'win32') {
    env.CC = cc;
    env.CFLAGS = cflags;
    env.CXX = cxx;
    env.CXXFLAGS = cxxflags;
    env.LD = ld;
    env.LDFLAGS = ldflags;
  }

  const nodeGypVersion = getNodeGypVersion();
  const { status: buildStatus, signal } = cp.spawnSync(NPX_CMD, [`node-gyp@${nodeGypVersion}`, 'rebuild', '--verbose', '--directory', 'test', '-j', 'max'], {
    env,
    cwd: NAN_DIR,
    stdio: 'inherit',
    shell: process.platform === 'win32'
  });
  if (buildStatus !== 0 || signal != null) {
    console.error('Failed to build nan test modules');
    return process.exit(buildStatus !== 0 ? buildStatus : signal);
  }

  const { status: installStatus, signal: installSignal } = cp.spawnSync(process.execPath, [YARN_SCRIPT_PATH, 'install'], {
    env,
    cwd: NAN_DIR,
    stdio: 'inherit',
    shell: process.platform === 'win32'
  });

  if (installStatus !== 0 || installSignal != null) {
    console.error('Failed to install nan node_modules');
    return process.exit(installStatus !== 0 ? installStatus : installSignal);
  }

  const onlyTests = args.only?.split(',');

  const DISABLED_TESTS = new Set([
    'nannew-test.js',
    'buffer-test.js'
  ]);
  const testsToRun = fs.readdirSync(path.resolve(NAN_DIR, 'test', 'js'))
    .filter(test => !DISABLED_TESTS.has(test))
    .filter(test => {
      return !onlyTests || onlyTests.includes(test) || onlyTests.includes(test.replace('.js', '')) || onlyTests.includes(test.replace('-test.js', ''));
    })
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

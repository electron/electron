// Downloads the llvm-profdata binary matching the clang version Electron is
// built with, for machines that do NOT have a Chromium checkout (e.g. the CI
// profile-merge job, which runs on a plain ubuntu runner).
//
//   node script/pgo/fetch-llvm-profdata.js \
//     --deps <path to electron DEPS> \
//     --output-dir <dir to extract into>
//
// Prints the absolute path to the llvm-profdata binary as the last stdout line.
//
// How it works:
//   1. electron's DEPS pins chromium_version.
//   2. tools/clang/scripts/update.py at that Chromium revision defines
//      CLANG_REVISION / CLANG_SUB_REVISION (fetched from gitiles).
//   3. The clang "coverage_tools" package for that revision contains
//      llvm-profdata; download and extract it.
//
// Note: this fetches the Linux x64 package - the merge job always runs on a
// Linux x64 runner. .profraw files from any platform/arch can be merged by it.
const { execFileSync } = require('node:child_process');
const fs = require('node:fs');
const https = require('node:https');
const os = require('node:os');
const path = require('node:path');

const CLANG_PACKAGE_BASE_URL = 'https://commondatastorage.googleapis.com/chromium-browser-clang';

function parseArgs (argv) {
  const args = {};
  for (let i = 2; i < argv.length; i++) {
    if (argv[i].startsWith('--')) {
      const key = argv[i].slice(2);
      args[key] = argv[i + 1] && !argv[i + 1].startsWith('--') ? argv[++i] : true;
    }
  }
  return args;
}

function log (...args) {
  // Logs go to stderr - stdout carries only the resulting binary path.
  // eslint-disable-next-line no-console
  console.error('[fetch-llvm-profdata]', ...args);
}

function fetch (url) {
  return new Promise((resolve, reject) => {
    https.get(url, { headers: { 'User-Agent': 'electron-pgo-tooling' } }, (res) => {
      if (res.statusCode >= 300 && res.statusCode < 400 && res.headers.location) {
        res.resume();
        return resolve(fetch(new URL(res.headers.location, url).toString()));
      }
      if (res.statusCode !== 200) {
        res.resume();
        return reject(new Error(`HTTP ${res.statusCode} for ${url}`));
      }
      const chunks = [];
      res.on('data', (chunk) => chunks.push(chunk));
      res.on('end', () => resolve(Buffer.concat(chunks)));
      res.on('error', reject);
    }).on('error', reject);
  });
}

async function main () {
  const args = parseArgs(process.argv);
  if (!args.deps || !args['output-dir']) {
    console.error('Usage: fetch-llvm-profdata.js --deps <DEPS file> --output-dir <dir>');
    process.exit(1);
  }

  const outputDir = path.resolve(args['output-dir']);
  const profdataPath = path.join(outputDir, 'bin', 'llvm-profdata');
  if (fs.existsSync(profdataPath)) {
    log('already present');
    console.log(profdataPath);
    return;
  }

  // 1. Chromium version from electron's DEPS.
  const deps = fs.readFileSync(path.resolve(args.deps), 'utf8');
  const versionMatch = deps.match(/'chromium_version':\s*'([^']+)'/s);
  if (!versionMatch) throw new Error('could not find chromium_version in DEPS');
  const chromiumVersion = versionMatch[1];
  log(`chromium version: ${chromiumVersion}`);

  // 2. CLANG_REVISION from that revision's update.py (gitiles serves base64).
  const updatePyUrl = `https://chromium.googlesource.com/chromium/src/+/refs/tags/${chromiumVersion}/tools/clang/scripts/update.py?format=TEXT`;
  const updatePy = Buffer.from((await fetch(updatePyUrl)).toString(), 'base64').toString('utf8');
  const revMatch = updatePy.match(/^CLANG_REVISION = '([^']+)'/m);
  const subRevMatch = updatePy.match(/^CLANG_SUB_REVISION = (\d+)/m);
  if (!revMatch || !subRevMatch) throw new Error('could not parse CLANG_REVISION from update.py');
  const packageVersion = `${revMatch[1]}-${subRevMatch[1]}`;
  log(`clang package version: ${packageVersion}`);

  // 3. Download and extract the coverage tools package. Note: the package is
  //    *named* coverage_tools in update.py but the file on storage is
  //    llvm-code-coverage-<version>.tar.xz.
  const packageUrl = `${CLANG_PACKAGE_BASE_URL}/Linux_x64/llvm-code-coverage-${packageVersion}.tar.xz`;
  log(`downloading ${packageUrl}`);
  const tarball = path.join(os.tmpdir(), `llvm-code-coverage-${packageVersion}.tar.xz`);
  fs.writeFileSync(tarball, await fetch(packageUrl));

  fs.mkdirSync(outputDir, { recursive: true });
  execFileSync('tar', ['-xJf', tarball, '-C', outputDir], { stdio: ['ignore', 'inherit', 'inherit'] });
  fs.rmSync(tarball, { force: true });

  if (!fs.existsSync(profdataPath)) {
    // Some package layouts nest under Release+Asserts - handle both.
    const nested = path.join(outputDir, 'Release+Asserts', 'bin', 'llvm-profdata');
    if (fs.existsSync(nested)) {
      console.log(nested);
      return;
    }
    throw new Error(`llvm-profdata not found under ${outputDir} after extraction`);
  }
  console.log(profdataPath);
}

main().catch((err) => {
  console.error('[fetch-llvm-profdata] FATAL:', err);
  process.exit(1);
});

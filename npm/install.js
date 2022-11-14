#!/usr/bin/env node

const { version } = require('./package');

const childProcess = require('child_process');
const fs = require('fs');
const os = require('os');
const path = require('path');
const extract = require('extract-zip');
const { downloadArtifact } = require('@electron/get');

if (process.env.ELECTRON_SKIP_BINARY_DOWNLOAD) {
  process.exit(0);
}

const platformPath = getPlatformPath();

if (isInstalled()) {
  process.exit(0);
}

const platform = process.env.npm_config_platform || process.platform;
let arch = process.env.npm_config_arch || process.arch;

if (platform === 'darwin' && process.platform === 'darwin' && arch === 'x64' &&
    process.env.npm_config_arch === undefined) {
  // When downloading for macOS ON macOS and we think we need x64 we should
  // check if we're running under rosetta and download the arm64 version if appropriate
  try {
    const output = childProcess.execSync('sysctl -in sysctl.proc_translated');
    if (output.toString().trim() === '1') {
      arch = 'arm64';
    }
  } catch {
    // Ignore failure
  }
}

// downloads if not cached
downloadArtifact({
  version,
  artifactName: 'electron',
  force: process.env.force_no_cache === 'true',
  cacheRoot: process.env.electron_config_cache,
  checksums: process.env.electron_use_remote_checksums ? undefined : require('./checksums.json'),
  platform,
  arch
}).then(extractFile).catch(err => {
  console.error(err.stack);
  process.exit(1);
});

function isInstalled () {
  try {
    if (fs.readFileSync(path.join(__dirname, 'dist', 'version'), 'utf-8').replace(/^v/, '') !== version) {
      return false;
    }

    if (fs.readFileSync(path.join(__dirname, 'path.txt'), 'utf-8') !== platformPath) {
      return false;
    }
  } catch (ignored) {
    return false;
  }

  const electronPath = process.env.ELECTRON_OVERRIDE_DIST_PATH || path.join(__dirname, 'dist', platformPath);

  return fs.existsSync(electronPath);
}

// unzips and makes path.txt point at the correct executable
function extractFile (zipPath) {
  return new Promise((resolve, reject) => {
    const distPath = process.env.ELECTRON_OVERRIDE_DIST_PATH || path.join(__dirname, 'dist');

    extract(zipPath, { dir: path.join(__dirname, 'dist') })
      .then(() => {
        // If the zip contains an "electron.d.ts" file,
        // move that up
        const srcTypeDefPath = path.join(distPath, 'electron.d.ts');
        const targetTypeDefPath = path.join(__dirname, 'electron.d.ts');
        const hasTypeDefinitions = fs.existsSync(srcTypeDefPath);

        if (hasTypeDefinitions) {
          try {
            fs.renameSync(srcTypeDefPath, targetTypeDefPath);
          } catch (err) {
            reject(err);
          }
        }

        // Write a "path.txt" file.
        return fs.promises.writeFile(path.join(__dirname, 'path.txt'), platformPath);
      })
      .catch((err) => reject(err));
  });
}

function getPlatformPath () {
  const platform = process.env.npm_config_platform || os.platform();

  switch (platform) {
    case 'mas':
    case 'darwin':
      return 'Electron.app/Contents/MacOS/Electron';
    case 'freebsd':
    case 'openbsd':
    case 'linux':
      return 'electron';
    case 'win32':
      return 'electron.exe';
    default:
      throw new Error('Electron builds are not available on platform: ' + platform);
  }
}

#!/usr/bin/env node

const { downloadArtifact } = require('@electron/get');
const { ZipReaderStream } = require('@zip.js/zip.js');

const childProcess = require('child_process');
const fs = require('fs');
const os = require('os');
const path = require('path');
const { Readable, Writable } = require('stream');

const { version } = require('./package');

const platformPath = getPlatformPath();

if (isInstalled()) {
  process.exit(0);
}

const platform = process.env.ELECTRON_INSTALL_PLATFORM || process.env.npm_config_platform || process.platform;
let arch = process.env.ELECTRON_INSTALL_ARCH || process.env.npm_config_arch || process.arch;

if (
  platform === 'darwin' &&
  process.platform === 'darwin' &&
  arch === 'x64' &&
  process.env.npm_config_arch === undefined
) {
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
  checksums:
    process.env.electron_use_remote_checksums || process.env.npm_config_electron_use_remote_checksums
      ? undefined
      : require('./checksums.json'),
  platform,
  arch
})
  .then(extractFile)
  .catch((err) => {
    console.error(err.stack);
    process.exit(1);
  });

function isInstalled() {
  try {
    if (fs.readFileSync(path.join(__dirname, 'dist', 'version'), 'utf-8').replace(/^v/, '') !== version) {
      return false;
    }

    if (fs.readFileSync(path.join(__dirname, 'path.txt'), 'utf-8') !== platformPath) {
      return false;
    }
  } catch {
    return false;
  }

  const electronPath = process.env.ELECTRON_OVERRIDE_DIST_PATH || path.join(__dirname, 'dist', platformPath);

  return fs.existsSync(electronPath);
}

// unzips and makes path.txt point at the correct executable
async function extractFile(zipPath) {
  const distPath = process.env.ELECTRON_OVERRIDE_DIST_PATH || path.join(__dirname, 'dist');
  const targetDir = path.join(__dirname, 'dist');

  const fileStream = Readable.toWeb(fs.createReadStream(zipPath));
  const zipStream = fileStream.pipeThrough(new ZipReaderStream());

  for await (const entry of zipStream) {
    const entryPath = path.join(targetDir, entry.filename);

    if (!entryPath.startsWith(targetDir + path.sep) && entryPath !== targetDir) {
      throw new Error(`Zip entry outside target directory: ${entry.filename}`);
    }

    if (entry.directory) {
      fs.mkdirSync(entryPath, { recursive: true });
    } else {
      fs.mkdirSync(path.dirname(entryPath), { recursive: true });
      const writable = fs.createWriteStream(entryPath);
      await entry.readable.pipeTo(Writable.toWeb(writable));

      if (entry.executable) {
        fs.chmodSync(entryPath, 0o755);
      } else if (entry.externalFileAttribute) {
        // Restore Unix file permissions if available
        const unixMode = (entry.externalFileAttribute >>> 16) & 0xFFFF;
        if (unixMode !== 0) {
          fs.chmodSync(entryPath, unixMode);
        }
      }
    }
  }

  // If the zip contains an "electron.d.ts" file,
  // move that up
  const srcTypeDefPath = path.join(distPath, 'electron.d.ts');
  const targetTypeDefPath = path.join(__dirname, 'electron.d.ts');
  const hasTypeDefinitions = fs.existsSync(srcTypeDefPath);

  if (hasTypeDefinitions) {
    fs.renameSync(srcTypeDefPath, targetTypeDefPath);
  }

  // Write a "path.txt" file.
  await fs.promises.writeFile(path.join(__dirname, 'path.txt'), platformPath);
}

function getPlatformPath() {
  const platform = process.env.ELECTRON_INSTALL_PLATFORM || process.env.npm_config_platform || os.platform();

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

const { spawnSync } = require('child_process');
const fs = require('fs');
const path = require('path');

const pathFile = path.join(__dirname, 'path.txt');

function downloadElectron () {
  console.log('Downloading Electron binary...');
  const result = spawnSync(process.execPath, [path.join(__dirname, 'install.js')], {
    stdio: 'inherit'
  });
  if (result.status !== 0) {
    throw new Error(
      'Electron failed to install correctly. Please delete `node_modules/electron` and run "npx install-electron --no" manually.'
    );
  }
}

/**
 * Fetches the path to the Electron executable to use in development mode.
 * If the executable is missing, attempt to download it first.
 *
 * @returns the path to the Electron executable to run
 */
function getElectronPath () {
  let executablePath;
  if (fs.existsSync(pathFile)) {
    executablePath = fs.readFileSync(pathFile, 'utf-8');
  }
  if (process.env.ELECTRON_OVERRIDE_DIST_PATH) {
    return path.join(process.env.ELECTRON_OVERRIDE_DIST_PATH, executablePath || 'electron');
  }
  if (executablePath) {
    const fullPath = path.join(__dirname, 'dist', executablePath);
    if (!fs.existsSync(fullPath)) {
      downloadElectron();
    }
    return fullPath;
  } else {
    try {
      downloadElectron();
    } catch {
      throw new Error(
        'Electron failed to install correctly. Please delete `node_modules/electron` and run "npx install-electron --no" manually.'
      );
    }
    executablePath = fs.readFileSync(pathFile, 'utf-8');
    return path.join(__dirname, 'dist', executablePath);
  }
}

module.exports = getElectronPath();

const { spawnSync } = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');

const { ELECTRON_DIR, getOutDir } = require('./utils');

// Print the value of electron_version set in gn config.
module.exports.getElectronVersion = () => {
  // Read the override_electron_version from args.gn file.
  try {
    const outDir = path.resolve(ELECTRON_DIR, '..', 'out', getOutDir());
    const content = fs.readFileSync(path.join(outDir, 'args.gn'));
    const regex = /override_electron_version\s*=\s*["']([^"']+)["']/;
    const match = content.toString().match(regex);
    if (match) {
      return match[1];
    }
  } catch (error) {
    // Error may happen when trying to get version before running gn, which is a
    // valid case and error will be ignored.
  }
  // Most win32 machines have python.exe but no python3.exe.
  const python = process.platform === 'win32' ? 'python.exe' : 'python3';
  // Get the version from git tag if it is not defined in gn args.
  const output = spawnSync(python, [path.join(ELECTRON_DIR, 'script', 'get-git-version.py')]);
  if (output.status !== 0) {
    throw new Error(`Failed to get git tag, script quit with ${output.status}: ${output.stdout}`);
  }
  return output.stdout.toString().trim();
};

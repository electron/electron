import { describe, expect, it } from 'vitest';

import * as childProcess from 'node:child_process';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';

import { defer } from './lib/spec-helpers.ts';

// npm/index.js is the entry point of the published `electron` package. It exports the path of the Electron
// executable, which is what `require('electron')` returns and what the `electron` command runs.
const indexJsPath = path.resolve(import.meta.dirname, '..', 'npm', 'index.js');

// Where the executable lives inside an Electron distribution (a build's output directory or an extracted
// release zip) on the platform running the spec.
const getExecutableInDist = () => {
  if (process.platform === 'darwin') return 'Electron.app/Contents/MacOS/Electron';
  if (process.platform === 'win32') return 'electron.exe';
  return 'electron';
};

// Creates a directory to stand in for node_modules/electron.
const makePackageDir = () => {
  const dir = fs.realpathSync(fs.mkdtempSync(path.join(os.tmpdir(), 'electron-npm-package-spec-')));
  defer(() => fs.rmSync(dir, { recursive: true, force: true }));
  return dir;
};

// Evaluates require('electron') on a copy of npm/index.js in packageDir, so that the test decides whether
// path.txt and dist/ exist next to it.
const requireElectron = (packageDir: string, env: NodeJS.ProcessEnv = {}) => {
  const indexJs = path.join(packageDir, 'index.js');
  fs.copyFileSync(indexJsPath, indexJs);
  const { status, stdout, stderr } = childProcess.spawnSync(
    process.execPath,
    ['-p', `require(${JSON.stringify(indexJs)})`],
    {
      encoding: 'utf8',
      env: { ...process.env, ELECTRON_RUN_AS_NODE: '1', ELECTRON_OVERRIDE_DIST_PATH: '', ...env }
    }
  );
  expect(status, stderr).toBe(0);
  return stdout.trim();
};

describe('electron npm package', () => {
  describe('ELECTRON_OVERRIDE_DIST_PATH', () => {
    it('resolves to the executable of the current platform when path.txt does not exist', () => {
      const packageDir = makePackageDir();
      const distPath = path.join(packageDir, 'out', 'Testing');

      expect(requireElectron(packageDir, { ELECTRON_OVERRIDE_DIST_PATH: distPath })).toBe(
        path.join(distPath, getExecutableInDist())
      );
    });

    it('resolves to the executable listed in path.txt when it exists', () => {
      const packageDir = makePackageDir();
      const distPath = path.join(packageDir, 'out', 'Testing');
      fs.writeFileSync(path.join(packageDir, 'path.txt'), 'bin/custom-electron');

      expect(requireElectron(packageDir, { ELECTRON_OVERRIDE_DIST_PATH: distPath })).toBe(
        path.join(distPath, 'bin', 'custom-electron')
      );
    });
  });

  it('resolves to the executable in dist/ listed in path.txt', () => {
    const packageDir = makePackageDir();
    const executable = getExecutableInDist();
    const executablePath = path.join(packageDir, 'dist', executable);
    fs.mkdirSync(path.dirname(executablePath), { recursive: true });
    fs.writeFileSync(executablePath, '');
    fs.writeFileSync(path.join(packageDir, 'path.txt'), executable);

    expect(requireElectron(packageDir)).toBe(executablePath);
  });
});

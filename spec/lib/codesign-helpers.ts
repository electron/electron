import { expect } from 'chai';

import * as cp from 'node:child_process';
import * as fs from 'node:fs';
import * as path from 'node:path';

const features = process._linkedBinding('electron_common_features');
const fixturesPath = path.resolve(__dirname, '..', 'fixtures');

// Re-enable codesign tests for macOS x64
// Refs https://github.com/electron/electron/issues/48182
export const shouldRunCodesignTests =
    process.platform === 'darwin' &&
    !(process.env.CI) &&
    !process.mas &&
    !features.isComponentBuild();

let identity: string | null;

export function getCodesignIdentity () {
  if (identity === undefined) {
    const result = cp.spawnSync(path.resolve(__dirname, '../../script/codesign/get-trusted-identity.sh'));
    if (result.status !== 0 || result.stdout.toString().trim().length === 0) {
      identity = null;
    } else {
      identity = result.stdout.toString().trim();
    }
  }
  return identity;
}

export async function copyMacOSFixtureApp (newDir: string, fixture: string | null = 'initial') {
  const appBundlePath = path.resolve(process.execPath, '../../..');
  const newPath = path.resolve(newDir, 'Electron.app');
  cp.spawnSync('cp', ['-R', appBundlePath, path.dirname(newPath)]);
  if (fixture) {
    const appDir = path.resolve(newPath, 'Contents/Resources/app');
    await fs.promises.mkdir(appDir, { recursive: true });
    await fs.promises.cp(path.resolve(fixturesPath, 'auto-update', fixture), appDir, { recursive: true });
  }
  const plistPath = path.resolve(newPath, 'Contents', 'Info.plist');
  await fs.promises.writeFile(
    plistPath,
    (await fs.promises.readFile(plistPath, 'utf8')).replace('<key>BuildMachineOSBuild</key>', `<key>NSAppTransportSecurity</key>
    <dict>
        <key>NSAllowsArbitraryLoads</key>
        <true/>
        <key>NSExceptionDomains</key>
        <dict>
            <key>localhost</key>
            <dict>
                <key>NSExceptionAllowsInsecureHTTPLoads</key>
                <true/>
                <key>NSIncludesSubdomains</key>
                <true/>
            </dict>
        </dict>
    </dict><key>BuildMachineOSBuild</key>`)
  );
  return newPath;
};

export function spawn (cmd: string, args: string[], opts: any = {}) {
  let out = '';
  const child = cp.spawn(cmd, args, opts);
  child.stdout.on('data', (chunk: Buffer) => {
    out += chunk.toString();
  });
  child.stderr.on('data', (chunk: Buffer) => {
    out += chunk.toString();
  });
  return new Promise<{ code: number, out: string }>((resolve) => {
    child.on('exit', (code, signal) => {
      expect(signal).to.equal(null);
      resolve({
        code: code!,
        out
      });
    });
  });
};

export function signApp (appPath: string, identity: string) {
  return spawn('codesign', ['-s', identity, '--deep', '--force', appPath]);
};

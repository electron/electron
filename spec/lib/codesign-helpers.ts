import { expect } from 'chai';

import * as cp from 'node:child_process';
import * as fs from 'node:fs';
import * as path from 'node:path';

const features = process._linkedBinding('electron_common_features');
const fixturesPath = path.resolve(__dirname, '..', 'fixtures');

export const shouldRunCodesignTests = process.platform === 'darwin' && !process.mas && !features.isComponentBuild();

let identity: string | null;

export function getCodesignIdentity() {
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

/** The Electron.app under test. */
export function defaultAppBundlePath() {
  return path.resolve(process.execPath, '../../..');
}

// APFS clone (`cp -c`), falling back to a byte copy across volumes.
function cloneTree(source: string, destParent: string) {
  const dest = path.resolve(destParent, path.basename(source));
  let result = cp.spawnSync('cp', ['-cR', source, destParent]);
  if (result.status !== 0) {
    cp.spawnSync('rm', ['-rf', dest]);
    result = cp.spawnSync('cp', ['-R', source, destParent]);
  }
  if (result.status !== 0) {
    throw new Error(`Failed to copy ${source} to ${destParent}: ${result.stderr?.toString()}`);
  }
  return dest;
}

export type CopyAppOptions = {
  /** Bundle to copy. Defaults to the Electron.app under test. */
  sourceApp?: string;
  /** Replaces CFBundleIdentifier in the copy's Info.plist. */
  bundleId?: string;
  /** Appended to the fixture's package.json `name` (and so its userData dir). */
  appNameSuffix?: string;
};

export async function copyMacOSFixtureApp(
  newDir: string,
  fixture: string | null = 'initial',
  options: CopyAppOptions = {}
) {
  const newPath = cloneTree(options.sourceApp ?? defaultAppBundlePath(), newDir);
  if (fixture) {
    const appDir = path.resolve(newPath, 'Contents/Resources/app');
    await fs.promises.rm(appDir, { recursive: true, force: true });
    await fs.promises.mkdir(appDir, { recursive: true });
    await fs.promises.cp(path.resolve(fixturesPath, 'auto-update', fixture), appDir, { recursive: true });
    if (options.appNameSuffix) {
      const packageJsonPath = path.resolve(appDir, 'package.json');
      const packageJson = JSON.parse(await fs.promises.readFile(packageJsonPath, 'utf8'));
      packageJson.name = `${packageJson.name}${options.appNameSuffix}`;
      await fs.promises.writeFile(packageJsonPath, JSON.stringify(packageJson, null, 2));
    }
  }
  const plistPath = path.resolve(newPath, 'Contents', 'Info.plist');
  let plist = await fs.promises.readFile(plistPath, 'utf8');
  // The source may be a template that already went through here once.
  if (!plist.includes('<key>NSExceptionAllowsInsecureHTTPLoads</key>')) {
    plist = plist.replace(
      '<key>BuildMachineOSBuild</key>',
      `<key>NSAppTransportSecurity</key>
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
    </dict><key>BuildMachineOSBuild</key>`
    );
  }
  if (options.bundleId) {
    plist = plist.replace(/(<key>CFBundleIdentifier<\/key>\s*<string>)[^<]+/, `$1${options.bundleId}`);
  }
  await fs.promises.writeFile(plistPath, plist);
  return newPath;
}

export function spawn(cmd: string, args: string[], opts: any = {}) {
  let out = '';
  const child = cp.spawn(cmd, args, opts);
  child.stdout.on('data', (chunk: Buffer) => {
    out += chunk.toString();
  });
  child.stderr.on('data', (chunk: Buffer) => {
    out += chunk.toString();
  });
  return new Promise<{ code: number; out: string }>((resolve) => {
    child.on('exit', (code, signal) => {
      expect(signal).to.equal(null);
      resolve({
        code: code!,
        out
      });
    });
  });
}

export type SignAppOptions = {
  /**
   * Also sign nested code. A shallow sign is enough, and much cheaper, for a
   * clone of an already deep-signed bundle where only outer resources changed.
   */
  deep?: boolean;
};

export function signApp(appPath: string, identity: string, { deep = true }: SignAppOptions = {}) {
  return spawn('codesign', ['-s', identity, ...(deep ? ['--deep'] : []), '--force', appPath]);
}

export function unsignApp(appPath: string) {
  return spawn('codesign', ['--remove-signature', '--deep', appPath]);
}

/**
 * `strip -x` the Electron Framework binary. Testing builds carry a full symbol
 * table and every codesign/zip/ditto pass scales with it. Run before signing.
 */
export function stripFrameworkSymbols(appPath: string) {
  const frameworksDir = path.resolve(appPath, 'Contents', 'Frameworks');
  if (!fs.existsSync(frameworksDir)) return;
  for (const entry of fs.readdirSync(frameworksDir)) {
    if (!entry.endsWith(' Framework.framework')) continue;
    const name = entry.replace(/\.framework$/, '');
    const binary = path.resolve(frameworksDir, entry, 'Versions', 'A', name);
    if (!fs.existsSync(binary)) continue;
    const result = cp.spawnSync('strip', ['-x', binary]);
    if (result.status !== 0) {
      throw new Error(`strip -x failed for ${binary}: ${result.stderr?.toString()}`);
    }
  }
}

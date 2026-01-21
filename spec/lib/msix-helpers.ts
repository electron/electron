import { expect } from 'chai';

import * as cp from 'node:child_process';
import * as fs from 'node:fs';
import * as path from 'node:path';

const fixturesPath = path.resolve(__dirname, '..', 'fixtures', 'api', 'autoupdater', 'msix');
const manifestFixturePath = path.resolve(fixturesPath, 'ElectronDevAppxManifest.xml');
const installCertScriptPath = path.resolve(fixturesPath, 'install_test_cert.ps1');

// Install the signing certificate for MSIX test packages to the Trusted People store
// This is required to install self-signed MSIX packages
export async function installMsixCertificate (): Promise<void> {
  const result = cp.spawnSync('powershell', [
    '-NoProfile',
    '-ExecutionPolicy', 'Bypass',
    '-File', installCertScriptPath
  ]);

  if (result.status !== 0) {
    throw new Error(`Failed to install MSIX certificate: ${result.stderr.toString() || result.stdout.toString()}`);
  }
}

// Check if we should run MSIX tests
export const shouldRunMsixTests =
  process.platform === 'win32';

// Get the Electron executable path
export function getElectronExecutable (): string {
  return process.execPath;
}

// Get path to main.js fixture
export function getMainJsFixturePath (): string {
  return path.resolve(fixturesPath, 'main.js');
}

// Register executable with identity
export async function registerExecutableWithIdentity (executablePath: string): Promise<void> {
  if (!fs.existsSync(manifestFixturePath)) {
    throw new Error(`Manifest fixture not found: ${manifestFixturePath}`);
  }

  const executableDir = path.dirname(executablePath);
  const manifestPath = path.join(executableDir, 'AppxManifest.xml');
  const escapedManifestPath = manifestPath.replace(/'/g, "''").replace(/\\/g, '\\\\');
  const psCommand = `
    $ErrorActionPreference = 'Stop';
    try {
      Add-AppxPackage -Register '${escapedManifestPath}' -ForceUpdateFromAnyVersion
    } catch {
      Write-Error $_.Exception.Message
      exit 1
    }
  `;

  fs.copyFileSync(manifestFixturePath, manifestPath);

  const result = cp.spawnSync('powershell', ['-NoProfile', '-Command', psCommand]);
  if (result.status !== 0) {
    const errorMsg = result.stderr.toString() || result.stdout.toString();
    try {
      fs.unlinkSync(manifestPath);
    } catch {
      // Ignore cleanup errors
    }
    throw new Error(`Failed to register executable with identity: ${errorMsg}`);
  }
}

// Unregister the Electron Dev MSIX package
// This removes the sparse package registration created by registerExecutableWithIdentity
export async function unregisterExecutableWithIdentity (): Promise<void> {
  const result = cp.spawnSync('powershell', ['-NoProfile', '-Command', ' Get-AppxPackage Electron.Dev.MSIX | Remove-AppxPackage']);
  // Don't throw if package doesn't exist
  if (result.status !== 0) {
    throw new Error(`Failed to unregister executable with identity: ${result.stderr.toString() || result.stdout.toString()}`);
  }

  const electronExec = getElectronExecutable();
  const executableDir = path.dirname(electronExec);
  const manifestPath = path.join(executableDir, 'AppxManifest.xml');
  try {
    if (fs.existsSync(manifestPath)) {
      fs.unlinkSync(manifestPath);
    }
  } catch {
    // Ignore cleanup errors
  }
}

// Get path to MSIX fixture package
export function getMsixFixturePath (version: 'v1' | 'v2'): string {
  const filename = `HelloMSIX_${version}.msix`;
  return path.resolve(fixturesPath, filename);
}

// Install MSIX package
export async function installMsixPackage (msixPath: string): Promise<void> {
  // Use Add-AppxPackage PowerShell cmdlet
  const result = cp.spawnSync('powershell', [
    '-Command',
    `Add-AppxPackage -Path "${msixPath}" -ForceApplicationShutdown`
  ]);
  if (result.status !== 0) {
    throw new Error(`Failed to install MSIX package: ${result.stderr.toString()}`);
  }
}

// Uninstall MSIX package by  name
export async function uninstallMsixPackage (name: string): Promise<void> {
  const result = cp.spawnSync('powershell', ['-NoProfile', '-Command', `Get-AppxPackage ${name} | Remove-AppxPackage`]);
  // Don't throw if package doesn't exist
  if (result.status !== 0) {
    throw new Error(`Failed to uninstall MSIX package: ${result.stderr.toString() || result.stdout.toString()}`);
  }
}

// Get version of installed MSIX package by name
export async function getMsixPackageVersion (name: string): Promise<string | null> {
  const psCommand = `(Get-AppxPackage -Name '${name}').Version`;
  const result = cp.spawnSync('powershell', ['-NoProfile', '-Command', psCommand]);
  if (result.status !== 0) {
    return null;
  }
  const version = result.stdout.toString().trim();
  return version || null;
}

export function spawn (cmd: string, args: string[], opts: any = {}): Promise<{ code: number, out: string }> {
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
}

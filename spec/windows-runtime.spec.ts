import { expect } from 'chai';

import { spawnSync } from 'node:child_process';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';

import { copyApp } from './lib/fs-helpers.ts';
import { ifdescribe } from './lib/spec-helpers.ts';

// Inspect the linked images, including the forwarder RVA rather than only the
// names in the generated DEF file.
function readPeExports(file: string): Map<string, string | null> {
  const data = fs.readFileSync(file);
  expect(data.toString('ascii', 0, 2)).to.equal('MZ');
  const pe = data.readUInt32LE(0x3c);
  expect(data.toString('ascii', pe, pe + 4)).to.equal('PE\0\0');
  const sectionCount = data.readUInt16LE(pe + 6);
  const optionalSize = data.readUInt16LE(pe + 20);
  const optional = pe + 24;
  const magic = data.readUInt16LE(optional);
  expect([0x10b, 0x20b]).to.include(magic);
  const directories = optional + (magic === 0x10b ? 96 : 112);
  const exportRva = data.readUInt32LE(directories);
  const exportSize = data.readUInt32LE(directories + 4);
  expect(exportRva).to.be.greaterThan(0);

  const offsetFor = (rva: number): number => {
    for (let i = 0; i < sectionCount; i++) {
      const section = optional + optionalSize + i * 40;
      const start = data.readUInt32LE(section + 12);
      const size = data.readUInt32LE(section + 16);
      if (rva >= start && rva - start < size) {
        const offset = data.readUInt32LE(section + 20) + rva - start;
        expect(offset).to.be.lessThan(data.length);
        return offset;
      }
    }
    throw new Error(`Unmapped export RVA ${rva} in ${file}`);
  };
  const stringAt = (rva: number): string => {
    const start = offsetFor(rva);
    const end = data.indexOf(0, start);
    expect(end).to.be.at.least(start);
    return data.toString('ascii', start, end);
  };

  const directory = offsetFor(exportRva);
  const functions = offsetFor(data.readUInt32LE(directory + 28));
  const names = offsetFor(data.readUInt32LE(directory + 32));
  const ordinals = offsetFor(data.readUInt32LE(directory + 36));
  const exports = new Map<string, string | null>();
  for (let i = 0; i < data.readUInt32LE(directory + 24); i++) {
    const name = stringAt(data.readUInt32LE(names + i * 4));
    const ordinal = data.readUInt16LE(ordinals + i * 2);
    expect(ordinal).to.be.lessThan(data.readUInt32LE(directory + 20));
    const target = data.readUInt32LE(functions + ordinal * 4);
    expect(target, name).not.to.equal(0);
    exports.set(name, target >= exportRva && target - exportRva < exportSize ? stringAt(target) : null);
  }
  return exports;
}

ifdescribe(process.platform === 'win32')('Windows runtime DLL', function () {
  this.timeout(120000);

  it('preserves named runtime exports with explicit direct-export exceptions', () => {
    const runtimeExports = readPeExports(path.join(path.dirname(process.execPath), 'main.dll'));
    const executableExports = readPeExports(process.execPath);
    // Shared static dependencies also export these symbols directly from the EXE.
    // They may be direct exports or forwarders; all other symbols must forward.
    const allowedDirectExports = new Set([
      'Cr_z_adler32',
      'Cr_z_adler32_combine',
      'Cr_z_adler32_combine64',
      'Cr_z_adler32_z',
      'Cr_z_crc32',
      'Cr_z_crc32_combine',
      'Cr_z_crc32_combine64',
      'Cr_z_crc32_combine_gen',
      'Cr_z_crc32_combine_gen64',
      'Cr_z_crc32_combine_op',
      'Cr_z_crc32_z',
      'Cr_z_deflate',
      'Cr_z_deflateBound',
      'Cr_z_deflateBound_z',
      'Cr_z_deflateCopy',
      'Cr_z_deflateEnd',
      'Cr_z_deflateGetDictionary',
      'Cr_z_deflateInit2_',
      'Cr_z_deflateInit_',
      'Cr_z_deflateParams',
      'Cr_z_deflatePending',
      'Cr_z_deflatePrime',
      'Cr_z_deflateReset',
      'Cr_z_deflateResetKeep',
      'Cr_z_deflateSetDictionary',
      'Cr_z_deflateSetHeader',
      'Cr_z_deflateTune',
      'Cr_z_deflateUsed',
      'Cr_z_get_crc_table',
      'Cr_z_zError',
      'Cr_z_zlibCompileFlags',
      'Cr_z_zlibVersion',
      'GetHandleVerifier',
      'IsSandboxedProcess'
    ]);
    for (const prefix of ['node_', 'napi_', 'uv_']) {
      const names = (exports: Map<string, string | null>) =>
        [...exports.keys()].filter((name) => name.startsWith(prefix)).sort();
      expect(names(runtimeExports).length, prefix).to.be.greaterThan(0);
      expect(names(executableExports), prefix).to.deep.equal(names(runtimeExports));
    }
    for (const name of runtimeExports.keys()) {
      const target = executableExports.get(name);
      if (target === null) {
        expect(allowedDirectExports.has(name), `Unexpected direct export: ${name}`).to.equal(true);
      } else {
        expect(target, name).to.equal(`main.${name}`);
      }
    }
  });

  describe('copied application', function () {
    let tempDir: string;
    let executable: string;

    before(async () => {
      tempDir = fs.mkdtempSync(path.join(os.tmpdir(), 'electron-runtime-'));
      executable = await copyApp(tempDir);
    });

    after(() => {
      if (tempDir) fs.rmSync(tempDir, { recursive: true, force: true, maxRetries: 5 });
    });

    it('prints the version after loading the runtime', () => {
      const env = { ...process.env };
      delete env.ELECTRON_RUN_AS_NODE;
      delete env.ELECTRON_ENABLE_LOGGING;
      delete env.ELECTRON_LOG_FILE;
      env.NODE_OPTIONS = '';
      const result = spawnSync(executable, ['--version'], {
        env,
        encoding: 'utf8',
        timeout: 30000
      });
      expect(result.error).to.equal(undefined);
      expect(result.status, result.stderr).to.equal(0);
      expect(result.stdout.trim()).to.equal(`v${process.versions.electron}`);
    });

    it('reports a missing runtime DLL', () => {
      const directory = path.join(tempDir, 'missing-runtime');
      fs.mkdirSync(directory);
      const missingExecutable = path.join(directory, 'electron.exe');
      fs.copyFileSync(executable, missingExecutable);
      const result = spawnSync(missingExecutable, ['-v'], { encoding: 'utf8', timeout: 30000 });
      expect(result.error).to.equal(undefined);
      expect(result.status).to.equal(126); // ERROR_MOD_NOT_FOUND
      expect(result.stderr).to.include('Unable to load Electron runtime');
    });

    it('reports a missing runtime entry point', () => {
      const directory = path.join(tempDir, 'missing-entry-point');
      fs.mkdirSync(directory);
      const missingEntryExecutable = path.join(directory, 'electron.exe');
      fs.copyFileSync(executable, missingEntryExecutable);
      // Use a valid system DLL that does not export ElectronMain.
      fs.copyFileSync(path.join(process.env.SystemRoot!, 'System32', 'version.dll'), path.join(directory, 'main.dll'));
      const result = spawnSync(missingEntryExecutable, ['-v'], { encoding: 'utf8', timeout: 30000 });
      expect(result.error).to.equal(undefined);
      expect(result.status).to.equal(127); // ERROR_PROC_NOT_FOUND
      expect(result.stderr).to.include('Unable to find Electron runtime entry point');
    });
  });
});

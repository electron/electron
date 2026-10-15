import { getRawHeader } from '@electron/asar';
import { flipFuses, type FuseV1Config, FuseV1Options, FuseVersion } from '@electron/fuses';
import { resedit } from '@electron/packager/dist/resedit.js';

import { afterEach, beforeEach, describe, expect, it } from 'vitest';

import * as cp from 'node:child_process';
import * as nodeCrypto from 'node:crypto';
import * as fs from 'node:fs';
import * as originalFs from 'node:original-fs';
import * as os from 'node:os';
import * as path from 'node:path';

import { copyApp } from './lib/fs-helpers.ts';
import { ifdescribe } from './lib/spec-helpers.ts';

const bufferReplace = (haystack: Buffer, needle: string, replacement: string, throwOnMissing = true): Buffer => {
  const needleBuffer = Buffer.from(needle);
  const idx = haystack.indexOf(needleBuffer);
  if (idx === -1) {
    if (throwOnMissing) throw new Error(`Needle ${needle} not found in haystack`);
    return haystack;
  }

  const before = haystack.slice(0, idx);
  const after = bufferReplace(haystack.slice(idx + needleBuffer.length), needle, replacement, false);
  const len = idx + replacement.length + after.length;
  return Buffer.concat([before, Buffer.from(replacement), after], len);
};

type SpawnResult = { code: number | null; out: string; signal: NodeJS.Signals | null };
function spawn(cmd: string, args: string[], opts: any = {}) {
  let out = '';
  const child = cp.spawn(cmd, args, opts);
  child.stdout.on('data', (chunk: Buffer) => {
    out += chunk.toString();
  });
  child.stderr.on('data', (chunk: Buffer) => {
    out += chunk.toString();
  });
  return new Promise<SpawnResult>((resolve) => {
    child.on('close', (code, signal) => {
      resolve({
        code,
        signal,
        out
      });
    });
  });
}

const expectToHaveCrashed = (res: SpawnResult) => {
  if (process.platform === 'win32') {
    expect(res.code).not.toBe(0);
    expect(res.code).not.toBe(null);
    expect(res.signal).toBe(null);
  } else {
    expect(res.code).toBe(null);
    expect(['SIGABRT', 'SIGTRAP']).toContain(res.signal);
  }
};

describe('fuses', { timeout: 120000 }, () => {
  let tmpDir: string;
  let appPath: string;

  const launchApp = (args: string[] = []) => {
    if (process.platform === 'darwin') {
      return spawn(path.resolve(appPath, 'Contents/MacOS/Electron'), args);
    }
    return spawn(appPath, args);
  };

  const ensureFusesBeforeEach = (
    fuses: Omit<FuseV1Config<boolean>, 'version' | 'strictlyRequireAllFuses' | 'resetAdhocDarwinSignature'>
  ) => {
    beforeEach(async () => {
      await flipFuses(appPath, {
        version: FuseVersion.V1,
        resetAdHocDarwinSignature: true,
        ...fuses
      });
    }, 120000);
  };

  beforeEach(async () => {
    tmpDir = await fs.promises.mkdtemp(path.resolve(os.tmpdir(), 'electron-asar-integrity-spec-'));
    appPath = await copyApp(tmpDir);
  }, 120000);

  afterEach(async () => {
    for (let attempt = 0; attempt <= 3; attempt++) {
      // Sometimes windows holds on to a DLL during the crash for a little bit, so we try a few times to delete it
      if (attempt > 0) await new Promise((resolve) => setTimeout(resolve, 500 * attempt));
      try {
        await originalFs.promises.rm(tmpDir, { recursive: true });
        break;
      } catch {}
    }
  }, 120000);

  ifdescribe((process.platform === 'win32' && process.arch !== 'arm64') || process.platform === 'darwin')(
    'ASAR Integrity',
    () => {
      let pathToAsar: string;

      beforeEach(async () => {
        if (process.platform === 'darwin') {
          pathToAsar = path.resolve(appPath, 'Contents', 'Resources', 'default_app.asar');
        } else {
          pathToAsar = path.resolve(path.dirname(appPath), 'resources', 'default_app.asar');
        }

        if (process.platform === 'win32') {
          await resedit(appPath, {
            asarIntegrity: {
              'resources\\default_app.asar': {
                algorithm: 'SHA256',
                hash: nodeCrypto.createHash('sha256').update(getRawHeader(pathToAsar).headerString).digest('hex')
              }
            }
          });
        }
      }, 120000);

      describe('when enabled', () => {
        ensureFusesBeforeEach({
          [FuseV1Options.EnableEmbeddedAsarIntegrityValidation]: true
        });

        it('opens normally when unmodified', async () => {
          const res = await launchApp([path.resolve(import.meta.dirname, 'fixtures/apps/hello/hello.js')]);
          expect(res.code).toBe(0);
          expect(res.signal).toBe(null);
          expect(res.out).toContain('alive');
        });

        it('fatals if the integrity header does not match', async () => {
          const asar = await originalFs.promises.readFile(pathToAsar);
          // Ensure that the header still starts with the same thing, if build system
          // things result in the header changing we should update this test
          expect(asar.toString()).toContain('{"files":{"default_app.js"');
          await originalFs.promises.writeFile(
            pathToAsar,
            bufferReplace(asar, '{"files":{"default_app.js"', '{"files":{"default_oop.js"')
          );

          const res = await launchApp(['--version']);
          expectToHaveCrashed(res);
          expect(res.out).toContain('Integrity check failed for asar archive');
        });

        it('fatals if a loaded main process JS file does not match', async () => {
          const asar = await originalFs.promises.readFile(pathToAsar);
          // Ensure that the header still starts with the same thing, if build system
          // things result in the header changing we should update this test
          expect(asar.toString()).toContain('Invalid Usage');
          await originalFs.promises.writeFile(pathToAsar, bufferReplace(asar, 'Invalid Usage', 'VVValid Usage'));

          const res = await launchApp(['--version']);
          expect(res.code).toBe(1);
          expect(res.signal).toBe(null);
          expect(res.out).toContain('ASAR Integrity Violation: got a hash mismatch');
        });

        it('fatals if a renderer content file does not match', async () => {
          const asar = await originalFs.promises.readFile(pathToAsar);
          // Ensure that the header still starts with the same thing, if build system
          // things result in the header changing we should update this test
          expect(asar.toString()).toContain('require-trusted-types-for');
          await originalFs.promises.writeFile(
            pathToAsar,
            bufferReplace(asar, 'require-trusted-types-for', 'require-trusted-types-not')
          );

          const res = await launchApp();
          expectToHaveCrashed(res);
          expect(res.out).toContain('Failed to validate block while ending ASAR file stream');
        });
      });

      describe('when disabled', () => {
        ensureFusesBeforeEach({
          [FuseV1Options.EnableEmbeddedAsarIntegrityValidation]: false
        });

        it('does nothing if the integrity header does not match', async () => {
          const asar = await originalFs.promises.readFile(pathToAsar);
          // Ensure that the header still starts with the same thing, if build system
          // things result in the header changing we should update this test
          expect(asar.toString()).toContain('{"files":{"default_app.js"');
          await originalFs.promises.writeFile(
            pathToAsar,
            bufferReplace(asar, '{"files":{"default_app.js"', '{"files":{"default_oop.js"')
          );

          const res = await launchApp(['--version']);
          expect(res.code).toBe(0);
        });
      });
    }
  );
});

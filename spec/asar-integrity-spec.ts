import { createPackage, getRawHeader } from '@electron/asar';
import { flipFuses, FuseV1Config, FuseV1Options, FuseVersion } from '@electron/fuses';
import { resedit } from '@electron/packager/resedit';

import { expect } from 'chai';

import * as cp from 'node:child_process';
import * as nodeCrypto from 'node:crypto';
import * as fs from 'node:fs';
import * as originalFs from 'node:original-fs';
import * as os from 'node:os';
import * as path from 'node:path';

import { copyApp } from './lib/fs-helpers';
import { ifdescribe } from './lib/spec-helpers';

// eslint-disable-next-line @typescript-eslint/no-require-imports
const plist = require('plist');

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
    child.on('exit', (code, signal) => {
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
    expect(res.code).to.not.equal(0);
    expect(res.code).to.not.equal(null);
    expect(res.signal).to.equal(null);
  } else {
    expect(res.code).to.equal(null);
    expect(res.signal).to.be.oneOf(['SIGABRT', 'SIGTRAP']);
  }
};

describe('fuses', function () {
  this.timeout(120000);

  let tmpDir: string;
  let appPath: string;

  const launchApp = (args: string[] = [], opts: any = {}) => {
    if (process.platform === 'darwin') {
      // Several of these apps terminate abnormally on purpose; tell AppKit
      // not to save/restore window state for them so macOS does not show the
      // "unexpectedly quit while reopening windows" dialog on the next launch
      // of an app with the same bundle identifier.
      // (Only when a script/flag is given: with no arguments the default app
      // would otherwise treat 'YES' as the app path.)
      const macArgs = args.length > 0 ? [...args, '-ApplePersistenceIgnoreState', 'YES'] : args;
      return spawn(path.resolve(appPath, 'Contents/MacOS/Electron'), macArgs, opts);
    }
    return spawn(appPath, args, opts);
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
    });
  };

  beforeEach(async () => {
    tmpDir = await fs.promises.mkdtemp(path.resolve(os.tmpdir(), 'electron-asar-integrity-spec-'));
    appPath = await copyApp(tmpDir);
  });

  afterEach(async () => {
    for (let attempt = 0; attempt <= 3; attempt++) {
      // Sometimes windows holds on to a DLL during the crash for a little bit, so we try a few times to delete it
      if (attempt > 0) await new Promise((resolve) => setTimeout(resolve, 500 * attempt));
      try {
        await originalFs.promises.rm(tmpDir, { recursive: true });
        break;
      } catch {}
    }
  });

  // Layout of the archive used by fixtures/apps/asar-integrity-reads: a
  // multi-block entry (with a recognisable marker at the start of every 4MB
  // integrity block so tests can corrupt a specific block), an entry that is
  // exactly one block, and a small one.  Keep in sync with the fixture.
  const kIntegrityBlockSize = 4 * 1024 * 1024;
  const kMultiSize = 2 * kIntegrityBlockSize + 1024 * 1024 + 5;
  const patternByte = (i: number) => (i * 7 + (i >> 8)) & 0xff;
  const blockMarker = (n: number) => `ASARBLOCK${n}MAGIC!`;
  const buildIntegrityReadsAsar = async (dest: string) => {
    const src = await fs.promises.mkdtemp(path.resolve(os.tmpdir(), 'electron-asar-integrity-reads-src-'));
    const multi = Buffer.alloc(kMultiSize);
    for (let i = 0; i < kMultiSize; i++) multi[i] = patternByte(i);
    for (let n = 0; n * kIntegrityBlockSize < kMultiSize; n++) {
      multi.write(blockMarker(n), n * kIntegrityBlockSize, 'latin1');
    }
    const exact = Buffer.alloc(kIntegrityBlockSize);
    for (let i = 0; i < exact.length; i++) exact[i] = patternByte(i);
    const small = Buffer.alloc(1000);
    for (let i = 0; i < small.length; i++) small[i] = patternByte(i);
    await fs.promises.writeFile(path.join(src, 'multi.bin'), multi);
    await fs.promises.writeFile(path.join(src, 'exact.bin'), exact);
    await fs.promises.writeFile(path.join(src, 'small.bin'), small);
    await createPackage(src, dest);
    await fs.promises.rm(src, { recursive: true, force: true });
  };
  const headerHash = (asarPath: string) =>
    nodeCrypto.createHash('sha256').update(getRawHeader(asarPath).headerString).digest('hex');

  ifdescribe((process.platform === 'win32' && process.arch !== 'arm64') || process.platform === 'darwin')(
    'ASAR Integrity',
    () => {
      let pathToAsar: string;
      let pathToReadsAsar: string;

      beforeEach(async () => {
        let resourcesDir: string;
        if (process.platform === 'darwin') {
          resourcesDir = path.resolve(appPath, 'Contents', 'Resources');
        } else {
          resourcesDir = path.resolve(path.dirname(appPath), 'resources');
        }
        pathToAsar = path.resolve(resourcesDir, 'default_app.asar');
        pathToReadsAsar = path.resolve(resourcesDir, 'integrity-reads.asar');
        await buildIntegrityReadsAsar(pathToReadsAsar);

        // Register both archives in the app's integrity table.  This has to
        // happen before the fuses are flipped, which re-signs the app.
        if (process.platform === 'win32') {
          await resedit(appPath, {
            asarIntegrity: {
              'resources\\default_app.asar': {
                algorithm: 'SHA256',
                hash: headerHash(pathToAsar)
              },
              'resources\\integrity-reads.asar': {
                algorithm: 'SHA256',
                hash: headerHash(pathToReadsAsar)
              }
            }
          });
        } else {
          const infoPlistPath = path.resolve(appPath, 'Contents', 'Info.plist');
          const info = plist.parse(await fs.promises.readFile(infoPlistPath, 'utf8'));
          info.ElectronAsarIntegrity = {
            ...info.ElectronAsarIntegrity,
            'Resources/integrity-reads.asar': {
              algorithm: 'SHA256',
              hash: headerHash(pathToReadsAsar)
            }
          };
          await fs.promises.writeFile(infoPlistPath, plist.build(info));
        }
      });

      describe('when enabled', () => {
        ensureFusesBeforeEach({
          [FuseV1Options.EnableEmbeddedAsarIntegrityValidation]: true
        });

        it('opens normally when unmodified', async () => {
          const res = await launchApp([path.resolve(__dirname, 'fixtures/apps/hello/hello.js')]);
          expect(res.code).to.equal(0);
          expect(res.signal).to.equal(null);
          expect(res.out).to.include('alive');
        });

        it('fatals if the integrity header does not match', async () => {
          const asar = await originalFs.promises.readFile(pathToAsar);
          // Ensure that the header still starts with the same thing, if build system
          // things result in the header changing we should update this test
          expect(asar.toString()).to.contain('{"files":{"default_app.js"');
          await originalFs.promises.writeFile(
            pathToAsar,
            bufferReplace(asar, '{"files":{"default_app.js"', '{"files":{"default_oop.js"')
          );

          const res = await launchApp(['--version']);
          expectToHaveCrashed(res);
          expect(res.out).to.include('Integrity check failed for asar archive');
        });

        it('fatals if a loaded main process JS file does not match', async () => {
          const asar = await originalFs.promises.readFile(pathToAsar);
          // Ensure that the header still starts with the same thing, if build system
          // things result in the header changing we should update this test
          expect(asar.toString()).to.contain('Invalid Usage');
          await originalFs.promises.writeFile(pathToAsar, bufferReplace(asar, 'Invalid Usage', 'VVValid Usage'));

          const res = await launchApp(['--version']);
          expect(res.code).to.equal(1);
          expect(res.signal).to.equal(null);
          expect(res.out).to.include('ASAR Integrity Violation: got a hash mismatch');
        });

        it('fatals if a renderer content file does not match', async () => {
          const asar = await originalFs.promises.readFile(pathToAsar);
          // Ensure that the header still starts with the same thing, if build system
          // things result in the header changing we should update this test
          expect(asar.toString()).to.contain('require-trusted-types-for');
          await originalFs.promises.writeFile(
            pathToAsar,
            bufferReplace(asar, 'require-trusted-types-for', 'require-trusted-types-not')
          );

          const res = await launchApp();
          expectToHaveCrashed(res);
          expect(res.out).to.include('Failed to validate block while ending ASAR file stream');
        });

        const fdReadModes = ['fd', 'stream', 'handle', 'copy'];
        const fdReadsApp = path.resolve(__dirname, 'fixtures/apps/asar-fd-reads/main.js');

        for (const mode of fdReadModes) {
          it(`serves unmodified files through fs.open-style APIs (${mode})`, async () => {
            const res = await launchApp([fdReadsApp], { env: { ...process.env, ASAR_FD_READ_MODE: mode } });
            expect(res.out).to.include(`${mode}-read-ok`);
            expect(res.code).to.equal(0);
            expect(res.signal).to.equal(null);
          });

          it(`fatals if a file read through fs.open-style APIs does not match (${mode})`, async () => {
            const asar = await originalFs.promises.readFile(pathToAsar);
            expect(asar.toString()).to.contain('require-trusted-types-for');
            await originalFs.promises.writeFile(
              pathToAsar,
              bufferReplace(asar, 'require-trusted-types-for', 'require-trusted-types-not')
            );

            const res = await launchApp([fdReadsApp], { env: { ...process.env, ASAR_FD_READ_MODE: mode } });
            expect(res.code).to.equal(1);
            expect(res.signal).to.equal(null);
            expect(res.out).to.include('ASAR Integrity Violation: got a hash mismatch');
            expect(res.out).to.not.include(`${mode}-read-ok`);
          });
        }

        describe('block validated reads of a multi-block entry', () => {
          const readsApp = path.resolve(__dirname, 'fixtures/apps/asar-integrity-reads/main.js');
          const runReads = (mode: string) =>
            launchApp([readsApp], { env: { ...process.env, ASAR_INTEGRITY_READS_MODE: mode } });
          const corruptBlock = async (n: number) => {
            const asar = await originalFs.promises.readFile(pathToReadsAsar);
            const marker = blockMarker(n);
            expect(asar.indexOf(marker)).to.not.equal(-1);
            await originalFs.promises.writeFile(
              pathToReadsAsar,
              bufferReplace(asar, marker, `TAMPERED${n}MAGIC!!`.slice(0, marker.length))
            );
          };
          const expectViolation = (res: SpawnResult) => {
            expect(res.code).to.equal(1);
            expect(res.signal).to.equal(null);
            expect(res.out).to.include('ASAR Integrity Violation: got a hash mismatch');
          };

          it('serves streams, ranged reads, fds, handles, readv and copies correctly when unmodified', async () => {
            const res = await runReads('sweep');
            expect(res.out).to.include('sweep-streams-ok');
            expect(res.out).to.include('sweep-ranges-ok');
            expect(res.out).to.include('sweep-fd-ok');
            expect(res.out).to.include('sweep-handle-ok');
            expect(res.out).to.include('sweep-concurrent-ok');
            expect(res.out).to.include('sweep-copy-ok');
            expect(res.out).to.include('sweep-ok');
            expect(res.code).to.equal(0);
            expect(res.signal).to.equal(null);
          });

          for (const block of [0, 1, 2]) {
            it(`terminates a stream when it reaches corrupted block ${block}`, async () => {
              await corruptBlock(block);
              const res = await runReads('block-stream');
              expectViolation(res);
              expect(res.out).to.not.include('stream-ok');
              // Every block before the corrupted one streamed successfully...
              for (let n = 0; n < block; n++) expect(res.out).to.include(`reached-block-${n}`);
              // ...and no byte from the corrupted block was ever delivered.
              expect(res.out).to.not.include(`reached-block-${block}`);
            });
          }

          it('still serves ranges that only touch intact blocks when a later block is corrupted', async () => {
            await corruptBlock(2);
            const res = await runReads('range-block0');
            expect(res.out).to.include('range-block0-ok');
            expect(res.code).to.equal(0);
            expect(res.signal).to.equal(null);
          });

          it('fatals whole-file reads of an entry with a corrupted block', async () => {
            await corruptBlock(1);
            const res = await runReads('readfile');
            expectViolation(res);
            expect(res.out).to.not.include('readfile-ok');
          });

          it('detects a block that is corrupted in place after it was already read once', async () => {
            const res = await runReads('tamper-after-read');
            expect(res.out).to.include('tamper-after-read-A-ok');
            expect(res.out).to.not.include('tamper-after-read-B-unexpectedly-ok');
            expectViolation(res);
          });
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
          expect(asar.toString()).to.contain('{"files":{"default_app.js"');
          await originalFs.promises.writeFile(
            pathToAsar,
            bufferReplace(asar, '{"files":{"default_app.js"', '{"files":{"default_oop.js"')
          );

          const res = await launchApp(['--version']);
          expect(res.code).to.equal(0);
        });
      });
    }
  );
});

import * as cp from 'child_process';
import * as path from 'path';
import { safeStorage } from 'electron/main';
import { expect } from 'chai';
import { emittedOnce } from './events-helpers';
import { ifit } from 'spec/spec-helpers';

const { promises: fs } = require('fs');

describe('safeStorage module', () => {
  after(async () => {
    const pathToEncryptedString = path.resolve(__dirname, 'fixtures', 'api', 'safe-storage', 'encrypted.txt');
    await fs.unlink(pathToEncryptedString);
  });

  /* isEncryptionAvailable returns false in Linux when running in headless mode (on CI). Headless mode stops
   * Chrome from reaching the system's keyring or libsecret. When running the tests with config.store
   * set to basic-text, a nullptr is returned from chromium,  defaulting the available encryption to false.
   * Thus, we expect false here when the operating system is Linux.
   *
   * The Chromium codebase uses mocks to ensure that OS_Crypt.isEncryptionAvailable returns
   * true in headless mode- as they have ensured corect functionality there is no need to
   * test this on electron's side.
  */
  describe('SafeStorage.isEncryptionAvailable()', () => {
    ifit(process.platform !== 'linux')('should return true when encryption key is available (macOS, Windows)', () => {
      expect(safeStorage.isEncryptionAvailable()).to.equal(true);
    });

    ifit(process.platform === 'linux')('should return true when encryption key is available (Linux)', () => {
      expect(safeStorage.isEncryptionAvailable()).to.equal(false);
    });
  });

  describe('SafeStorage.encryptString()', () => {
    it('valid input should correctly encrypt string', () => {
      const plaintext = 'plaintext';
      const encrypted = safeStorage.encryptString(plaintext);
      expect(Buffer.isBuffer(encrypted)).to.equal(true);
    });

    it('UTF-16 characters can be encrypted', () => {
      const plaintext = '€ - utf symbol';
      const encrypted = safeStorage.encryptString(plaintext);
      expect(Buffer.isBuffer(encrypted)).to.equal(true);
    });
  });

  describe('SafeStorage.decryptString()', () => {
    it('valid input should correctly decrypt string', () => {
      const encrypted = safeStorage.encryptString('plaintext');
      expect(safeStorage.decryptString(encrypted)).to.equal('plaintext');
    });

    it('UTF-16 characters can be decrypted', () => {
      const plaintext = '€ - utf symbol';
      const encrypted = safeStorage.encryptString(plaintext);
      expect(safeStorage.decryptString(encrypted)).to.equal(plaintext);
    });

    it('unencrypted input should throw', () => {
      const plaintextBuffer = Buffer.from('I am unencoded!', 'utf-8');
      expect(() => {
        safeStorage.decryptString(plaintextBuffer);
      }).to.throw(Error);
    });

    it('non-buffer input should throw', () => {
      const notABuffer = {} as any;
      expect(() => {
        safeStorage.decryptString(notABuffer);
      }).to.throw(Error);
    });
  });

  /* This test cannot be run in Linux on headless mode, as it depends on the use of the system's keyring
   * or secret storage. These are stateful system libraries which can hurt tests by reducing isolation,
   * reducing speed and introducing flakiness due to their own bugs. Chromium does not recommend running
   * on Linux.
  */
  describe('safeStorage persists encryption key across app relaunch', () => {
    ifit(process.platform !== 'linux')('can decrypt after closing and reopening app', async () => {
      const fixturesPath = path.resolve(__dirname, 'fixtures');

      const encryptAppPath = path.join(fixturesPath, 'api', 'safe-storage', 'encrypt-app');
      const encryptAppProcess = cp.spawn(process.execPath, [encryptAppPath]);
      let stdout: string = '';
      encryptAppProcess.stderr.on('data', data => { stdout += data; });
      encryptAppProcess.stderr.on('data', data => { stdout += data; });

      // this setTimeout is left here for QOL in case we need to debug this test in the future
      setTimeout(() => { console.log(stdout); }, 3000);

      await emittedOnce(encryptAppProcess, 'exit');

      const appPath = path.join(fixturesPath, 'api', 'safe-storage', 'decrypt-app');
      const relaunchedAppProcess = cp.spawn(process.execPath, [appPath]);

      let output = '';
      relaunchedAppProcess.stdout.on('data', data => { output += data; });
      relaunchedAppProcess.stderr.on('data', data => { output += data; });

      const [code] = await emittedOnce(relaunchedAppProcess, 'exit');

      if (!output.includes('plaintext')) {
        console.log(code, output);
      }
      expect(output).to.include('plaintext');
    });
  });
});

import * as cp from 'child_process';
import * as path from 'path';
import { safeStorage } from 'electron/main';
import { expect } from 'chai';
import { emittedOnce } from './events-helpers';
import { ifdescribe } from './spec-helpers';
import * as fs from 'fs';

/* isEncryptionAvailable returns false in Linux when running CI due to a mocked dbus. This stops
* Chrome from reaching the system's keyring or libsecret. When running the tests with config.store
* set to basic-text, a nullptr is returned from chromium,  defaulting the available encryption to false.
*
* Because all encryption methods are gated by isEncryptionAvailable, the methods will never return the correct values
* when run on CI and linux.
*/

ifdescribe(process.platform !== 'linux')('safeStorage module', () => {
  after(async () => {
    const pathToEncryptedString = path.resolve(__dirname, 'fixtures', 'api', 'safe-storage', 'encrypted.txt');
    if (fs.existsSync(pathToEncryptedString)) {
      await fs.unlinkSync(pathToEncryptedString);
    }
  });

  describe('SafeStorage.isEncryptionAvailable()', () => {
    it('should return true when encryption key is available (macOS, Windows)', () => {
      expect(safeStorage.isEncryptionAvailable()).to.equal(true);
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
  describe('safeStorage persists encryption key across app relaunch', () => {
    it('can decrypt after closing and reopening app', async () => {
      const fixturesPath = path.resolve(__dirname, 'fixtures');

      const encryptAppPath = path.join(fixturesPath, 'api', 'safe-storage', 'encrypt-app');
      const encryptAppProcess = cp.spawn(process.execPath, [encryptAppPath]);
      let stdout: string = '';
      encryptAppProcess.stderr.on('data', data => { stdout += data; });
      encryptAppProcess.stderr.on('data', data => { stdout += data; });

      try {
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
      } catch (e) {
        console.log(stdout);
        throw e;
      }
    });
  });
});

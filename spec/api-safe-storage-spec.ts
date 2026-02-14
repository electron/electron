import { safeStorage } from 'electron/main';

import * as chai from 'chai';
import { expect } from 'chai';
import * as chaiAsPromised from 'chai-as-promised';

import * as cp from 'node:child_process';
import { once } from 'node:events';
import * as fs from 'node:fs';
import * as path from 'node:path';

import { ifdescribe } from './lib/spec-helpers';

chai.use(chaiAsPromised);

describe('safeStorage module', () => {
  before(() => {
    if (process.platform === 'linux') {
      safeStorage.setUsePlainTextEncryption(true);
    }
  });

  after(async () => {
    const pathToEncryptedString = path.resolve(__dirname, 'fixtures', 'api', 'safe-storage', 'encrypted.txt');
    if (fs.existsSync(pathToEncryptedString)) {
      await fs.promises.rm(pathToEncryptedString, { force: true, recursive: true });
    }
  });

  describe('SafeStorage.isEncryptionAvailable()', () => {
    it('should return true when encryption key is available (macOS, Windows)', () => {
      expect(safeStorage.isEncryptionAvailable()).to.equal(true);
    });
  });

  ifdescribe(process.platform === 'linux')('SafeStorage.getSelectedStorageBackend()', () => {
    it('should return a valid backend', () => {
      expect(safeStorage.getSelectedStorageBackend()).to.equal('basic_text');
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

  describe('SafeStorage.isAsyncEncryptionAvailable()', () => {
    it('should return true when async encryption is available', () => {
      expect(safeStorage.isAsyncEncryptionAvailable()).to.equal(true);
    });
  });

  describe('SafeStorage.encryptStringAsync()', () => {
    it('should return a promise', () => {
      const result = safeStorage.encryptStringAsync('plaintext');
      expect(result).to.be.a('promise');
    });

    it('valid input should correctly encrypt string', async () => {
      const plaintext = 'plaintext';
      const encrypted = await safeStorage.encryptStringAsync(plaintext);
      expect(Buffer.isBuffer(encrypted)).to.equal(true);
    });

    it('UTF-16 characters can be encrypted', async () => {
      const plaintext = '€ - utf symbol';
      const encrypted = await safeStorage.encryptStringAsync(plaintext);
      expect(Buffer.isBuffer(encrypted)).to.equal(true);
    });

    it('empty string can be encrypted', async () => {
      const plaintext = '';
      const encrypted = await safeStorage.encryptStringAsync(plaintext);
      expect(Buffer.isBuffer(encrypted)).to.equal(true);
    });

    it('long strings can be encrypted', async () => {
      const plaintext = 'a'.repeat(10000);
      const encrypted = await safeStorage.encryptStringAsync(plaintext);
      expect(Buffer.isBuffer(encrypted)).to.equal(true);
    });

    it('special characters can be encrypted', async () => {
      const plaintext = '!@#$%^&*()_+-=[]{}|;:\'",.<>?/\\`~\n\t\r';
      const encrypted = await safeStorage.encryptStringAsync(plaintext);
      expect(Buffer.isBuffer(encrypted)).to.equal(true);
    });
  });

  describe('SafeStorage.decryptStringAsync()', () => {
    it('should return a promise', () => {
      const encrypted = safeStorage.encryptString('plaintext');
      const result = safeStorage.decryptStringAsync(encrypted);
      expect(result).to.be.a('promise');
    });

    it('valid input should correctly decrypt string', async () => {
      const encrypted = await safeStorage.encryptStringAsync('plaintext');
      const decryptResult = await safeStorage.decryptStringAsync(encrypted);
      expect(decryptResult).to.have.property('result');
      expect(decryptResult).to.have.property('shouldReEncrypt');
      expect(decryptResult.result).to.equal('plaintext');
      expect(decryptResult.shouldReEncrypt).to.be.a('boolean');
    });

    it('UTF-16 characters can be decrypted', async () => {
      const plaintext = '€ - utf symbol';
      const encrypted = await safeStorage.encryptStringAsync(plaintext);
      const decryptResult = await safeStorage.decryptStringAsync(encrypted);
      expect(decryptResult.result).to.equal(plaintext);
    });

    it('empty string can be decrypted', async () => {
      const plaintext = '';
      const encrypted = await safeStorage.encryptStringAsync(plaintext);
      const decryptResult = await safeStorage.decryptStringAsync(encrypted);
      expect(decryptResult.result).to.equal(plaintext);
    });

    it('long strings can be decrypted', async () => {
      const plaintext = 'a'.repeat(10000);
      const encrypted = await safeStorage.encryptStringAsync(plaintext);
      const decryptResult = await safeStorage.decryptStringAsync(encrypted);
      expect(decryptResult.result).to.equal(plaintext);
    });

    it('special characters can be decrypted', async () => {
      const plaintext = '!@#$%^&*()_+-=[]{}|;:\'",.<>?/\\`~\n\t\r';
      const encrypted = await safeStorage.encryptStringAsync(plaintext);
      const decryptResult = await safeStorage.decryptStringAsync(encrypted);
      expect(decryptResult.result).to.equal(plaintext);
    });

    it('unencrypted input should reject', async () => {
      const plaintextBuffer = Buffer.from('I am unencoded!', 'utf-8');
      await expect(safeStorage.decryptStringAsync(plaintextBuffer)).to.be.rejectedWith(Error);
    });

    it('non-buffer input should reject', async () => {
      const notABuffer = {} as any;
      await expect(safeStorage.decryptStringAsync(notABuffer)).to.be.rejectedWith(Error);
    });

    it('can decrypt data encrypted with sync method', async () => {
      const plaintext = 'sync-to-async test';
      const encrypted = safeStorage.encryptString(plaintext);
      const decryptResult = await safeStorage.decryptStringAsync(encrypted);
      expect(decryptResult.result).to.equal(plaintext);
    });
  });

  describe('SafeStorage sync and async interoperability', () => {
    it('sync decrypt can handle async encrypted data', async () => {
      const plaintext = 'async-to-sync test';
      const encrypted = await safeStorage.encryptStringAsync(plaintext);
      const decrypted = safeStorage.decryptString(encrypted);
      expect(decrypted).to.equal(plaintext);
    });

    it('multiple concurrent async operations work correctly', async () => {
      const plaintexts = ['text1', 'text2', 'text3', 'text4', 'text5'];

      const encryptPromises = plaintexts.map(pt => safeStorage.encryptStringAsync(pt));
      const encryptedBuffers = await Promise.all(encryptPromises);

      const decryptPromises = encryptedBuffers.map(buf => safeStorage.decryptStringAsync(buf));
      const decryptResults = await Promise.all(decryptPromises);
      const decryptedTexts = decryptResults.map(result => result.result);

      expect(decryptedTexts).to.deep.equal(plaintexts);
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
        await once(encryptAppProcess, 'exit');

        const appPath = path.join(fixturesPath, 'api', 'safe-storage', 'decrypt-app');
        const relaunchedAppProcess = cp.spawn(process.execPath, [appPath]);

        let output = '';
        relaunchedAppProcess.stdout.on('data', data => { output += data; });
        relaunchedAppProcess.stderr.on('data', data => { output += data; });

        const [code] = await once(relaunchedAppProcess, 'exit');

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

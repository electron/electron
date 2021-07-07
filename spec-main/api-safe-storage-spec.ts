import { app, safeStorage } from 'electron/main';
import { expect } from 'chai';
import { emittedOnce } from './events-helpers';

import * as cp from 'child_process';
import * as path from 'path';

const { promises: fs } = require('fs');
// const fixtureDir = path.resolve(__dirname, 'fixtures', 'safe-storage', 'encryption.txt');
// const readFile = fs.readFile;
const writeFile = fs.writeFile;

describe('safeStorage module', () => {
  afterEach(() => {
    app.quit();
  });

  describe('SafeStorage.isEncryptionAvailable()', () => {
    it('should return true when encryption key is available', () => {
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

    it('unencrypted input should not throw', () => {
      const plaintextBuffer = Buffer.from('I am unencoded!', 'utf-8');
      expect(() => {
        safeStorage.decryptString(plaintextBuffer);
      }).not.to.throw(Error);
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
      // encrypt string; save original decryption for later
      const encrypted = safeStorage.encryptString('plaintext');
      expect(Buffer.isBuffer(encrypted)).to.equal(true);
      const decrypted = safeStorage.decryptString(encrypted);

      // encrypt the string and write it into fixtures. Quit app
      const encryptionPath = path.resolve(__dirname, 'fixtures', 'api', 'safe-storage', 'encrypted.txt');
      await writeFile(encryptionPath, encrypted);
      // app.quit();
      console.log('storage spec', encrypted.length);

      // spawn second app process to test
      const fixturesPath = path.resolve(__dirname, 'fixtures');
      const appPath = path.join(fixturesPath, 'api', 'safe-storage');
      console.log(' BEFORE SPaWN');
      const relaunchedAppProcess = cp.spawn(process.execPath, [appPath]);
      console.log('AFTER SPAWN');

      // grab new decrypted output to test
      let output = '';
      relaunchedAppProcess.stdout.on('data', data => { output += data; });
      relaunchedAppProcess.stderr.on('data', data => { output += data; });
      console.log(output);
      // force second app process to quit (hopefully)
      const [code] = await emittedOnce(relaunchedAppProcess, 'exit');

      // parse fixture output and compare to decrypted
      if (!output.includes(decrypted)) {
        console.log(code, output);
      }
      expect(output).to.include(decrypted);
    });
  });
});

// goals for pairing
// write this above test ^
// determine whether windows/mac unencrypted string should be able to decrypt or not
//     apparently windows and mac are giving different errors

import { app, safeStorage } from 'electron/main';
import { expect } from 'chai';
import { emittedOnce } from './events-helpers';

import * as cp from 'child_process';
import * as path from 'path';

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
      const fixturesPath = path.resolve(__dirname, 'fixtures');

      // spawn first app to encrypt string; save original decryption for later\
      const encryptAppPath = path.join(fixturesPath, 'api', 'safe-storage', 'encrypt-app');
      const encryptAppProcess = cp.spawn(process.execPath, [encryptAppPath]);

      // note for tomorrow: following commented code will be useful for debugging/ if we un-remove the dcheck
      // let output = '';
      // encryptAppProcess.stdout.on('data', data => { output += data; });
      // encryptAppProcess.stderr.on('data', data => { output += data; });
      // console.log(output);

      // force encryptor app process to quit
      // const [exitCode] = await emittedOnce(encryptAppProcess, 'exit');
      await emittedOnce(encryptAppProcess, 'exit');

      // spawn second app process
      const appPath = path.join(fixturesPath, 'api', 'safe-storage', 'decrypt-app');
      const relaunchedAppProcess = cp.spawn(process.execPath, [appPath]);

      // grab new decrypted output to test
      let output = '';
      relaunchedAppProcess.stdout.on('data', data => { output += data; });
      relaunchedAppProcess.stderr.on('data', data => { output += data; });

      // force second app process to quit (hopefully)
      const [code] = await emittedOnce(relaunchedAppProcess, 'exit');
      console.log('DECRYPTED, CODE, OUTPUT', code, output);
      // parse fixture output and compare to decrypted
      if (!output.includes('plaintext')) {
        console.log(code, output);
      }
      expect(output).to.include('plaintext');
    });
  });
});

// api-autoupdater-darwin-spec.ts for spawn code example, (refactor into new helper)
// issue caused by app.name resetting,
// in our spec file, add another spawn for writing
// wrote file wihitn test runner but read it within a spawn. if we write file in a spawn, read within child, we will have the same app name!

import { expect } from 'chai';
import { safeStorage } from 'electron/main';

describe('safeStorage module', () => {
  describe('SafeStorage.isEncryptionAvailable()', () => {
    it('should return true when encryption key is available', () => {
      expect(safeStorage.isEncryptionAvailable()).to.equal(true);
    });
  });

  describe('SafeStorage.encryptString()', () => {
    it('valid input should correctly encrypt string', () => {
      const plaintext = 'plaintext';
      expect(String(safeStorage.encryptString(plaintext))).to.equal('v10NT@�\u001f2�\u001f��2�j\u0011�L');
    });
  });

  describe('SafeStorage.decryptString()', () => {
    it('valid input should correctly decrypt string', () => {
      const encrypted = safeStorage.encryptString('plaintext');
      expect(safeStorage.decryptString(encrypted)).to.equal('plaintext');
    });
  });
});

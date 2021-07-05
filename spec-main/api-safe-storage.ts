import { expect } from 'chai';
import { SafeStorage } from 'electron';

describe('safeStorage module', () => {
  let safeStorage: SafeStorage;

  describe('SafeStorage.isEncryptionAvailable', () => {
    expect(safeStorage.isEncryptionAvailable()).to.equal(true);
  });
});

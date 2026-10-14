const { app, safeStorage } = require('electron');

const { expect } = require('../../../lib/vitest-cjs.cjs');

(async () => {
  if (!app.isReady()) {
    expect(safeStorage.isEncryptionAvailable()).toBe(false);

    expect(() => safeStorage.encryptString('plaintext')).toThrow(/safeStorage cannot be used before app is ready/);
    expect(() => safeStorage.decryptString(Buffer.from(''))).toThrow(/safeStorage cannot be used before app is ready/);
  }

  await app.whenReady();

  if (safeStorage.isEncryptionAvailable()) {
    const plaintext = 'plaintext';
    const ciphertext = safeStorage.encryptString(plaintext);
    expect(Buffer.isBuffer(ciphertext)).toBe(true);
    expect(safeStorage.decryptString(ciphertext)).toBe(plaintext);
  } else {
    expect(() => safeStorage.encryptString('plaintext')).toThrow(/Encryption is not available/);
    expect(() => safeStorage.decryptString(Buffer.from(''))).toThrow(/Decryption is not available/);
  }
})()
  .then(app.quit)
  .catch((err) => {
    console.error(err);
    app.exit(1);
  });

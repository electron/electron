const { app, safeStorage } = require('electron');

const { expect } = require('chai');

(async () => {
  if (!app.isReady()) {
    // isEncryptionAvailable() returns false before the app is ready on
    // Linux: https://github.com/electron/electron/issues/32206
    // and
    // Windows: https://github.com/electron/electron/issues/33640.
    expect(safeStorage.isEncryptionAvailable()).to.equal(process.platform === 'darwin');
    if (safeStorage.isEncryptionAvailable()) {
      const plaintext = 'plaintext';
      const ciphertext = safeStorage.encryptString(plaintext);
      expect(Buffer.isBuffer(ciphertext)).to.equal(true);
      expect(safeStorage.decryptString(ciphertext)).to.equal(plaintext);
    } else {
      expect(() => safeStorage.encryptString('plaintext')).to.throw(/safeStorage cannot be used before app is ready/);
      expect(() => safeStorage.decryptString(Buffer.from(''))).to.throw(/safeStorage cannot be used before app is ready/);
    }
  }
  await app.whenReady();
  // isEncryptionAvailable() will always return false on CI due to a mocked
  // dbus as mentioned above.
  expect(safeStorage.isEncryptionAvailable()).to.equal(process.platform !== 'linux');
  if (safeStorage.isEncryptionAvailable()) {
    const plaintext = 'plaintext';
    const ciphertext = safeStorage.encryptString(plaintext);
    expect(Buffer.isBuffer(ciphertext)).to.equal(true);
    expect(safeStorage.decryptString(ciphertext)).to.equal(plaintext);
  } else {
    expect(() => safeStorage.encryptString('plaintext')).to.throw(/Encryption is not available/);
    expect(() => safeStorage.decryptString(Buffer.from(''))).to.throw(/Decryption is not available/);
  }
})()
  .then(app.quit)
  .catch((err) => {
    console.error(err);
    app.exit(1);
  });

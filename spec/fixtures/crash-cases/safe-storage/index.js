const { app, safeStorage } = require('electron');
const { once } = require('node:events');

const { expect } = require('chai');

(async () => {
  if (!app.isReady()) {
    expect(safeStorage.isEncryptionAvailable()).to.equal(false);

    expect(() => safeStorage.encryptString('plaintext')).to.throw(/safeStorage cannot be used before app is ready/);
    expect(() => safeStorage.decryptString(Buffer.from(''))).to.throw(/safeStorage cannot be used before app is ready/);
  }

  await app.whenReady();
  await once(safeStorage, 'ready-to-use');

  // isEncryptionAvailable() is always false on CI due to mocked dbus.
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

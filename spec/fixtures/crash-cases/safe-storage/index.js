const { app, safeStorage } = require('electron');

const { expect } = require('chai');

async function expectRejection(promise, pattern) {
  let error;
  try {
    await promise;
  } catch (e) {
    error = e;
  }
  expect(error).to.be.an('error');
  expect(error.message).to.match(pattern);
}

(async () => {
  if (!app.isReady()) {
    expect(await safeStorage.isAsyncEncryptionAvailable()).to.equal(false);

    await expectRejection(
      safeStorage.encryptStringAsync('plaintext'),
      /safeStorage cannot be used before app is ready/
    );
    await expectRejection(
      safeStorage.decryptStringAsync(Buffer.from('')),
      /safeStorage cannot be used before app is ready/
    );
  }

  await app.whenReady();

  if (await safeStorage.isAsyncEncryptionAvailable()) {
    const plaintext = 'plaintext';
    const ciphertext = await safeStorage.encryptStringAsync(plaintext);
    expect(Buffer.isBuffer(ciphertext)).to.equal(true);
    expect((await safeStorage.decryptStringAsync(ciphertext)).result).to.equal(plaintext);
  }
})()
  .then(app.quit)
  .catch((err) => {
    console.error(err);
    app.exit(1);
  });

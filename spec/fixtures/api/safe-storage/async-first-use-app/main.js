const { app, safeStorage } = require('electron');

app.whenReady().then(async () => {
  try {
    if (process.platform === 'linux') {
      safeStorage.setUsePlainTextEncryption(true);
    }

    const isAsyncEncryptionAvailable =
      await safeStorage.isAsyncEncryptionAvailable();
    const encrypted = await safeStorage.encryptStringAsync('plaintext');
    const decrypted = await safeStorage.decryptStringAsync(encrypted);

    console.log(JSON.stringify({
      isAsyncEncryptionAvailable,
      encryptedIsBuffer: Buffer.isBuffer(encrypted),
      decrypted: decrypted.result
    }));
    app.quit();
  } catch (error) {
    console.error(error);
    app.exit(1);
  }
});

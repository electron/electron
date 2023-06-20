const { app, safeStorage } = require('electron');
const { promises: fs } = require('node:fs');
const path = require('node:path');

const pathToEncryptedString = path.resolve(__dirname, '..', 'encrypted.txt');
const writeFile = fs.writeFile;

app.whenReady().then(async () => {
  if (process.platform === 'linux') {
    safeStorage.setUsePlainTextEncryption(true);
  }
  const encrypted = safeStorage.encryptString('plaintext');
  await writeFile(pathToEncryptedString, encrypted);
  app.quit();
});

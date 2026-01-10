const { app, safeStorage } = require('electron');
const { once } = require('node:events');

const { promises: fs } = require('node:fs');
const path = require('node:path');

const pathToEncryptedString = path.resolve(__dirname, '..', 'encrypted.txt');
const writeFile = fs.writeFile;

app.whenReady().then(async () => {
  await once(safeStorage, 'ready-to-use');
  if (process.platform === 'linux') {
    safeStorage.setUsePlainTextEncryption(true);
  }
  const encrypted = safeStorage.encryptString('plaintext');
  await writeFile(pathToEncryptedString, encrypted);
  app.quit();
});

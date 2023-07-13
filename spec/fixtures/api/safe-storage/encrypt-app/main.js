const { app, safeStorage, ipcMain } = require('electron');
const { promises: fs } = require('fs');
const path = require('path');

const pathToEncryptedString = path.resolve(__dirname, '..', 'encrypted.txt');
const writeFile = fs.writeFile;

app.whenReady().then(async () => {
  if (process.platform === 'linux') {
    safeStorage.setUsePlainTextEncryption(true);
  }
  const encrypted = safeStorage.encryptString('plaintext');
  const encryptedString = await writeFile(pathToEncryptedString, encrypted);
  app.quit();
});

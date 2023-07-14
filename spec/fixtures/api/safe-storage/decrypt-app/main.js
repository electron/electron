const { app, safeStorage } = require('electron');
const { promises: fs } = require('node:fs');
const path = require('node:path');

const pathToEncryptedString = path.resolve(__dirname, '..', 'encrypted.txt');
const readFile = fs.readFile;

app.whenReady().then(async () => {
  if (process.platform === 'linux') {
    safeStorage.setUsePlainTextEncryption(true);
  }
  const encryptedString = await readFile(pathToEncryptedString);
  const decrypted = safeStorage.decryptString(encryptedString);
  console.log(decrypted);
  app.quit();
});

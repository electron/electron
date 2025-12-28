const { app, safeStorage } = require('electron');
const { once } = require('node:events');

const { promises: fs } = require('node:fs');
const path = require('node:path');

const pathToEncryptedString = path.resolve(__dirname, '..', 'encrypted.txt');
const readFile = fs.readFile;

app.whenReady().then(async () => {
  await once(safeStorage, 'ready-to-use');
  if (process.platform === 'linux') {
    safeStorage.setUsePlainTextEncryption(true);
  }
  const encryptedString = await readFile(pathToEncryptedString);
  const decrypted = safeStorage.decryptString(encryptedString);
  console.log(decrypted);
  app.quit();
});

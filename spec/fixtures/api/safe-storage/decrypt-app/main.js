const { app, safeStorage } = require('electron');

const { promises: fs } = require('node:fs');
const path = require('node:path');

const pathToEncryptedString = path.resolve(__dirname, '..', 'encrypted.txt');
const readFile = fs.readFile;

app.whenReady().then(async () => {
  const encryptedString = await readFile(pathToEncryptedString);
  const { result } = await safeStorage.decryptStringAsync(encryptedString);
  console.log(result);
  app.quit();
});

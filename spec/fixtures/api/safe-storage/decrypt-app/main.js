const { app, safeStorage, ipcMain } = require('electron');
const { promises: fs } = require('fs');
const path = require('path');

const pathToEncryptedString = path.resolve(__dirname, '..', 'encrypted.txt');
const readFile = fs.readFile;

app.whenReady().then(async () => {
  const encryptedString = await readFile(pathToEncryptedString);
  const decrypted = safeStorage.decryptString(encryptedString);
  console.log(decrypted);
  app.quit();
});

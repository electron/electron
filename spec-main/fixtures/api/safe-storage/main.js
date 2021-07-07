const { app, safeStorage, ipcMain } = require('electron');
const { promises: fs } = require('fs');
const path = require('path');

const fixtureDir = path.resolve(__dirname, 'encrypted.txt');
const readFile = fs.readFile;

app.whenReady().then(async () => {
  // not sure if we'll need to JSON.stringify this :thinking:
  const encryptedString = await readFile(fixtureDir);
  const available = safeStorage.isEncryptionAvailable();
  //   const buffer = Buffer.from(encryptedString, 'utf8')
  console.log('fixtures, ', encryptedString.length);
  const decrypted = safeStorage.decryptString(encryptedString);
  //   console.log(decrypted);

  available ? console.log(decrypted) : console.log('no');
  app.quit();
});

// app.whenReady().then(() => {
//     console.log('plaintext');
//     app.quit();
//   });

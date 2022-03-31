const { app } = require('electron');
const fs = require('fs');
const path = require('path');

app.whenReady().then(() => {
  console.log('started'); // ping parent
});

// non-existing user data folder should not break requestSingleInstanceLock()
// ref: https://github.com/electron/electron/issues/33547
const userDataFolder = path.join(app.getPath('home'), 'electron-userData');
fs.rmSync(userDataFolder, { force: true, recursive: true });
app.setPath('userData', userDataFolder);

const gotTheLock = app.requestSingleInstanceLock();

app.on('second-instance', (event, args, workingDirectory) => {
  setImmediate(() => {
    console.log(JSON.stringify(args));
    app.exit(0);
  });
});

if (!gotTheLock) {
  app.exit(1);
}

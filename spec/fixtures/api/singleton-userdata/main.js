const { app } = require('electron');

const fs = require('node:fs');
const path = require('node:path');

const userDataFolder = path.join(app.getPath('temp'), 'electron-test-singleton-userdata');

// non-existent user data folder should not break requestSingleInstanceLock()
// ref: https://github.com/electron/electron/issues/33547
fs.rmSync(userDataFolder, { recursive: true, force: true });

// set the user data path after clearing out old state and right before we use it
app.setPath('userData', userDataFolder);
const gotTheLock = app.requestSingleInstanceLock();

app.releaseSingleInstanceLock();
fs.rmSync(userDataFolder, { recursive: true, force: true });

app.exit(gotTheLock ? 0 : 1);

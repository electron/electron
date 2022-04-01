const { app } = require('electron');
const fs = require('fs');
const path = require('path');

// non-existing user data folder should not break requestSingleInstanceLock()
// ref: https://github.com/electron/electron/issues/33547
const userDataFolder = path.join(app.getPath('home'), 'electron-test-singleton-userdata');
fs.rmSync(userDataFolder, { force: true, recursive: true });
app.setPath('userData', userDataFolder);

const gotTheLock = app.requestSingleInstanceLock();
app.exit(gotTheLock ? 0 : 1);

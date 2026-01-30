const { app } = require('electron');

const fs = require('node:fs');
const path = require('node:path');

const userDataFolder = path.join(app.getPath('temp'), 'electron-test-singleton-userdata')
app.setPath('userData', userDataFolder);

// non-existent user data folder should not break requestSingleInstanceLock()
// ref: https://github.com/electron/electron/issues/33547
fs.rmSync(userDataFolder, { recursive: true, force: true });

const gotTheLock = app.requestSingleInstanceLock();
app.exit(gotTheLock ? 0 : 1);

fs.rmSync(userDataFolder, { recursive: true, force: true });
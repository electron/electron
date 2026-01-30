const { app } = require('electron');

const fs = require('node:fs');
const path = require('node:path');

const userDataFolder = path.join(app.getPath('temp'), 'electron-test-singleton-userdata');

function removeUserDataFolder() {
  try {
    fs.rmSync(userDataFolder, { recursive: true, force: true });
  } catch (e) {
    // ignore
  }
}

// non-existent user data folder should not break requestSingleInstanceLock()
// ref: https://github.com/electron/electron/issues/33547
removeUserDataFolder();

app.setPath('userData', userDataFolder);

const gotTheLock = app.requestSingleInstanceLock();
app.exit(gotTheLock ? 0 : 1);

removeUserDataFolder();

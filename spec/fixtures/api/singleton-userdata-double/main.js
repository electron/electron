const { app } = require('electron');
const fs = require('fs');
const path = require('path');

const hasSuffix = app.commandLine.hasSwitch('user-data-dir-suffix');

if (!hasSuffix) {
  app.exit(2);
}

const userDataDirSuffix = app.commandLine.getSwitchValue('user-data-dir-suffix');

const userDataFolder = path.join(app.getPath('home'), 'electron-test-singleton-userdata-double-' + userDataDirSuffix);
app.setPath('userData', userDataFolder);

const gotTheLock = app.requestSingleInstanceLock();

setTimeout(() => {
  app.exit(gotTheLock ? 0 : 1);
}, 500);

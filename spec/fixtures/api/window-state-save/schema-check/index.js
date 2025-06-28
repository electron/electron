const { app, BrowserWindow } = require('electron');

const os = require('node:os');
const path = require('node:path');

const sharedUserData = path.join(os.tmpdir(), 'electron-window-state-test');
app.setPath('userData', sharedUserData);

app.whenReady().then(() => {
  const w = new BrowserWindow({
    width: 400,
    height: 300,
    windowStateRestoreOptions: {
      stateId: 'test-window-state-schema'
    },
    show: false
  });

  w.close();

  setTimeout(() => {
    app.quit();
  }, 1000);
});

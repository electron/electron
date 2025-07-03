const { app, BrowserWindow } = require('electron');

const os = require('node:os');
const path = require('node:path');

const sharedUserData = path.join(os.tmpdir(), 'electron-window-state-test');
app.setPath('userData', sharedUserData);

app.whenReady().then(async () => {
  const w = new BrowserWindow({
    width: 400,
    height: 300,
    name: 'test-resize-save',
    windowStateRestoreOptions: true,
    show: false
  });

  w.setSize(500, 400);

  setTimeout(() => {
    app.quit();
  }, 1000);
});

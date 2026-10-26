const { app, BrowserWindow } = require('electron');

const os = require('node:os');
const path = require('node:path');

const sharedUserData = path.join(os.tmpdir(), 'electron-window-state-test');
app.setPath('userData', sharedUserData);

app.whenReady().then(() => {
  const w = new BrowserWindow({
    width: 400,
    height: 300,
    name: 'test-move-save',
    windowStatePersistence: true,
    show: false
  });

  w.setPosition(100, 150);

  setTimeout(() => {
    app.quit();
  }, 1000);
});

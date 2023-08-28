const { app, BrowserWindow } = require('electron');
const path = require('node:path');

app.whenReady().then(() => {
  const w = new BrowserWindow({
    show: false,
    icon: path.join(__dirname, 'icon.png')
  });
  w.webContents.on('did-finish-load', () => {
    app.quit();
  });
  w.loadURL('about:blank');
});

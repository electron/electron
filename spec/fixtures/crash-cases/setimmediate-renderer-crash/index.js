const { app, BrowserWindow } = require('electron');

const path = require('node:path');

app.whenReady().then(() => {
  const win = new BrowserWindow({
    show: false,
    webPreferences: {
      nodeIntegration: true,
      preload: path.resolve(__dirname, 'preload.js')
    }
  });

  win.loadURL('about:blank');

  win.webContents.on('render-process-gone', () => {
    process.exit(1);
  });

  win.webContents.on('did-finish-load', () => {
    setTimeout(() => app.quit());
  });
});

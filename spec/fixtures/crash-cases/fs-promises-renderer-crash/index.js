const { app, BrowserWindow } = require('electron');
const path = require('node:path');

app.whenReady().then(() => {
  let reloadCount = 0;
  const win = new BrowserWindow({
    show: false,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false
    }
  });

  win.loadFile('index.html');

  win.webContents.on('render-process-gone', () => {
    process.exit(1);
  });

  win.webContents.on('did-finish-load', () => {
    if (reloadCount > 2) {
      setImmediate(() => app.quit());
    } else {
      reloadCount += 1;
      win.webContents.send('reload', path.join(__dirname, '..', '..', 'cat.pdf'));
    }
  });
});

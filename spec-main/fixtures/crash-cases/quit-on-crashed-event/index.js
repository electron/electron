const { app, BrowserWindow } = require('electron');

app.once('ready', () => {
  const w = new BrowserWindow({
    show: false,
    webPreferences: {
      contextIsolation: false,
      nodeIntegration: true
    }
  });
  w.webContents.once('crashed', () => {
    app.quit();
  });
  w.webContents.loadURL('about:blank');
  w.webContents.executeJavaScript('process.crash()');
});

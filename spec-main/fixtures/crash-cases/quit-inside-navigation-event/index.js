const { app, BrowserWindow } = require('electron');

app.whenReady().then(() => {
  const mainWindow = new BrowserWindow();
  mainWindow.loadURL('about:blank');
  mainWindow.webContents.on('did-finish-load', () => {
    app.quit();
  });
});

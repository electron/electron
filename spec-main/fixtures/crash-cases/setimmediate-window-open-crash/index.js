const { app, BrowserWindow } = require('electron');

function createWindow () {
  const mainWindow = new BrowserWindow({
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false,
      nativeWindowOpen: true
    }
  });

  mainWindow.on('close', () => {
    app.quit();
  });

  mainWindow.loadFile('index.html');
}

app.whenReady().then(() => {
  createWindow();
});

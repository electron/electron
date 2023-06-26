// Modules to control application life and create native browser window
const { app, BrowserWindow, ipcMain } = require('electron');

function createWindow () {
  const mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    show: false,
    webPreferences: {
      nodeIntegration: true,
      nodeIntegrationInWorker: true,
      contextIsolation: false
    }
  });

  mainWindow.loadFile('index.html');
}

ipcMain.handle('module-paths', (e, success) => {
  process.exit(success ? 0 : 1);
});

app.whenReady().then(() => {
  createWindow();

  app.on('activate', function () {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') app.quit();
});

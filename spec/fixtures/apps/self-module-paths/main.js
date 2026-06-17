const { app, BrowserWindow, ipcMain } = require('electron');

async function createWindow() {
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

  await mainWindow.loadFile('index.html');
}

ipcMain.handle('module-paths', (e, success) => {
  process.exit(success ? 0 : 1);
});

app
  .whenReady()
  .then(createWindow)
  .catch(() => process.exit(1));

app.on('window-all-closed', app.quit);

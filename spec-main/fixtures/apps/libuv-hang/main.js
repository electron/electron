const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');

async function createWindow () {
  const mainWindow = new BrowserWindow({
    show: false,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js')
    }
  });

  await mainWindow.loadFile('index.html');
}

app.whenReady().then(() => {
  createWindow();
  app.on('activate', function () {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

let count = 0;
ipcMain.handle('reload-successful', () => {
  if (count === 2) {
    app.quit();
  } else {
    count++;
    return count;
  }
});

app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') app.quit();
});

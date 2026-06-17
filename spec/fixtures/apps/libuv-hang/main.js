const { app, BrowserWindow, ipcMain } = require('electron');

const path = require('node:path');

async function createWindow() {
  const mainWindow = new BrowserWindow({
    show: false,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      sandbox: false
    }
  });

  await mainWindow.loadFile('index.html');
}

app
  .whenReady()
  .then(createWindow)
  .catch(() => process.exit(1));

let count = 0;
ipcMain.handle('reload-successful', () => {
  if (count === 2) {
    app.quit();
  } else {
    count++;
    return count;
  }
});

app.on('window-all-closed', app.quit);

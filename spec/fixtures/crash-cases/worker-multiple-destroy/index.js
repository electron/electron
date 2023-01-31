const { app, BrowserWindow } = require('electron');

async function createWindow () {
  const mainWindow = new BrowserWindow({
    webPreferences: {
      nodeIntegrationInWorker: true
    }
  });

  let loads = 1;
  mainWindow.webContents.on('did-finish-load', async () => {
    if (loads === 2) {
      process.exit(0);
    } else {
      loads++;
      await mainWindow.webContents.executeJavaScript('addPaintWorklet()');
      await mainWindow.webContents.executeJavaScript('location.reload()');
    }
  });

  mainWindow.webContents.on('render-process-gone', () => {
    process.exit(1);
  });

  await mainWindow.loadFile('index.html');
}

app.whenReady().then(() => {
  createWindow();

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});

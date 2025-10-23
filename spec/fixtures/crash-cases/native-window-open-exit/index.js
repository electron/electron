const { app, ipcMain, BrowserWindow } = require('electron');

const http = require('node:http');
const path = require('node:path');

function createWindow () {
  const mainWindow = new BrowserWindow({
    webPreferences: {
      webSecurity: false,
      preload: path.join(__dirname, 'preload.js')
    }
  });

  mainWindow.loadFile('index.html');
  mainWindow.webContents.on('render-process-gone', () => {
    process.exit(1);
  });
}

const server = http.createServer((_req, res) => {
  res.end('<title>hello</title>');
}).listen(7001, '127.0.0.1');

app.whenReady().then(() => {
  createWindow();
  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') app.quit();
});

ipcMain.on('test-done', () => {
  console.log('test passed');
  server.close();
  process.exit(0);
});

const { app, ipcMain, BrowserWindow } = require('electron');

const http = require('node:http');
const path = require('node:path');

async function createWindow() {
  const mainWindow = new BrowserWindow({
    webPreferences: {
      webSecurity: false,
      preload: path.join(__dirname, 'preload.js')
    }
  });

  mainWindow.webContents.on('render-process-gone', () => {
    process.exit(1);
  });

  await mainWindow.loadFile('index.html');
}

const server = http
  .createServer((_req, res) => {
    res.end('<title>hello</title>');
  })
  .listen(7001, '127.0.0.1');

app
  .whenReady()
  .then(createWindow)
  .catch(() => process.exit(1));

app.on('window-all-closed', app.quit);

ipcMain.on('test-done', () => {
  console.log('test passed');
  server.close();
  process.exit(0);
});

const { app, BrowserWindow } = require('electron');
const net = require('net');
const path = require('path');

function createWindow () {
  const mainWindow = new BrowserWindow({
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false,
      nodeIntegrationInSubFrames: true,
      preload: path.resolve(__dirname, 'preload.js')
    }
  });

  mainWindow.loadFile('index.html');
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

const server = net.createServer((c) => {
  console.log('client connected');

  c.on('end', () => {
    console.log('client disconnected');
    app.quit();
  });

  c.write('hello\r\n');
  c.pipe(c);
});

server.on('error', (err) => {
  throw err;
});

server.listen('/tmp/echo.sock', () => {
  console.log('server bound');
});

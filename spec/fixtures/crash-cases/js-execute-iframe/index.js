const { app, BrowserWindow } = require('electron');

const net = require('node:net');
const path = require('node:path');

async function createWindow() {
  const mainWindow = new BrowserWindow({
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false,
      nodeIntegrationInSubFrames: true
    }
  });

  await mainWindow.loadFile('index.html');
}

app
  .whenReady()
  .then(createWindow)
  .catch(() => process.exit(1));

app.on('window-all-closed', app.quit);

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

const p = process.platform === 'win32' ? path.join('\\\\?\\pipe', process.cwd(), 'myctl') : '/tmp/echo.sock';

server.listen(p, () => {
  console.log('server bound');
});

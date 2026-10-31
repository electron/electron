const { app, BrowserWindow } = require('electron');

const net = require('node:net');
const os = require('node:os');
const path = require('node:path');

// Use a path of our own: a run that was killed leaves its socket behind, and a
// fixed path would make the next run's listen() fail with EADDRINUSE.
const socketPath =
  process.platform === 'win32'
    ? path.join('\\\\?\\pipe', process.cwd(), `myctl-${process.pid}`)
    : path.join(os.tmpdir(), `electron-js-execute-iframe-${process.pid}.sock`);

function createWindow() {
  const mainWindow = new BrowserWindow({
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false,
      nodeIntegrationInSubFrames: true
    }
  });

  mainWindow.loadFile('index.html', { query: { socketPath } });
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
  // Throwing here would show a modal error dialog and the app would never exit.
  console.error(err);
  app.exit(1);
});

server.listen(socketPath, () => {
  console.log('server bound');
});

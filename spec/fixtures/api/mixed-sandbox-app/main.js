const { app, BrowserWindow, ipcMain } = require('electron');

const net = require('node:net');
const path = require('node:path');

process.on('uncaughtException', () => {
  app.exit(1);
});

if (process.argv.includes('--app-enable-sandbox')) {
  app.enableSandbox();
}

let currentWindowSandboxed = false;

app.whenReady().then(() => {
  function testWindow (isSandboxed, callback) {
    currentWindowSandboxed = isSandboxed;
    const currentWindow = new BrowserWindow({
      show: false,
      webPreferences: {
        preload: path.join(__dirname, 'electron-app-mixed-sandbox-preload.js'),
        sandbox: isSandboxed
      }
    });
    currentWindow.loadURL('about:blank');
    currentWindow.webContents.once('devtools-opened', () => {
      if (isSandboxed) {
        argv.sandboxDevtools = true;
      } else {
        argv.noSandboxDevtools = true;
      }
      if (callback) {
        callback();
      }
      finish();
    });
    currentWindow.webContents.openDevTools();
  }

  const argv = {
    sandbox: null,
    noSandbox: null,
    sandboxDevtools: null,
    noSandboxDevtools: null
  };

  let connected = false;

  testWindow(true, () => {
    testWindow();
  });

  function finish () {
    if (connected && argv.sandbox != null && argv.noSandbox != null &&
        argv.noSandboxDevtools != null && argv.sandboxDevtools != null) {
      client.once('end', () => {
        app.exit(0);
      });
      client.end(JSON.stringify(argv));
    }
  }

  const socketPath = process.platform === 'win32' ? '\\\\.\\pipe\\electron-mixed-sandbox' : '/tmp/electron-mixed-sandbox';
  const client = net.connect(socketPath, () => {
    connected = true;
    finish();
  });

  ipcMain.on('argv', (event, value) => {
    if (currentWindowSandboxed) {
      argv.sandbox = value;
    } else {
      argv.noSandbox = value;
    }
    finish();
  });
});

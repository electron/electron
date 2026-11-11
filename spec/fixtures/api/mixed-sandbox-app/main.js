const { app, BrowserWindow } = require('electron');

const { once } = require('node:events');
const net = require('node:net');
const path = require('node:path');

process.on('uncaughtException', () => {
  app.exit(1);
});

if (process.argv.includes('--app-enable-sandbox')) {
  app.enableSandbox();
}

// Opens a window and its DevTools and resolves with the window's renderer
// argv. The argv is taken from that window's own IPC so that it can't be
// credited to the other window, whatever order the two windows report in.
async function testWindow(sandbox) {
  const w = new BrowserWindow({
    show: false,
    webPreferences: {
      preload: path.join(__dirname, 'electron-app-mixed-sandbox-preload.js'),
      sandbox
    }
  });
  const argv = once(w.webContents.ipc, 'argv').then(([, value]) => value);
  const devtoolsOpened = once(w.webContents, 'devtools-opened').then(() => true);
  w.loadURL('about:blank');
  w.webContents.openDevTools();
  const [value, devtools] = await Promise.all([argv, devtoolsOpened]);
  return { argv: value, devtools };
}

async function main() {
  const socketPath =
    process.platform === 'win32' ? '\\\\.\\pipe\\electron-mixed-sandbox' : '/tmp/electron-mixed-sandbox';
  const client = net.connect(socketPath);

  const [sandboxed, notSandboxed] = await Promise.all([testWindow(true), testWindow(false), once(client, 'connect')]);

  client.once('end', () => {
    app.exit(0);
  });
  client.end(
    JSON.stringify({
      sandbox: sandboxed.argv,
      noSandbox: notSandboxed.argv,
      sandboxDevtools: sandboxed.devtools,
      noSandboxDevtools: notSandboxed.devtools
    })
  );
}

app
  .whenReady()
  .then(main)
  .catch(() => {
    app.exit(1);
  });

const { app, BrowserWindow } = require('electron');

const net = require('node:net');

const socketPath = process.platform === 'win32' ? '\\\\.\\pipe\\electron-app-relaunch' : '/tmp/electron-app-relaunch';

process.on('uncaughtException', () => {
  app.exit(1);
});

app.whenReady().then(() => {
  const lastArg = process.argv[process.argv.length - 1];
  const win = new BrowserWindow({
    show: false,
    titleBarStyle: 'hidden'
  });

  win.loadURL('about:blank');

  const client = net.connect(socketPath);
  client.once('connect', () => {
    client.end(lastArg);
  });
  client.once('end', () => {
    if (lastArg === '--first') {
      app.relaunch({ args: process.argv.slice(1, -1).concat('--second') });
    }
    app.exit(0);
  });
});

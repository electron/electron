const { app, BrowserWindow } = require('electron');

process.on('uncaughtException', (err) => {
  console.error(err);
  process.exit(1);
});

process.on('unhandledRejection', (reason) => {
  console.error(reason);
  process.exit(1);
});

app.on('browser-window-created', (ev, window) => {
  window.webContents.once('did-frame-navigate', (ev, url) => {
    process.exit(0);
  });
});

app.whenReady().then(() => {
  const win = new BrowserWindow({ show: false });
  win.loadFile('index.html');
});
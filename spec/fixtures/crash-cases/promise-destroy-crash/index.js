const { app, BrowserWindow, shell } = require('electron');
const fs = require('node:fs');

fs.writeSync(2, '[pdc] fixture loaded\n');

let win = null;

function createWindow() {
  fs.writeSync(2, '[pdc] createWindow\n');
  win = new BrowserWindow({
    width: 800,
    height: 600,
    show: true,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false
    }
  });

  win.on('closed', () => fs.writeSync(2, '[pdc] window closed\n'));

  win.loadURL('data:text/html,<h1>repro</h1>');
}

async function createPromiseAndQuit() {
  const url = `unknownscheme-${Date.now()}://test`;
  const p = shell.openExternal(url, { activate: false });
  fs.writeSync(2, '[pdc] openExternal returned, promise pending\n');

  setTimeout(() => {
    fs.writeSync(2, '[pdc] calling app.quit()\n');
    app.quit();
  }, 0);

  p.then(() => {
    fs.writeSync(2, '[pdc] promise resolved.\n');
  }).catch(() => {
    fs.writeSync(2, '[pdc] promise rejected.\n');
  });
}

app.whenReady().then(() => {
  fs.writeSync(2, '[pdc] app ready\n');
  app.on('before-quit', () => fs.writeSync(2, '[pdc] before-quit\n'));
  app.on('will-quit', () => fs.writeSync(2, '[pdc] will-quit\n'));
  app.on('quit', () => fs.writeSync(2, '[pdc] quit\n'));

  createWindow();

  setTimeout(() => {
    createPromiseAndQuit();
  }, 500);
});

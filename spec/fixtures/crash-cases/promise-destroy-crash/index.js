const { app, BrowserWindow, shell } = require('electron');

console.error('[pdc] fixture loaded');

let win = null;

function createWindow() {
  console.error('[pdc] createWindow');
  win = new BrowserWindow({
    width: 800,
    height: 600,
    show: true,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false
    }
  });

  win.on('closed', () => console.error('[pdc] window closed'));

  win.loadURL('data:text/html,<h1>repro</h1>');
}

async function createPromiseAndQuit() {
  const url = `unknownscheme-${Date.now()}://test`;
  const p = shell.openExternal(url, { activate: false });
  console.error('[pdc] openExternal returned, promise pending');

  setTimeout(() => {
    console.error('[pdc] calling app.quit()');
    app.quit();
  }, 0);

  p.then(() => {
    console.error('[pdc] promise resolved.');
  }).catch(() => {
    console.error('[pdc] promise rejected.');
  });
}

app.whenReady().then(() => {
  console.error('[pdc] app ready');
  app.on('before-quit', () => console.error('[pdc] before-quit'));
  app.on('will-quit', () => console.error('[pdc] will-quit'));
  app.on('quit', () => console.error('[pdc] quit'));

  createWindow();

  setTimeout(() => {
    createPromiseAndQuit();
  }, 500);
});

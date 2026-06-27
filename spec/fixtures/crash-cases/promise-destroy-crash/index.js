const { app, BrowserWindow, shell } = require('electron');

let win = null;

function createWindow() {
  win = new BrowserWindow({
    width: 800,
    height: 600,
    show: true,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false
    }
  });

  win.on('closed', () => console.log('[pdc] window closed'));

  win.loadURL('data:text/html,<h1>repro</h1>');
}

async function createPromiseAndQuit() {
  const url = `unknownscheme-${Date.now()}://test`;
  const p = shell.openExternal(url, { activate: false });
  console.log('[pdc] openExternal returned, promise pending');

  setTimeout(() => {
    console.log('[pdc] calling app.quit()');
    app.quit();
  }, 0);

  p.then(() => {
    console.log('[pdc] promise resolved.');
  }).catch(() => {
    console.log('[pdc] promise rejected.');
  });
}

app.whenReady().then(() => {
  app.on('before-quit', () => console.log('[pdc] before-quit'));
  app.on('will-quit', () => console.log('[pdc] will-quit'));
  app.on('quit', () => console.log('[pdc] quit'));

  createWindow();

  setTimeout(() => {
    createPromiseAndQuit();
  }, 500);
});

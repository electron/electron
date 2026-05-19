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

  win.loadURL('data:text/html,<h1>repro</h1>');
}

async function createPromiseAndQuit() {
  const url = `unknownscheme-${Date.now()}://test`;
  const p = shell.openExternal(url, { activate: false });

  setTimeout(() => {
    app.quit();
  }, 0);

  p.then(() => {
    console.log('promise resolved.');
  }).catch(() => {
    console.log('promise rejected.');
  });
}

app.whenReady().then(() => {
  createWindow();

  setTimeout(() => {
    createPromiseAndQuit();
  }, 500);
});

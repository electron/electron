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

async function triggerOpenPathAndQuit() {
  const p = shell.openExternal('http://127.0.0.1', { activate: false });

  setTimeout(() => {
    app.quit();
  }, 0);

  p.then((result) => {
    console.log('openPath resolved, result =', JSON.stringify(result));
  }).catch((err) => {
    console.log('openPath rejected, err =', String(err));
  });
}

app.whenReady().then(() => {
  createWindow();

  setTimeout(() => {
    triggerOpenPathAndQuit();
  }, 500);
});

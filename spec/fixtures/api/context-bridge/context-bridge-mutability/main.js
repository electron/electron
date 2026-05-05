const { app, BrowserWindow } = require('electron');

const path = require('node:path');

let win;
app
  .whenReady()
  .then(async function () {
    win = new BrowserWindow({
      webPreferences: {
        contextIsolation: true,
        preload: path.join(__dirname, 'preload.js')
      }
    });

    win.webContents.on('console-message', (event) => {
      console.log(event.message);
    });

    win.webContents.on('did-finish-load', app.quit);

    await win.loadFile('index.html');
  })
  .catch(() => process.exit(1));

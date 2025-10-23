const { app, BrowserWindow } = require('electron');

const path = require('node:path');

let win;
app.whenReady().then(function () {
  win = new BrowserWindow({
    webPreferences: {
      contextIsolation: true,
      preload: path.join(__dirname, 'preload.js')
    }
  });

  win.loadFile('index.html');

  win.webContents.on('console-message', (event) => {
    console.log(event.message);
  });

  win.webContents.on('did-finish-load', () => app.quit());
});

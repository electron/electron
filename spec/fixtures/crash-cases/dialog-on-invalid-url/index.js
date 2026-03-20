const { app, BrowserWindow } = require('electron');

app.whenReady().then(() => {
  const win = new BrowserWindow({ show: false });
  win.loadFile('index.html');
  setTimeout(() => {
    process.exit(0);
  }, 1000);
});
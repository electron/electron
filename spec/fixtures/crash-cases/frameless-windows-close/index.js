const { app, BrowserWindow } = require('electron');

if (process.platform !== 'win32') {
  process.exit(0);
}

app.on('window-all-closed', () => {
  app.quit();
});

app.whenReady().then(() => {
  let win = new BrowserWindow({
    frame: false,
    show: true
  });

  win.loadURL('about:blank');

  setTimeout(() => {
    win.close();
  }, 5000);

  win.on('closed', () => {
    win = null;
  });
});

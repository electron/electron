const { app, BrowserWindow } = require('electron');

app.on('web-contents-created', (wc) => {
  throw new Error();
});

app.whenReady().then(async () => {
  const mainWindow = new BrowserWindow({ show: false });
  await mainWindow.loadURL('about:blank');
  process.exit(0);
});

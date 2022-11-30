const { app, BrowserWindow } = require('electron');

app.whenReady().then(() => {
  const mainWindow = new BrowserWindow({
    show: false
  });
  mainWindow.loadFile('about:blank');

  app.on('web-contents-created', () => {
    throw new Error();
  });

  app.quit();
});

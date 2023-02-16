const { app, webContents } = require('electron');
app.whenReady().then(function () {
  webContents.create();

  app.quit();
});

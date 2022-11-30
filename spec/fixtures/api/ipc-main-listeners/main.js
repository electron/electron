const { app, ipcMain } = require('electron');

app.whenReady().then(() => {
  process.stdout.write(JSON.stringify(ipcMain.eventNames()));
  process.stdout.end();

  app.quit();
});

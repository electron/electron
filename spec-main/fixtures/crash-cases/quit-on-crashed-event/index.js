const { app, BrowserWindow } = require('electron');

app.once('ready', async () => {
  const w = new BrowserWindow({
    show: false,
    webPreferences: {
      contextIsolation: false,
      nodeIntegration: true
    }
  });
  w.webContents.on('render-process-gone', (_, details) => {
    if (details.reason === 'crashed') {
      process.exit(0);
    } else {
      process.exit(details.exitCode);
    }
  });
  await w.webContents.loadURL('about:blank');
  console.log(await w.webContents.executeJavaScript('42'));
  w.webContents.executeJavaScript('process.crash()');
});

// Navigating a regular window to a non-chrome:// URL whose host happens to
// match a WebUI page must not be treated as a WebUI navigation.
const { app, BrowserWindow } = require('electron');

app.whenReady().then(async () => {
  const w = new BrowserWindow({ show: false });
  await w.loadURL('about:blank');
  for (const url of ['http://accessibility/', 'http://devtools/']) {
    await w.webContents.executeJavaScript(`location.href = ${JSON.stringify(url)}; undefined`);
    await new Promise((resolve) => {
      w.webContents.once('did-fail-load', resolve);
      w.webContents.once('did-finish-load', resolve);
    });
  }
  app.quit();
});

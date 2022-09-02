const { app, BrowserWindow } = require('electron');

app.whenReady().then(() => {
  const w = new BrowserWindow({ show: false, webPreferences: { webviewTag: true } });
  w.loadURL('data:text/html,<webview src="data:text/html,hi"></webview>');
  app.on('web-contents-created', () => {
    w.close();
  });
});

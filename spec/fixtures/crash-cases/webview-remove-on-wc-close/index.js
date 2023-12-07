const { app, BrowserWindow } = require('electron');

app.whenReady().then(() => {
  const win = new BrowserWindow({
    webPreferences: {
      webviewTag: true
    }
  });

  win.loadFile('index.html');

  win.webContents.on('did-attach-webview', (event, contents) => {
    contents.on('render-process-gone', () => {
      process.exit(1);
    });

    contents.on('destroyed', () => {
      process.exit(0);
    });

    contents.on('did-finish-load', () => {
      win.webContents.executeJavaScript('closeBtn.click()');
    });

    contents.on('will-prevent-unload', event => {
      event.preventDefault();
    });
  });
});

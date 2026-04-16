const { app, BrowserWindow, webContents } = require('electron');

app.whenReady().then(() => {
  const w = new BrowserWindow({
    show: false,
    webPreferences: {
      webviewTag: true,
      nodeIntegration: true,
      contextIsolation: false
    }
  });

  w.loadURL('data:text/html,' + encodeURIComponent(
    '<webview id="wv" src="about:blank" style="width:100px;height:100px"></webview>' +
    '<script>document.getElementById("wv").addEventListener("dom-ready", function() {' +
    '  document.title = "READY:" + this.getWebContentsId();' +
    '});</script>'
  ));

  const check = setInterval(() => {
    if (!w.getTitle().startsWith('READY:')) return;
    clearInterval(check);
    const guestId = parseInt(w.getTitle().split(':')[1]);
    const guest = webContents.fromId(guestId);
    if (!guest) {
      app.quit();
      return;
    }

    // Destroying a guest webContents inside an event handler previously caused
    // a use-after-free because Destroy() deleted the C++ object synchronously
    // while the event emission was still on the call stack.
    guest.on('did-navigate-in-page', () => {
      guest.destroy();
    });

    guest.executeJavaScript('history.pushState({}, "", "#trigger")');

    setTimeout(() => app.quit(), 2000);
  }, 200);
});

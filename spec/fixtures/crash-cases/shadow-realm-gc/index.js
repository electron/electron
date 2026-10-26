const { app, BrowserWindow } = require('electron');

app.commandLine.appendSwitch('js-flags', '--expose-gc --harmony-shadow-realm');

app
  .whenReady()
  .then(async () => {
    const window = new BrowserWindow({ show: false });
    window.webContents.on('render-process-gone', () => app.exit(1));
    await window.loadURL('about:blank');
    await window.webContents.executeJavaScript(`
    const collect = 'gc({ type: "major", execution: "sync", flavor: "last-resort" })';
    gc({ type: 'major', execution: 'sync', flavor: 'last-resort' });
    const realm = new ShadowRealm();
    realm.evaluate(collect);
    realm.evaluate('new ShadowRealm().evaluate(' + JSON.stringify(collect) + ')');
  `);
    app.quit();
  })
  .catch((error) => {
    console.error(error);
    app.exit(1);
  });

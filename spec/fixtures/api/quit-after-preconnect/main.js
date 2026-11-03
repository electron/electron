const { app, BrowserWindow } = require('electron');

const { once } = require('node:events');

app
  .whenReady()
  .then(async () => {
    const window = new BrowserWindow({ show: false });
    const target = process.argv.includes('--session-preconnect')
      ? window.webContents.session
      : window.webContents.mainFrame;
    const preconnect = once(target, 'preconnect');
    await window.loadURL('data:text/html,<link rel="preconnect" href="http://127.0.0.1:12345">');
    await preconnect;
    console.log('preconnect observed');
    app.quit();
  })
  .catch((error) => {
    console.error(error);
    app.exit(1);
  });

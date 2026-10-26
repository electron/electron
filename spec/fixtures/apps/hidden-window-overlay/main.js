// Creates a hidden window with a titleBarOverlay and reports whether
// `ready-to-show` fires. See https://github.com/electron/electron/issues/54025.
const { app, BrowserWindow } = require('electron');

app.whenReady().then(() => {
  const w = new BrowserWindow({
    show: false,
    width: 400,
    height: 400,
    titleBarStyle: 'hidden',
    titleBarOverlay: { height: 40 }
  });
  const timeout = setTimeout(() => {
    console.log('no ready-to-show after 10s');
    app.exit(1);
  }, 10000);
  w.once('ready-to-show', () => {
    clearTimeout(timeout);
    console.log('ready-to-show');
    app.exit(0);
  });
  w.loadURL('data:text/html,<h1>hello</h1>');
});

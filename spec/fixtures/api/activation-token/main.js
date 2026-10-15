const { app, BrowserWindow } = require('electron');

const { once } = require('node:events');

app.whenReady().then(async () => {
  const w = new BrowserWindow({ width: 300, height: 200 });
  // A window that has painted has been configured by the compositor, so its
  // own first activation is no longer pending.
  const painted = once(w, 'ready-to-show');
  w.loadURL('about:blank');
  await painted;

  app.setActivationToken('electron-test-token');
  w.focus();
  setTimeout(() => app.quit(), 1000);
});

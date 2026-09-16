const { app, globalShortcut, session, WebContentsView } = require('electron');

app.whenReady().then(() => {
  const view = new WebContentsView();
  globalThis.view = view;
  view.webContents.once('destroyed', () => {
    session.fromPartition('created-at-exit');
    globalShortcut.isRegistered('Alt+F9');
  });
  app.quit();
});

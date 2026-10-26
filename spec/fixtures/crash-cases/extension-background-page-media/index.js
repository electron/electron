// An extension background page using navigator.mediaDevices must not crash
// the main process.
const { app, session, webContents } = require('electron');

const { once } = require('node:events');
const path = require('node:path');

app.whenReady().then(async () => {
  const bgPageLogged = once(app, 'web-contents-created').then(([, wc]) => once(wc, 'console-message'));
  await session.defaultSession.extensions.loadExtension(path.join(__dirname, 'extension'));
  await Promise.race([bgPageLogged, new Promise((resolve) => setTimeout(resolve, 5000))]);
  if (!webContents.getAllWebContents().some((wc) => wc.getType() === 'backgroundPage')) process.exitCode = 1;
  app.quit();
});

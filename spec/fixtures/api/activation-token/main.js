const { app, BrowserWindow } = require('electron');

// Stays open. The spec reads the window's activation request from the Wayland
// protocol log and then ends this process.
app.whenReady().then(() => {
  app.setActivationToken('electron-test-token');
  const w = new BrowserWindow({ width: 300, height: 200 });
  w.once('closed', () => app.quit());
});

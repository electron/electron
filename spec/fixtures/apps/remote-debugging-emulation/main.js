const { app, BrowserWindow, Menu } = require('electron');

app.whenReady().then(() => {
  // No menu bar, so the web contents size is a stable test baseline.
  Menu.setApplicationMenu(null);
  const win = new BrowserWindow({ width: 600, height: 400, show: true });
  win.loadURL('data:text/html,<title>remote-debugging-emulation</title>');
});

app.on('window-all-closed', () => app.quit());

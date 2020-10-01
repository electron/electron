const { app, BrowserWindow } = require('electron');

app.on('ready', async () => {
  const win = new BrowserWindow({ show: false, webPreferences: { partition: '123321' } });
  await win.loadURL('data:text/html,<body>');
  setTimeout(() => app.quit());
});

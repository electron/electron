const { app, BrowserWindow } = require('electron');

app.on('web-contents-created', () => {
  throw new Error();
});

function createWindow () {
  const mainWindow = new BrowserWindow({
    width: 800,
    height: 600
  });

  mainWindow.loadFile('about:blank');
}

app.whenReady().then(() => {
  createWindow();

  setTimeout(() => app.quit());
  process.exit(0);
});

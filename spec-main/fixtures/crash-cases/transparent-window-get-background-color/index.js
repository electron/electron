const { app, BrowserWindow } = require('electron');

function createWindow () {
  // Create the browser window.
  const mainWindow = new BrowserWindow({
    transparent: true
  });
  mainWindow.getBackgroundColor();
}

app.on('ready', () => {
  createWindow();
  setTimeout(() => app.quit());
});

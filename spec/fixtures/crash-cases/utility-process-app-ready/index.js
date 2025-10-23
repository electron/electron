const { app, BrowserWindow, utilityProcess } = require('electron');

const path = require('node:path');

function createWindow () {
  const mainWindow = new BrowserWindow();
  mainWindow.loadFile('about:blank');
}

app.whenReady().then(() => {
  createWindow();

  app.on('activate', function () {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') app.quit();
});

try {
  utilityProcess.fork(path.join(__dirname, 'utility.js'));
} catch (e) {
  if (/utilityProcess cannot be created before app is ready/.test(e.message)) {
    app.exit(0);
  } else {
    console.error(e);
    app.exit(1);
  }
}

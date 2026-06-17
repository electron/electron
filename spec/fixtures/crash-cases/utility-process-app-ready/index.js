const { app, BrowserWindow, utilityProcess } = require('electron');

const path = require('node:path');

async function createWindow() {
  const mainWindow = new BrowserWindow();
  await mainWindow.loadFile('about:blank');
}

app
  .whenReady()
  .then(createWindow)
  .catch(() => process.exit(1));

app.on('window-all-closed', app.quit);

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

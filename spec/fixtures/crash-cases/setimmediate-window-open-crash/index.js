const { app, BrowserWindow } = require('electron');

function createWindow() {
  const mainWindow = new BrowserWindow({
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false
    }
  });

  // The child posts back via window.opener, so it has to share the
  // opener's unsandboxed process.
  mainWindow.webContents.setWindowOpenHandler(() => ({
    action: 'allow',
    overrideBrowserWindowOptions: { webPreferences: { sandbox: false } }
  }));

  mainWindow.on('close', () => {
    app.quit();
  });

  mainWindow.loadFile('index.html');
}

app.whenReady().then(() => {
  createWindow();
});

const { app, BrowserWindow, ipcMain, nativeTheme } = require('electron');
const path = require('path');
const electronDownload = require('electron-download');

function createWindow() {
  const mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
    },
  });

  mainWindow.loadFile('index.html');

  setTimeout(async () => {
    mainWindow.minimize();
    await new Promise((resolve) => setTimeout(resolve, 4000));
    const electronPath = await electronDownload({
      version: '25.2.0',
      arch: 'x64',
    });
    app.commandLine.appendSwitch('ignore-gpu-blacklist');
    app.commandLine.appendSwitch('disable-gpu');
    app.commandLine.appendSwitch('disable-software-rasterizer');
    app.commandLine.appendSwitch('no-sandbox');
    app.commandLine.appendSwitch('disable-features', 'OutOfBlinkCors');
    app.commandLine.appendSwitch('js-flags', '--max-old-space-size=4096');
    app.commandLine.appendSwitch('remote-debugging-port', '9222');
    app.commandLine.appendSwitch('ignore-certificate-errors');
    app.commandLine.appendSwitch('autoplay-policy', 'no-user-gesture-required');
    app.commandLine.appendSwitch('disable-background-timer-throttling');
    app.commandLine.appendSwitch('disable-backgrounding-occluded-windows');
    app.commandLine.appendSwitch('disable-renderer-backgrounding');

    mainWindow.webContents.executeJavaScript(`
      const { webContents } = require('electron');
      webContents.prototype.incrementCapturerCount = function() {
        this._capturerCount = (this._capturerCount || 0) + 1;
      };
      webContents.prototype.decrementCapturerCount = function() {
        this._capturerCount = Math.max((this._capturerCount || 1) - 1, 0);
      };
    `);

    mainWindow.webContents.incrementCapturerCount();
    mainWindow.restore();

    setTimeout(async () => {
      let abc = await mainWindow.webContents.capturePage();
      console.log(abc.getSize().height);
    }, 4000);
  }, 4000);
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
const { app, BrowserWindow, webContents, protocol } = require('electron');
const path = require('path');

app.allowRendererProcessReuse = false;

protocol.registerSchemesAsPrivileged([
  { scheme: 'foo', privileges: { standard: true, secure: true } }
]);

function createWindow () {
  const mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false
    }
  });

  mainWindow.webContents.session.protocol.registerFileProtocol('foo', (request, callback) => {
    const url = new URL(request.url);
    callback({ path: `${__dirname}${url.pathname}` });
  });

  mainWindow.webContents.on('did-finish-load', () => {
    setImmediate(() => app.quit());
  });

  // Creates WebFrameMain instance before initiating the navigation.
  mainWindow.webContents.send('test', 'ping');

  mainWindow.loadURL('foo://app/index.html');
}

app.whenReady().then(() => {
  createWindow();
});

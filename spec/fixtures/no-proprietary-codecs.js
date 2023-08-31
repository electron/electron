// Verifies that Electron cannot play a video that uses proprietary codecs
//
// This application should be run with the ffmpeg that does not include
// proprietary codecs to ensure Electron uses it instead of the version
// that does include proprietary codecs.

const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('node:path');

const MEDIA_ERR_SRC_NOT_SUPPORTED = 4;
const FIVE_MINUTES = 5 * 60 * 1000;

let window;

app.whenReady().then(() => {
  window = new BrowserWindow({
    show: false,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false
    }
  });

  window.webContents.on('render-process-gone', (event, details) => {
    console.log(`WebContents crashed ${JSON.stringify(details)}`);
    app.exit(1);
  });

  window.loadFile(path.resolve(__dirname, 'test.asar', 'video.asar', 'index.html'));

  ipcMain.on('asar-video', (event, message, error) => {
    if (message === 'ended') {
      console.log('Video played, proprietary codecs are included');
      app.exit(1);
      return;
    }

    if (message === 'error' && error === MEDIA_ERR_SRC_NOT_SUPPORTED) {
      app.exit(0);
      return;
    }

    console.log(`Unexpected response from page: ${message} ${error}`);
    app.exit(1);
  });

  setTimeout(() => {
    console.log('No IPC message after 5 minutes');
    app.exit(1);
  }, FIVE_MINUTES);
});

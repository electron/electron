const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');

let mainWindow;
const sidebarWidth = 250;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, 'src', 'preload.js'),
      contextIsolation: false,
      nodeIntegration: true
    }
  });

  mainWindow.loadFile('src/index.html');
}

app.on('ready', createWindow);

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow();
  }
});

ipcMain.on('resize-window', (event, { side, action }) => {
  try {
    const [currentWidth, currentHeight] = mainWindow.getSize();
    let targetWidth = action === 'open' ? currentWidth + sidebarWidth : currentWidth - sidebarWidth;

    function resize() {
      const [width, height] = mainWindow.getSize();
      if ((action === 'open' && width < targetWidth) || (action === 'close' && width > targetWidth)) {
        mainWindow.setSize(width + (action === 'open' ? 10 : -10), height);
        requestAnimationFrame(resize);
      } else {
        mainWindow.setSize(targetWidth, height);
      }
    }

    resize();
  } catch (error) {
    console.error('Error resizing window:', error);
  }
});

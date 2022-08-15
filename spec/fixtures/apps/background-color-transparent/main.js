const { app, BrowserWindow, desktopCapturer, ipcMain } = require('electron');
const getColors = require('get-image-colors');

const colors = {};

// Fetch the test window.
const getWindow = async () => {
  const sources = await desktopCapturer.getSources({ types: ['window'] });
  const filtered = sources.filter(s => s.name === 'test-color-window');

  if (filtered.length === 0) {
    throw new Error('Could not find test window');
  }

  return filtered[0];
};

async function createWindow () {
  const mainWindow = new BrowserWindow({
    frame: false,
    transparent: true,
    vibrancy: 'under-window',
    webPreferences: {
      backgroundThrottling: false,
      contextIsolation: false,
      nodeIntegration: true
    }
  });

  await mainWindow.loadFile('index.html');

  // Get initial green background color.
  const window = await getWindow();
  const buf = window.thumbnail.toPNG();
  const result = await getColors(buf, { count: 1, type: 'image/png' });
  colors.green = result[0].hex();
}

ipcMain.on('set-transparent', async () => {
  // Get updated background color.
  const window = await getWindow();
  const buf = window.thumbnail.toPNG();
  const result = await getColors(buf, { count: 1, type: 'image/png' });
  colors.transparent = result[0].hex();

  const { green, transparent } = colors;
  console.log({ green, transparent });
  process.exit(green === transparent ? 1 : 0);
});

app.whenReady().then(() => {
  createWindow();

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});

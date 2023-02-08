const { app, BrowserWindow } = require('electron');
const path = require('path');

async function createWindow () {
  const mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    x: 100,
    y: 100,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: false,
      nodeIntegration: true
    }
  });

  await mainWindow.loadFile('index.html');

  const rect = await mainWindow.webContents.executeJavaScript('JSON.parse(JSON.stringify(document.querySelector("a").getBoundingClientRect()))');
  const x = rect.x + rect.width / 2;
  const y = rect.y + rect.height / 2;

  function click (x, y, options) {
    x = Math.floor(x);
    y = Math.floor(y);
    mainWindow.webContents.sendInputEvent({
      type: 'mouseDown',
      button: 'left',
      x,
      y,
      clickCount: 1,
      ...options
    });

    mainWindow.webContents.sendInputEvent({
      type: 'mouseUp',
      button: 'left',
      x,
      y,
      clickCount: 1,
      ...options
    });
  }

  click(x, y, { modifiers: ['shift'] });
}

app.whenReady().then(() => {
  app.on('web-contents-created', (e, wc) => {
    wc.on('render-process-gone', (e, details) => {
      console.error(details);
      process.exit(1);
    });

    wc.on('did-finish-load', () => {
      const title = wc.getTitle();
      if (title === 'Window From Link') {
        process.exit(0);
      }
    });
  });

  createWindow();
});

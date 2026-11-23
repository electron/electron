const { app, BrowserWindow, ipcMain } = require('electron');

const { once } = require('node:events');
const path = require('node:path');
const { setTimeout } = require('node:timers/promises');

// Blink cancels an in-flight spellcheck request when the selection changes
// without a user gesture, then sends the next one.
app
  .whenReady()
  .then(async () => {
    const win = new BrowserWindow({
      show: false,
      webPreferences: {
        nodeIntegration: true,
        contextIsolation: false
      }
    });

    win.webContents.on('render-process-gone', () => {
      process.exit(1);
    });

    const waitForRequest = (count) =>
      new Promise((resolve) => {
        const listener = (_event, n) => {
          if (n >= count) {
            ipcMain.off('spell-check-request', listener);
            resolve();
          }
        };
        ipcMain.on('spell-check-request', listener);
      });

    const typeText = async (text) => {
      for (const keyCode of text) {
        win.webContents.sendInputEvent({ type: 'char', keyCode });
        await setTimeout(30);
      }
    };

    const timeout = setTimeout(10000).then(() => {
      throw new Error('Timed out waiting for overlapping spellcheck requests');
    });

    await win.loadFile(path.join(__dirname, 'index.html'));
    win.focus();
    await win.webContents.executeJavaScript('document.querySelector("input").focus()', true);

    const firstRequest = waitForRequest(1);
    await typeText('first ');
    await Promise.race([firstRequest, timeout]);

    const secondRequest = waitForRequest(2);
    await typeText('second ');
    await Promise.race([secondRequest, timeout]);

    const flushed = once(ipcMain, 'flushed');
    win.webContents.send('flush');
    const [, value] = await flushed;
    if (value !== 'first second ') {
      throw new Error(`Unexpected input value: ${value}`);
    }

    app.quit();
  })
  .catch((error) => {
    console.error(error);
    process.exit(2);
  });

import { app, BrowserWindow } from 'electron';

import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

async function createWindow() {
  const mainWindow = new BrowserWindow({
    show: false,
    webPreferences: {
      preload: fileURLToPath(new URL('preload.mjs', import.meta.url)),
      sandbox: false,
      contextIsolation: false
    }
  });

  await mainWindow.loadFile('index.html');

  const importMetaPreload = await mainWindow.webContents.executeJavaScript('window.importMetaPath');
  const expected = join(dirname(fileURLToPath(import.meta.url)), 'preload.mjs');

  process.exit(importMetaPreload === expected ? 0 : 1);
}

app
  .whenReady()
  .then(() => createWindow())
  .catch(() => process.exit(1));

app.on('window-all-closed', app.quit);

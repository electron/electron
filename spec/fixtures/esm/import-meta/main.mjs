import { app, BrowserWindow } from 'electron';

import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

// DO-NOT-MERGE: #51462 instrumentation. The hang is between WILL and DID.
const tag = `[import-meta ${process.pid}]`;
process.stderr.write(`${tag} TOP\n`);
app.on('will-finish-launching', () => process.stderr.write(`${tag} WILL\n`));
app.on('ready', () => process.stderr.write(`${tag} DID\n`));

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

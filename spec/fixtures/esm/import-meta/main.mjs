import { app, BrowserWindow } from 'electron';

import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const log = (msg) => process.stderr.write(`[import-meta:${process.pid}] ${msg}\n`);

process.on('uncaughtException', (err, origin) => {
  log(`uncaughtException origin=${origin} name=${err && err.name} message=${err && err.message}`);
  if (err && err.stack) log(`uncaughtException.stack=${err.stack}`);
});
process.on('unhandledRejection', (reason, _) => {
  const r = reason && (reason.stack || reason.message || String(reason));
  log(`unhandledRejection reason=${r}`);
});
process.on('warning', (warning) => {
  log(`warning name=${warning.name} message=${warning.message}`);
  if (warning.stack) log(`warning.stack=${warning.stack}`);
});

const v8Util = process._linkedBinding('electron_common_v8_util');
const wait = (ms) => new Promise((r) => setTimeout(r, ms));
const gc = async (ms = 50) => {
  v8Util.requestGarbageCollectionForTesting();
  await wait(ms);
};

log('module-top');

async function createWindow() {
  log('createWindow:start');
  await gc();
  const mainWindow = new BrowserWindow({
    show: false,
    webPreferences: {
      preload: fileURLToPath(new URL('preload.mjs', import.meta.url)),
      sandbox: false,
      contextIsolation: false
    }
  });
  log('createWindow:bw-created');
  await gc();

  const loadFilePromise = mainWindow.loadFile('index.html');
  await gc();
  await loadFilePromise;
  log('createWindow:loadFile-done');
  await gc();

  const execPromise = mainWindow.webContents.executeJavaScript('window.importMetaPath');
  await gc();
  const importMetaPreload = await execPromise;
  log('createWindow:executeJavaScript-done');
  await gc();
  const expected = join(dirname(fileURLToPath(import.meta.url)), 'preload.mjs');

  process.exit(importMetaPreload === expected ? 0 : 1);
}

app
  .whenReady()
  .then(() => createWindow())
  .catch(() => process.exit(1));

app.on('window-all-closed', app.quit);

import { app, BrowserWindow } from 'electron'
import { fileURLToPath } from 'node:url'
import { dirname, join } from 'node:path';

async function createWindow() {
  const mainWindow = new BrowserWindow({
    show: false,
    webPreferences: {
      preload: fileURLToPath(new URL('preload.mjs', import.meta.url)),
      sandbox: false,
      contextIsolation: false
    }
  })

  await mainWindow.loadFile('index.html')

  const importMetaPreload = await mainWindow.webContents.executeJavaScript('window.importMetaPath');
  const expected = join(dirname(fileURLToPath(import.meta.url)), 'preload.mjs');

  process.exit(importMetaPreload === expected ? 0 : 1);
}

app.whenReady().then(() => {
  createWindow()

  app.on('activate', function () {
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})

app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') app.quit()
})

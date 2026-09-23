import { app, BrowserWindow, ipcMain, utilityProcess } from 'electron/main';

import { once } from 'node:events';
import * as path from 'node:path';

import { compareModules } from './compare.mjs';

process.on('uncaughtException', (error) => {
  console.error(error);
  process.exit(1);
});

async function compareInUtilityProcess() {
  const child = utilityProcess.fork(path.join(import.meta.dirname, 'utility.mjs'));
  const [problems] = await once(child, 'message');
  child.kill();
  return problems;
}

async function compareInRendererProcess() {
  const window = new BrowserWindow({
    show: false,
    webPreferences: {
      preload: path.join(import.meta.dirname, 'preload.mjs'),
      sandbox: false,
      contextIsolation: false
    }
  });
  const result = once(ipcMain, 'parity');
  await window.loadFile(path.join(import.meta.dirname, '..', 'empty.html'));
  const [, problems] = await result;
  window.destroy();
  return problems;
}

// Electron holds the 'ready' event until the entry point module has finished
// evaluating, so only the main process comparison can use top-level await.
const results = {
  main: await compareModules(['electron', 'electron/main', 'electron/common'], import.meta.url)
};

app.whenReady().then(async () => {
  results.utility = await compareInUtilityProcess();
  results.renderer = await compareInRendererProcess();

  console.log(JSON.stringify(results));
  app.exit(Object.values(results).flat().length === 0 ? 0 : 1);
});

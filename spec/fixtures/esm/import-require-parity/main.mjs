import { app, BrowserWindow, ipcMain, utilityProcess } from 'electron/main';

import { once } from 'node:events';
import * as path from 'node:path';

import { compareModules } from './compare.mjs';

// Exit codes 0 and 1 tell the spec that every comparison ran,
// 1 meaning that some found a difference.
// Anything that keeps a comparison from finishing
// exits with 2 and the error on stderr,
// so the spec reports it at once
// instead of timing out on an app that still waits for a child's result.
function fail(error) {
  console.error(error);
  process.exit(2);
}
process.on('uncaughtException', fail);

// Progress on stderr shows where a run that the spec had to kill was stuck.
function step(name) {
  console.error(`[import-require-parity +${Math.round(performance.now())}ms] ${name}`);
}

async function compareInUtilityProcess() {
  const child = utilityProcess.fork(path.join(import.meta.dirname, 'utility.mjs'));
  const exited = once(child, 'exit');
  const [problems] = await Promise.race([
    once(child, 'message'),
    exited.then(([code]) => {
      throw new Error(`The utility process exited with code ${code} before reporting`);
    })
  ]);
  step('utility process compared');
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
  const failed = new Promise((resolve, reject) => {
    window.webContents.once('preload-error', (event, preloadPath, error) => reject(error));
    window.webContents.once('render-process-gone', (event, { reason }) => {
      reject(new Error(`The renderer process is gone (${reason}) before reporting`));
    });
  });
  const result = once(ipcMain, 'parity');
  const loaded = window.loadFile(path.join(import.meta.dirname, '..', 'empty.html'));
  const [[, problems]] = await Promise.race([Promise.all([result, loaded]), failed]);
  step('renderer process compared');
  window.destroy();
  return problems;
}

// Electron holds the 'ready' event until the entry point module has finished
// evaluating, so only the main process comparison can use top-level await.
const results = {
  main: await compareModules(['electron', 'electron/main', 'electron/common'], import.meta.url)
};
step('main process compared');

app
  .whenReady()
  .then(async () => {
    results.utility = await compareInUtilityProcess();
    results.renderer = await compareInRendererProcess();

    console.log(JSON.stringify(results));
    app.exit(Object.values(results).flat().length === 0 ? 0 : 1);
  })
  .catch(fail);

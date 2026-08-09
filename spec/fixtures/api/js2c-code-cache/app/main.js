// Spawned by the js2c-code-cache spec; collects per-process code-cache
// status for every flavor and prints `JS2C_RESULT <json>`.
const { app, BrowserWindow, ipcMain, utilityProcess } = require('electron');
const path = require('node:path');

const FIX = __dirname;
const PRELOAD = path.join(FIX, '..', 'report-preload.js');
const UTIL_CHILD = path.join(FIX, '..', 'util-child.js');
const os = require('node:os');
app.setPath('userData', path.join(os.tmpdir(), 'js2c-cc-ud-' + process.pid));
const v8Util = process._linkedBinding('electron_common_v8_util');

app.on('window-all-closed', () => {});

function withTimeout(p, what, ms = 20000) {
  return Promise.race([
    p,
    new Promise((_resolve, reject) => setTimeout(() => reject(new Error('timeout: ' + what)), ms))
  ]);
}

async function rendererStatus(sandbox) {
  const channel = sandbox ? 'js2c-sandbox' : 'js2c-renderer';
  const w = new BrowserWindow({
    show: false,
    webPreferences: {
      sandbox,
      contextIsolation: true,
      preload: PRELOAD,
      nodeIntegration: false,
      additionalArguments: ['--js2c-cc-channel=' + channel]
    }
  });
  try {
    return await withTimeout(
      new Promise((resolve) => {
        ipcMain.once(channel, (_e, s) => resolve(s));
        w.loadURL('about:blank');
      }),
      channel
    );
  } finally {
    if (!w.isDestroyed()) w.destroy();
  }
}

app.whenReady().then(async () => {
  try {
    console.log('JS2C_STEP browser');
    const browser = v8Util.getJs2cCodeCacheStatus();
    console.log('JS2C_STEP sandbox');
    const sandbox = await rendererStatus(true);
    await new Promise((resolve) => setTimeout(resolve, 300));
    console.log('JS2C_STEP renderer');
    const renderer = await rendererStatus(false);
    console.log('JS2C_STEP utility');
    const utility = await withTimeout(
      new Promise((resolve) => {
        const child = utilityProcess.fork(UTIL_CHILD);
        child.once('message', (m) => resolve(m));
      }),
      'utility'
    );
    console.log('JS2C_RESULT ' + JSON.stringify({ browser, sandbox, renderer, utility }));
    setTimeout(() => app.exit(0), 100);
  } catch (err) {
    console.log('JS2C_ERROR ' + String((err && err.stack) || err));
    setTimeout(() => app.exit(1), 100);
  }
});

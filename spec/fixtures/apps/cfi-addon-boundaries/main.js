const { app, BrowserWindow, ipcMain, utilityProcess } = require('electron');
const { once } = require('node:events');
const path = require('node:path');
const { Worker } = require('node:worker_threads');

const mode = process.argv[2];
const runEmbedderCallbacks = require('../../module/run-v8-embedder-callbacks');

const finish = () => {
  console.log(`CFI_ADDON_BOUNDARY_OK:${mode}`);
  app.exit(0);
};

const fail = (error) => {
  console.error(error);
  app.exit(1);
};

const runWorker = async (script) => {
  const worker = new Worker(script);
  const messagePromise = once(worker, 'message');
  const exitPromise = once(worker, 'exit');
  const [message] = await messagePromise;
  const [code] = await exitPromise;
  if (message !== 'ok' || code !== 0) {
    throw new Error(`Worker failed: message=${message}, code=${code}`);
  }
};

const runRenderer = async (options) => {
  const result = once(ipcMain, 'cfi-addon-boundary-result');
  const window = new BrowserWindow({
    show: false,
    webPreferences: options
  });
  await window.loadFile(path.join(__dirname, 'index.html'), {
    query: { mode }
  });
  const [, message] = await result;
  window.destroy();
  if (message !== 'ok') {
    throw new Error(message);
  }
};

const runUtility = async (script) => {
  const child = utilityProcess.fork(script, [], {
    stdio: ['ignore', 'pipe', 'pipe']
  });
  let stderr = '';
  child.stderr.on('data', (chunk) => {
    stderr += chunk;
  });
  const messagePromise = once(child, 'message');
  const exitPromise = once(child, 'exit');
  const [message] = await messagePromise;
  const [code] = await exitPromise;
  if (message !== 'ok' || code !== 0) {
    throw new Error(`Utility process failed: message=${message}, code=${code}\n${stderr}`);
  }
};

app
  .whenReady()
  .then(async () => {
    switch (mode) {
      case 'main':
        await runEmbedderCallbacks();
        break;
      case 'renderer-main-world':
        await runRenderer({
          nodeIntegration: true,
          contextIsolation: false
        });
        break;
      case 'renderer-isolated-world':
        await runRenderer({
          preload: path.join(__dirname, 'preload.js'),
          contextIsolation: true,
          sandbox: false
        });
        break;
      case 'renderer-web-worker':
        await runRenderer({
          nodeIntegration: true,
          nodeIntegrationInWorker: true,
          contextIsolation: false
        });
        break;
      case 'node-worker':
        await runWorker(path.join(__dirname, 'node-worker.js'));
        break;
      case 'utility':
        await runUtility(path.join(__dirname, 'utility.js'));
        break;
      case 'utility-node-worker':
        await runUtility(path.join(__dirname, 'utility-node-worker.js'));
        break;
      default:
        throw new Error(`Unknown CFI addon boundary mode: ${mode}`);
    }
    finish();
  })
  .catch(fail);

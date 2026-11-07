const { app, BrowserWindow, utilityProcess } = require('electron');

const { once } = require('node:events');
const { enableCompileCache, getCompileCacheDir } = require('node:module');
const path = require('node:path');
const { Worker } = require('node:worker_threads');

const report = require('./report');

const reportPath = path.join(__dirname, 'report.js');
const appVars = JSON.parse(process.env.TEST_APP_VARS);

const fromUtility = async (options) => {
  const child = utilityProcess.fork(reportPath, [], options);
  const [message] = await once(child, 'message');
  child.kill();
  return message;
};

app.whenReady().then(async () => {
  const result = { main: report() };

  const worker = new Worker(
    `require('node:worker_threads').parentPort.postMessage(require(${JSON.stringify(reportPath)})())`,
    { eval: true }
  );
  [result.worker] = await once(worker, 'message');

  result.utility = await fromUtility();
  result.utilityWithEnv = await fromUtility({ env: { ...process.env, ...appVars } });

  const w = new BrowserWindow({
    show: false,
    webPreferences: { nodeIntegration: true, contextIsolation: false, sandbox: false }
  });
  await w.loadURL('about:blank');
  result.renderer = await w.webContents.executeJavaScript(`require(${JSON.stringify(reportPath)})()`);
  w.destroy();

  // Whatever the fuse says, the app can turn the compile cache on itself.
  enableCompileCache(process.env.TEST_CACHE_DIR);
  result.enabledByApp = getCompileCacheDir() !== undefined;

  console.log(JSON.stringify(result));
  app.exit(0);
});

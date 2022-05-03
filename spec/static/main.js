// Deprecated APIs are still supported and should be tested.
process.throwDeprecation = false;

const electron = require('electron');
const { app, BrowserWindow, dialog, ipcMain, session } = electron;

try {
  require('fs').rmdirSync(app.getPath('userData'), { recursive: true });
} catch (e) {
  console.warn('Warning: couldn\'t clear user data directory:', e);
}

const fs = require('fs');
const path = require('path');
const util = require('util');
const v8 = require('v8');

const argv = require('yargs')
  .boolean('ci')
  .array('files')
  .string('g').alias('g', 'grep')
  .boolean('i').alias('i', 'invert')
  .argv;

let window = null;

v8.setFlagsFromString('--expose_gc');
app.commandLine.appendSwitch('js-flags', '--expose_gc');
app.commandLine.appendSwitch('ignore-certificate-errors');
app.commandLine.appendSwitch('disable-renderer-backgrounding');
// Some ports are considered to be "unsafe" by Chromium
// but Windows on Microsoft-hosted agents sometimes assigns one of them
// to a Node.js server. Chromium refuses to establish a connection
// and the whole app crashes with the "Error: net::ERR_UNSAFE_PORT" error.
// Let's allow connections to those ports to avoid test failures.
// Use a comma-separated list of ports as a flag value, e.g. "666,667,668".
app.commandLine.appendSwitch('explicitly-allowed-ports', '2049');

// Disable security warnings (the security warnings test will enable them)
process.env.ELECTRON_DISABLE_SECURITY_WARNINGS = true;

// Accessing stdout in the main process will result in the process.stdout
// throwing UnknownSystemError in renderer process sometimes. This line makes
// sure we can reproduce it in renderer process.
// eslint-disable-next-line
process.stdout

// Access console to reproduce #3482.
// eslint-disable-next-line
console

ipcMain.on('message', function (event, ...args) {
  event.sender.send('message', ...args);
});

ipcMain.handle('get-modules', () => Object.keys(electron));
ipcMain.handle('get-temp-dir', () => app.getPath('temp'));
ipcMain.handle('ping', () => null);

// Write output to file if OUTPUT_TO_FILE is defined.
const outputToFile = process.env.OUTPUT_TO_FILE;
const print = function (_, method, args) {
  const output = util.format.apply(null, args);
  if (outputToFile) {
    fs.appendFileSync(outputToFile, output + '\n');
  } else {
    console[method](output);
  }
};
ipcMain.on('console-call', print);

ipcMain.on('process.exit', function (event, code) {
  process.exit(code);
});

ipcMain.on('eval', function (event, script) {
  event.returnValue = eval(script) // eslint-disable-line
});

ipcMain.on('echo', function (event, msg) {
  event.returnValue = msg;
});

process.removeAllListeners('uncaughtException');
process.on('uncaughtException', function (error) {
  console.error(error, error.stack);
  process.exit(1);
});

global.nativeModulesEnabled = !process.env.ELECTRON_SKIP_NATIVE_MODULE_TESTS;

app.on('window-all-closed', function () {
  app.quit();
});

app.on('gpu-process-crashed', (event, killed) => {
  console.log(`GPU process crashed (killed=${killed})`);
});

app.on('renderer-process-crashed', (event, contents, killed) => {
  console.log(`webContents ${contents.id} crashed: ${contents.getURL()} (killed=${killed})`);
});

app.whenReady().then(async function () {
  await session.defaultSession.clearCache();
  await session.defaultSession.clearStorageData();
  // Test if using protocol module would crash.
  electron.protocol.registerStringProtocol('test-if-crashes', function () {});

  window = new BrowserWindow({
    title: 'Electron Tests',
    show: false,
    width: 800,
    height: 600,
    webPreferences: {
      backgroundThrottling: false,
      nodeIntegration: true,
      webviewTag: true,
      contextIsolation: false
    }
  });
  window.loadFile('static/index.html', {
    query: {
      grep: argv.grep,
      invert: argv.invert ? 'true' : '',
      files: argv.files ? argv.files.join(',') : undefined
    }
  });
  window.on('unresponsive', function () {
    const chosen = dialog.showMessageBox(window, {
      type: 'warning',
      buttons: ['Close', 'Keep Waiting'],
      message: 'Window is not responsing',
      detail: 'The window is not responding. Would you like to force close it or just keep waiting?'
    });
    if (chosen === 0) window.destroy();
  });
  window.webContents.on('crashed', function () {
    console.error('Renderer process crashed');
    process.exit(1);
  });
});

ipcMain.on('prevent-next-will-attach-webview', (event) => {
  event.sender.once('will-attach-webview', event => event.preventDefault());
});

ipcMain.on('break-next-will-attach-webview', (event, id) => {
  event.sender.once('will-attach-webview', (event, webPreferences, params) => {
    params.instanceId = null;
  });
});

ipcMain.on('disable-node-on-next-will-attach-webview', (event, id) => {
  event.sender.once('will-attach-webview', (event, webPreferences, params) => {
    params.src = `file://${path.join(__dirname, '..', 'fixtures', 'pages', 'c.html')}`;
    webPreferences.nodeIntegration = false;
  });
});

ipcMain.on('disable-preload-on-next-will-attach-webview', (event, id) => {
  event.sender.once('will-attach-webview', (event, webPreferences, params) => {
    params.src = `file://${path.join(__dirname, '..', 'fixtures', 'pages', 'webview-stripped-preload.html')}`;
    delete webPreferences.preload;
    delete webPreferences.preloadURL;
  });
});

ipcMain.on('handle-uncaught-exception', (event, message) => {
  suspendListeners(process, 'uncaughtException', (error) => {
    event.returnValue = error.message;
  });
  fs.readFile(__filename, () => {
    throw new Error(message);
  });
});

ipcMain.on('handle-unhandled-rejection', (event, message) => {
  suspendListeners(process, 'unhandledRejection', (error) => {
    event.returnValue = error.message;
  });
  fs.readFile(__filename, () => {
    Promise.reject(new Error(message));
  });
});

// Suspend listeners until the next event and then restore them
const suspendListeners = (emitter, eventName, callback) => {
  const listeners = emitter.listeners(eventName);
  emitter.removeAllListeners(eventName);
  emitter.once(eventName, (...args) => {
    emitter.removeAllListeners(eventName);
    listeners.forEach((listener) => {
      emitter.on(eventName, listener);
    });

    // eslint-disable-next-line standard/no-callback-literal
    callback(...args);
  });
};

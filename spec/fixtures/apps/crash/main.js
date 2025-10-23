const { app, BrowserWindow, crashReporter } = require('electron');

const childProcess = require('node:child_process');
const path = require('node:path');

app.setVersion('0.1.0');

const url = app.commandLine.getSwitchValue('crash-reporter-url');
const uploadToServer = !app.commandLine.hasSwitch('no-upload');
const setExtraParameters = app.commandLine.hasSwitch('set-extra-parameters-in-renderer');
const addGlobalParam = app.commandLine.getSwitchValue('add-global-param')?.split(':');

crashReporter.start({
  productName: 'Zombies',
  companyName: 'Umbrella Corporation',
  compress: false,
  uploadToServer,
  submitURL: url,
  ignoreSystemCrashHandler: true,
  extra: {
    mainProcessSpecific: 'mps'
  },
  globalExtra: addGlobalParam[0] ? { [addGlobalParam[0]]: addGlobalParam[1] } : {}
});

app.whenReady().then(() => {
  const crashType = app.commandLine.getSwitchValue('crash-type');

  if (crashType === 'main') {
    process.crash();
  } else if (crashType === 'renderer') {
    const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
    w.loadURL('about:blank');
    if (setExtraParameters) {
      w.webContents.executeJavaScript(`
        require('electron').crashReporter.addExtraParameter('rendererSpecific', 'rs');
        require('electron').crashReporter.addExtraParameter('addedThenRemoved', 'to-be-removed');
        require('electron').crashReporter.removeExtraParameter('addedThenRemoved');
      `);
    }
    w.webContents.executeJavaScript('process.crash()');
    w.webContents.on('render-process-gone', () => process.exit(0));
  } else if (crashType === 'sandboxed-renderer') {
    const w = new BrowserWindow({
      show: false,
      webPreferences: {
        sandbox: true,
        preload: path.resolve(__dirname, 'sandbox-preload.js'),
        contextIsolation: false
      }
    });
    w.loadURL(`about:blank?set_extra=${setExtraParameters ? 1 : 0}`);
    w.webContents.on('render-process-gone', () => process.exit(0));
  } else if (crashType === 'node') {
    const crashPath = path.join(__dirname, 'node-crash.js');
    const child = childProcess.fork(crashPath, { silent: true });
    child.on('exit', () => process.exit(0));
  } else if (crashType === 'node-fork') {
    const scriptPath = path.join(__dirname, 'fork.js');
    const child = childProcess.fork(scriptPath, { silent: true });
    child.on('exit', () => process.exit(0));
  } else if (crashType === 'node-extra-args') {
    let exitcode = -1;
    const crashPath = path.join(__dirname, 'node-extra-args.js');
    const child = childProcess.fork(crashPath, ['--enable-logging'], { silent: true });
    child.send('message');
    child.on('message', (forkedArgs) => {
      if (JSON.stringify(forkedArgs) !== JSON.stringify(child.spawnargs)) { exitcode = 1; } else { exitcode = 0; }
      process.exit(exitcode);
    });
  } else {
    console.error(`Unrecognized crash type: '${crashType}'`);
    process.exit(1);
  }
});

setTimeout(() => app.exit(), 30000);

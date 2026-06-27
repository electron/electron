const fs = require('node:fs');
const t0 = Date.now();
const log = (m) => {
  try {
    fs.writeSync(2, `[pdc] +${Date.now() - t0}ms ${m}\n`);
  } catch {}
};
log('script start');

const { app, BrowserWindow, shell } = require('electron');
log('electron required');

// Heartbeat to show event-loop liveness + timing during startup. unref() so it
// never keeps the process alive (the fixture must still exit via app.quit()).
const heartbeat = setInterval(() => log('heartbeat'), 1000);
heartbeat.unref();

process.on('uncaughtException', (e) => log(`uncaughtException: ${e && e.stack ? e.stack : e}`));
process.on('unhandledRejection', (e) => log(`unhandledRejection: ${e}`));

let win = null;

function createWindow() {
  log('createWindow');
  win = new BrowserWindow({
    width: 800,
    height: 600,
    show: true,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false
    }
  });

  win.on('closed', () => log('window closed'));

  win.loadURL('data:text/html,<h1>repro</h1>');
}

async function createPromiseAndQuit() {
  const url = `unknownscheme-${Date.now()}://test`;
  const p = shell.openExternal(url, { activate: false });
  log('openExternal returned, promise pending');

  setTimeout(() => {
    log('calling app.quit()');
    app.quit();
  }, 0);

  p.then(() => {
    log('promise resolved.');
  }).catch(() => {
    log('promise rejected.');
  });
}

log('registering whenReady');
app.whenReady().then(() => {
  log('app ready');
  app.on('before-quit', () => log('before-quit'));
  app.on('will-quit', () => log('will-quit'));
  app.on('quit', () => log('quit'));

  createWindow();

  setTimeout(() => {
    createPromiseAndQuit();
  }, 500);
});

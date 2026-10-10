// Manual check for JS entered straight from a native event while the app is
// idle and another app has focus: run `electron probe.js`, then press
// CmdOrCtrl+Shift+F9 or use the "uv probe > Run" menu item and read the
// latencies printed to stdout. Each should be a few milliseconds at most.
const { app, BrowserWindow, globalShortcut, Menu } = require('electron');

const fs = require('node:fs');
const os = require('node:os');
const timersP = require('node:timers/promises');

let watchedOnce = false;

function probe(from) {
  const t0 = performance.now();
  const report =
    (what, expect = 0) =>
    () =>
      console.log(`${from}: ${what} ${(performance.now() - t0 - expect).toFixed(1)} ms late`);
  timersP.setTimeout(50).then(report('timers/promises.setTimeout(50)', 50));
  timersP.setImmediate().then(report('timers/promises.setImmediate'));
  if (!watchedOnce) {
    watchedOnce = true;
    const file = `${os.tmpdir()}/uv-probe-${process.pid}`;
    fs.writeFileSync(file, '0');
    const w = fs.watch(file, () => {
      w.close();
      fs.rmSync(file);
      report('first fs.watch')();
    });
    setImmediate(() => fs.writeFileSync(file, '1'));
  }
}

app.whenReady().then(() => {
  const w = new BrowserWindow({ width: 300, height: 200 });
  w.loadURL('data:text/html,uv probe: press CmdOrCtrl+Shift+F9 or use the menu with another app focused');
  globalShortcut.register('CmdOrCtrl+Shift+F9', () => probe('globalShortcut'));
  Menu.setApplicationMenu(
    Menu.buildFromTemplate([
      { label: 'uv probe', submenu: [{ label: 'Run', click: () => probe('menu') }, { role: 'quit' }] }
    ])
  );
  console.log('ready');
});

// Shows a window on the display it was started on and reports on stdout once
// the window is up.
const { app, BrowserWindow } = require('electron');

const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');

app.setPath('userData', fs.mkdtempSync(path.join(os.tmpdir(), 'display-lost-userdata-')));

app.whenReady().then(async () => {
  const w = new BrowserWindow({ show: true });
  await w.loadURL('about:blank');
  process.stdout.write('window-ready\n');
});

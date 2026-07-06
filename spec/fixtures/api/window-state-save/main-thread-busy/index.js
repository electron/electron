const { app, BrowserWindow } = require('electron');

const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');

const sharedUserData = path.join(os.tmpdir(), 'electron-window-state-test');
app.setPath('userData', sharedUserData);
const prefsPath = path.join(sharedUserData, 'Local State');

const waitForPrefsFile = async () => {
  while (!fs.existsSync(prefsPath)) {
    await new Promise((resolve) => setTimeout(resolve, 100));
  }
};

app.whenReady().then(async () => {
  const w = new BrowserWindow({
    width: 400,
    height: 300,
    name: 'test-main-thread-busy',
    windowStatePersistence: true,
    show: false
  });

  await waitForPrefsFile();
  const initialMtime = fs.statSync(prefsPath).mtimeMs;

  const moved = new Promise((resolve) => w.once('move', resolve));
  w.setPosition(100, 100);
  await moved;

  const startTime = Date.now();
  while (Date.now() - startTime < 25_000);

  const finalMtime = fs.statSync(prefsPath).mtimeMs;
  const exitCode = finalMtime === initialMtime ? 0 : 1;

  w.destroy();
  app.exit(exitCode);
});

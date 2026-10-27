const { app, nativeImage, powerMonitor, Tray } = require('electron');

const fs = require('node:fs');

// Only the lost D-Bus connection may end this app. That path exits without
// 'before-quit', so this blocks any other quit, such as one from SIGTERM.
app.on('before-quit', (event) => event.preventDefault());

// Electron exits when a bus goes away, so 'quit' runs after Chromium has
// handled the disconnect.
// Holds the tray created in 'quit', so that it is not garbage collected.
let tray; // eslint-disable-line @typescript-eslint/no-unused-vars
app.on('quit', () => {
  // Starts an org.freedesktop.login1 method call on the lost system bus.
  powerMonitor.on('shutdown', () => {});
  // Makes a NameHasOwner call for org.kde.StatusNotifierWatcher on the
  // session bus, which is still up. Both buses use the same D-Bus thread, so
  // that call goes out only after the call above.
  tray = new Tray(nativeImage.createEmpty());
  process.stdout.write('quit\n');
  // Keep the process alive until the test has seen the call.
  fs.readSync(0, Buffer.alloc(1));
});

app.whenReady().then(() => {
  // Creates the org.freedesktop.login1 proxy on the system bus.
  powerMonitor.on('suspend', () => {});
  process.stdout.write('ready\n');
});

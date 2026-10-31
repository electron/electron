const { app, powerMonitor } = require('electron');

const fs = require('node:fs');

// Only the lost D-Bus connection may end this app. That path exits without
// 'before-quit', so this blocks any other quit, such as one from SIGTERM.
app.on('before-quit', (event) => event.preventDefault());

// Electron exits when a bus goes away, so 'quit' runs after Chromium has
// handled the disconnect.
app.on('quit', () => {
  // Starts an org.freedesktop.login1 method call on the lost system bus.
  powerMonitor.on('shutdown', () => {});
  // Sends a signal on the session bus, which is still up. Both buses use the
  // same D-Bus thread, so the signal goes out only after the call above.
  app.setBadgeCount(1);
  process.stdout.write('quit\n');
  // Keep the process alive until the test has seen the signal.
  fs.readSync(0, Buffer.alloc(1));
});

app.whenReady().then(() => {
  // Creates the org.freedesktop.login1 proxy on the system bus.
  powerMonitor.on('suspend', () => {});
  process.stdout.write('ready\n');
});

const { app } = require('electron');

app.disableHardwareAcceleration();

app.whenReady().then(() => {
  process.stdout.write(JSON.stringify(app.commandLine.hasSwitch('disable-gpu')));
  process.stdout.end();
  app.quit();
});

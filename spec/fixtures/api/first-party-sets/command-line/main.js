const { app } = require('electron');

app.whenReady().then(function () {
  const hasSwitch = app.commandLine.hasSwitch('use-first-party-set');
  const value = app.commandLine.getSwitchValue('use-first-party-set');
  if (hasSwitch) {
    process.stdout.write(JSON.stringify(value));
    process.stdout.end();
  }

  app.quit();
});

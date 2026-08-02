const { app } = require('electron');

app.whenReady().then(() => {
  const payload = {
    hasSwitch: app.commandLine.hasSwitch('foobar'),
    getSwitchValue: app.commandLine.getSwitchValue('foobar'),
    hasDisableGpu: app.commandLine.hasSwitch('disable-gpu')
  };

  process.stdout.write(JSON.stringify(payload));
  process.stdout.end();

  app.quit();
});

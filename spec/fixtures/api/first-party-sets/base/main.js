const { app } = require('electron');

const sets = [
  'https://fps-member1.glitch.me',
  'https://fps-member2.glitch.me',
  'https://fps-member3.glitch.me'
];

app.commandLine.appendSwitch('use-first-party-set', sets.join(','));

app.whenReady().then(function () {
  const hasSwitch = app.commandLine.hasSwitch('use-first-party-set');
  const value = app.commandLine.getSwitchValue('use-first-party-set');
  if (hasSwitch) {
    process.stdout.write(JSON.stringify(value));
    process.stdout.end();
  }

  app.quit();
});

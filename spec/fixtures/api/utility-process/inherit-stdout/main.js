const { app, UtilityProcess } = require('electron');
const path = require('path');

app.whenReady().then(() => {
  const payload = app.commandLine.getSwitchValue('payload');
  const child = new UtilityProcess(path.join(__dirname, 'test.js'), [`--payload=${payload}`], {
    stdio: ['ignore', 'inherit', 'inherit']
  });
  child.on('exit', () => {
    app.quit();
  });
});

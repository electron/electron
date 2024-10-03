const { app, utilityProcess } = require('electron');

const path = require('node:path');

app.whenReady().then(() => {
  const payload = app.commandLine.getSwitchValue('payload');
  const child = utilityProcess.fork(path.join(__dirname, 'test.js'), [`--payload=${payload}`]);
  child.on('exit', () => {
    app.quit();
  });
});

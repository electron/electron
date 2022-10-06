const { app, utilityProcess } = require('electron');
const path = require('path');

app.whenReady().then(() => {
  let child = null;
  if (app.commandLine.hasSwitch('create-custom-env')) {
    child = utilityProcess.fork(path.join(__dirname, 'test.js'), {
      stdio: 'inherit',
      env: {
        FROM: 'child'
      }
    });
  } else {
    child = utilityProcess.fork(path.join(__dirname, 'test.js'), {
      stdio: 'inherit'
    });
  }
  child.on('exit', () => {
    app.quit();
  });
});

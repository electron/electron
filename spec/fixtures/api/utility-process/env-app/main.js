const { app, UtilityProcess } = require('electron');
const path = require('path');

app.whenReady().then(() => {
  let child = null;
  if (app.commandLine.hasSwitch('create-custom-env')) {
    child = new UtilityProcess(path.join(__dirname, 'test.js'), {
      stdio: 'inherit',
      env: {
        FROM: 'child'
      }
    });
  } else {
    child = new UtilityProcess(path.join(__dirname, 'test.js'), {
      stdio: 'inherit'
    });
  }
  child.on('exit', () => {
    app.quit();
  });
});

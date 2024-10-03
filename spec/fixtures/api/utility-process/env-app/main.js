const { app, utilityProcess } = require('electron');

const path = require('node:path');

app.whenReady().then(() => {
  let child = null;
  if (app.commandLine.hasSwitch('create-custom-env')) {
    child = utilityProcess.fork(path.join(__dirname, 'test.js'), {
      env: {
        FROM: 'child'
      }
    });
  } else {
    child = utilityProcess.fork(path.join(__dirname, 'test.js'));
  }
  child.on('message', (data) => {
    process.stdout.write(data);
    process.stdout.end();
  });
  child.on('exit', () => {
    app.quit();
  });
});

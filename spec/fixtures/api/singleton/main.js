const { app } = require('electron');

app.whenReady().then(() => {
  console.log('started'); // ping parent
});

const gotTheLock = app.requestSingleInstanceLock();

app.on('second-instance', (event, args, workingDirectory, data) => {
  setImmediate(() => {
    console.log([JSON.stringify(args), JSON.stringify(data)].join('||'));
    app.exit(0);
  });
});

if (!gotTheLock) {
  app.exit(1);
}

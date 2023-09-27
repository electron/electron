const { app } = require('electron');

app.whenReady().then(() => {
  console.log('started'); // ping parent
});

const gotTheLock = app.requestSingleInstanceLock();

app.on('second-instance', (event, args, workingDirectory) => {
  setImmediate(() => {
    console.log(JSON.stringify(args), workingDirectory);
    app.exit(0);
  });
});

if (!gotTheLock) {
  app.exit(1);
}

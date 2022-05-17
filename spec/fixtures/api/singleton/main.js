const { app } = require('electron');

app.whenReady().then(() => {
  console.log('started'); // ping parent
});

app.on('first-instance-ack', (event, additionalData) => {
  console.log(JSON.stringify(additionalData));
  setImmediate(() => {
    app.exit(1);
  });
});

const gotTheLock = app.requestSingleInstanceLock();

app.on('second-instance', (event, args, workingDirectory) => {
  setImmediate(() => {
    console.log(JSON.stringify(args));
    app.exit(0);
  });
});

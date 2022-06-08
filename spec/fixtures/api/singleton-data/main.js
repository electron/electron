const { app } = require('electron');

// Send data from the second instance to the first instance.
const sendAdditionalData = app.commandLine.hasSwitch('send-data');

app.whenReady().then(() => {
  console.log('started'); // ping parent
});

let obj = {
  level: 1,
  testkey: 'testvalue1',
  inner: {
    level: 2,
    testkey: 'testvalue2'
  }
};
if (app.commandLine.hasSwitch('data-content')) {
  obj = JSON.parse(app.commandLine.getSwitchValue('data-content'));
  if (obj === 'undefined') {
    obj = undefined;
  }
}

const gotTheLock = sendAdditionalData
  ? app.requestSingleInstanceLock(obj) : app.requestSingleInstanceLock();

app.on('second-instance', (event, args, workingDirectory, data) => {
  setImmediate(() => {
    console.log([JSON.stringify(args), JSON.stringify(data)].join('||'));
    app.exit(0);
  });
});

if (!gotTheLock) {
  app.exit(1);
}

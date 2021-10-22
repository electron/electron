const { app } = require('electron');

// Send data from the second instance to the first instance.
const sendAdditionalData = app.commandLine.hasSwitch('send-data');

app.whenReady().then(() => {
  console.log('started'); // ping parent
});

// Send data from the second instance to the first instance.
const sendAdditionalData = app.commandLine.hasSwitch('send-data');

// Prevent the default behaviour of second-instance, which sends back an empty ack.
const preventDefault = app.commandLine.hasSwitch('prevent-default');

// Send an object back for the ack rather than undefined.
const sendAck = app.commandLine.hasSwitch('send-ack');

let obj = {
  level: 1,
  testkey: 'testvalue1',
  inner: {
    level: 2,
    testkey: 'testvalue2'
  }
};
let ackObj = {
  level: 1,
  testkey: 'acktestvalue1',
  inner: {
    level: 2,
    testkey: 'acktestvalue2'
  }
};

if (app.commandLine.hasSwitch('data-content')) {
  obj = JSON.parse(app.commandLine.getSwitchValue('data-content'));
  if (obj === 'undefined') {
    obj = undefined;
  }
}
if (app.commandLine.hasSwitch('ack-content')) {
  ackObj = JSON.parse(app.commandLine.getSwitchValue('ack-content'));
  if (ackObj === 'undefined') {
    ackObj = undefined;
  }
}

app.on('first-instance-ack', (event, additionalData) => {
  console.log(JSON.stringify(additionalData));
});

const gotTheLock = sendAdditionalData
  ? app.requestSingleInstanceLock(obj) : app.requestSingleInstanceLock();

app.on('second-instance', (event, args, workingDirectory, data, ackCallback) => {
  if (preventDefault) {
    event.preventDefault();
  }
  setImmediate(() => {
    console.log([JSON.stringify(args), JSON.stringify(data)].join('||'));
    sendAck ? ackCallback(ackObj) : ackCallback();
    app.exit(0);
  });
});

if (!gotTheLock) {
  app.exit(1);
}

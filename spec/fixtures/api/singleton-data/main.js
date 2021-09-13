const { app } = require('electron');

app.whenReady().then(() => {
  console.log('started'); // ping parent
});

const obj = {
  level: 1,
  testkey: 'testvalue1',
  inner: {
    level: 2,
    testkey: 'testvalue2'
  }
};
const gotTheLock = app.requestSingleInstanceLock(obj);

app.on('second-instance', (event, args, workingDirectory, data) => {
  setImmediate(() => {
    console.log([JSON.stringify(args), JSON.stringify(data)].join('||'));
    app.exit(0);
  });
});

if (!gotTheLock) {
  app.exit(1);
}

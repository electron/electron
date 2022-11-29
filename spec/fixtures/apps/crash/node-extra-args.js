const path = require('path');
const childProcess = require('child_process');

process.on('message', function () {
  process.send(process.argv);
});

// Allow time to send args, then crash the app.
setTimeout(() => process.nextTick(() => process.crash()), 10000);

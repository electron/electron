const childProcess = require('node:child_process');
const path = require('node:path');

process.on('uncaughtException', function (error) {
  process.send(error.stack);
});

const child = childProcess.fork(path.join(__dirname, '/ping.js'));

process.on('message', function (msg) {
  child.send(msg);
});
child.on('message', function (msg) {
  process.send(msg);
});
child.on('exit', function (code) {
  process.exit(code);
});

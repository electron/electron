const path = require('path');
const childProcess = require('child_process');

const crashPath = path.join(__dirname, 'node-crash.js');
const child = childProcess.fork(crashPath, { silent: true });
child.on('exit', () => process.exit(0));

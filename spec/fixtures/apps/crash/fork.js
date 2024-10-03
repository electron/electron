const childProcess = require('node:child_process');
const path = require('node:path');

const crashPath = path.join(__dirname, 'node-crash.js');
const child = childProcess.fork(crashPath, { silent: true });
child.on('exit', () => process.exit(0));

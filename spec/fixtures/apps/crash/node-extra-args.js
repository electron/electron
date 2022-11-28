const path = require('path');
const childProcess = require('child_process');
const argv = process.argv;
let exitCode = 0;
if (argv.length !== 3) {
  exitCode = 1;
}
const crashPath = path.join(__dirname, 'node-crash.js');
const child = childProcess.fork(crashPath, { silent: true });
child.on('exit', () => process.exit(exitCode));

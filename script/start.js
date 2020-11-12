const cp = require('child_process');
const utils = require('./lib/utils');
const electronPath = utils.getAbsoluteElectronExec();

console.log('will start from start.js using', electronPath, 'argv = ', rprocess.argv)
const child = cp.spawn(electronPath, process.argv.slice(2), { stdio: 'inherit' });
child.on('close', (code) => process.exit(code));

const handleTerminationSignal = (signal) =>
  process.on(signal, () => {
    if (!child.killed) {
      child.kill(signal);
    }
  });

handleTerminationSignal('SIGINT');
handleTerminationSignal('SIGTERM');

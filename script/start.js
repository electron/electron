const cp = require('node:child_process');
const utils = require('./lib/utils');
const electronPath = utils.getAbsoluteElectronExec();

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
handleTerminationSignal('SIGUSR2');

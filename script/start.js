const { spawn } = require('node:child_process');
const utils = require('./lib/utils');

const electronPath = utils.getAbsoluteElectronExec();

let child;

try {
  child = spawn(electronPath, process.argv.slice(2), {
    stdio: 'inherit',
  });
} catch (err) {
  console.error('Failed to start Electron:', err);
  process.exit(1);
}

// child exit handling
child.once('exit', (code, signal) => {
  if (signal) {
    process.kill(process.pid, signal);
  } else {
    process.exit(code ?? 0);
  }
});

// forward signals
['SIGINT', 'SIGTERM', 'SIGUSR2'].forEach((signal) => {
  process.on(signal, () => {
    if (!child.killed) {
      child.kill(signal);
    }
  });
});


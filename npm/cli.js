#!/usr/bin/env node

const proc = require('child_process');

const electron = require('./');

const child = proc.spawn(electron, process.argv.slice(2), { stdio: 'inherit', windowsHide: false });
child.on('close', function (code, signal) {
  if (code === null) {
    console.error(electron, 'exited with signal', signal);
    process.exit(1);
  }
  process.exit(code);
});

const handleTerminationSignal = function (signal) {
  process.on(signal, function signalHandler () {
    if (!child.killed) {
      child.kill(signal);
    }
  });
};

handleTerminationSignal('SIGINT');
handleTerminationSignal('SIGTERM');

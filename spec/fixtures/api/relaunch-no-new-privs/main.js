const { app } = require('electron');

const fs = require('node:fs');
const net = require('node:net');

const socketPath = '/tmp/electron-app-relaunch';

process.on('uncaughtException', () => {
  app.exit(1);
});

app.whenReady().then(() => {
  if (process.argv[process.argv.length - 1] !== '--relaunched') {
    app.relaunch({ args: process.argv.slice(1).concat('--relaunched') });
    app.exit(0);
    return;
  }
  const noNewPrivs = fs.readFileSync('/proc/self/status', 'utf8').match(/NoNewPrivs:\s*(\d)/)[1];
  const client = net.connect(socketPath);
  client.once('connect', () => {
    client.end(noNewPrivs);
  });
  client.once('end', () => {
    app.exit(0);
  });
});

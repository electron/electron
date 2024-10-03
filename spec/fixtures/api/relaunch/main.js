const { app } = require('electron');

const net = require('node:net');

const socketPath = process.platform === 'win32' ? '\\\\.\\pipe\\electron-app-relaunch' : '/tmp/electron-app-relaunch';

process.on('uncaughtException', () => {
  app.exit(1);
});

app.whenReady().then(() => {
  const lastArg = process.argv[process.argv.length - 1];
  const client = net.connect(socketPath);
  client.once('connect', () => {
    client.end(lastArg);
  });
  client.once('end', () => {
    if (lastArg === '--first') {
      // Once without execPath specified
      app.relaunch({ args: process.argv.slice(1, -1).concat('--second') });
    } else if (lastArg === '--second') {
      // And once with execPath specified
      app.relaunch({ execPath: process.argv[0], args: process.argv.slice(1, -1).concat('--third') });
    }
    app.exit(0);
  });
});

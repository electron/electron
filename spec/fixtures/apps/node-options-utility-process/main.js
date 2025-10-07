const { app, utilityProcess } = require('electron');

const path = require('node:path');

app.whenReady().then(() => {
  const child = utilityProcess.fork(path.join(__dirname, 'noop.js'), [], {
    stdio: 'inherit',
    env: {
      ...process.env,
      NODE_OPTIONS: `--require ${path.join(__dirname, 'fail.js')}`
    }
  });

  child.once('exit', (code) => app.exit(code));
});

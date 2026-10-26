// Calling postMessage() on a utility process from its 'exit' listener, or
// after the process has gone away, must be a no-op rather than crashing the
// main process.
const { app, utilityProcess } = require('electron');

const path = require('node:path');

app.whenReady().then(() => {
  const child = utilityProcess.fork(path.join(__dirname, 'utility.js'));
  child.on('exit', () => {
    child.postMessage('after-exit');
    setTimeout(() => {
      child.postMessage('after-exit-later');
      app.quit();
    });
  });
  child.once('message', () => {
    // Child is up; quit while it is still running so the exit event is
    // delivered from the shutdown path.
    child.postMessage('before-exit');
    app.quit();
  });
});

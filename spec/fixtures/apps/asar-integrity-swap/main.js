const { app, utilityProcess } = require('electron');

const { execFileSync } = require('node:child_process');
const path = require('node:path');

// Replaces the running app's archive with the path in ASAR_SWAP_REPLACEMENT,
// then reports what a newly started child process loads from it. The move is
// made by another process, as blocking file operations are not allowed here.
app.whenReady().then(() => {
  const archive = path.join(process.resourcesPath, 'app.asar');
  execFileSync('/bin/mv', [archive, `${archive}.orig`]);
  execFileSync('/bin/mv', [process.env.ASAR_SWAP_REPLACEMENT, archive]);

  const child = utilityProcess.fork(path.join(archive, 'child.js'));
  child.on('message', (message) => console.log(message));
  child.on('exit', () => {
    console.log('child-exited');
    app.quit();
  });
  setTimeout(() => child.kill(), 10000);
});

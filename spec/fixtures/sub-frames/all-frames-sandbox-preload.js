const { ipcRenderer } = require('electron');

let canRequireFs = true;
try {
  require('node:fs');
} catch {
  canRequireFs = false;
}

ipcRenderer.send('all-frames-preload', window.location.href, process.sandboxed === true, canRequireFs);

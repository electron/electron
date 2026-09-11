// Reports what the preload's world sees on navigator.electron at its first
// line, then the result of calling test.ping() if it can.
const { ipcRenderer } = require('electron');

const names = 'electron' in navigator ? Object.keys(navigator.electron) : null;
ipcRenderer.send('api-bridge-preload', names);
if (typeof navigator.electron?.test?.ping === 'function') {
  navigator.electron.test.ping().then((value) => ipcRenderer.send('api-bridge-preload-ping', value));
}

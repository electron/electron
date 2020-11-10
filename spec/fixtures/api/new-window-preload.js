const { ipcRenderer, webFrame } = require('electron');

ipcRenderer.send('answer', {
  nativeWindowOpen: webFrame.getWebPreference('nativeWindowOpen'),
  argv: process.argv
});
window.close();

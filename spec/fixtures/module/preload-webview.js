const { ipcRenderer } = require('electron');

window.onload = function () {
  ipcRenderer.send('webview', typeof WebView);
};

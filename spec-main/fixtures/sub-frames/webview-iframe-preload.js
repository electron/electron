const { ipcRenderer } = require('electron');

if (process.isMainFrame) {
  window.addEventListener('DOMContentLoaded', () => {
    const webview = document.createElement('webview');
    webview.src = 'about:blank';
    webview.setAttribute('webpreferences', 'contextIsolation=no');
    webview.addEventListener('did-finish-load', () => {
      ipcRenderer.send('webview-loaded');
    }, { once: true });
    document.body.appendChild(webview);
  });
} else {
  ipcRenderer.send('preload-in-frame');
}

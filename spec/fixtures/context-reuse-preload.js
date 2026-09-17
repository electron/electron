const { contextBridge, ipcRenderer } = require('electron');

// Reported before the marker is exposed so that a second run in the same
// context is still counted: exposeInMainWorld() would throw on the marker.
ipcRenderer.send('context-reuse-preload-ran', window.location.href);

if (process.contextIsolated) {
  contextBridge.exposeInMainWorld('preloadMarker', 'ready');
} else {
  window.preloadMarker = 'ready';
}

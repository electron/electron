// Exposes the benchmark harness API to pages loaded in the host window.
// Pages check for window.cbHarness and skip the scenario when it is missing
// (on Electron versions without contextBridge).
const { contextBridge, ipcRenderer } = require('electron');

if (contextBridge && typeof ipcRenderer.invoke === 'function') {
  contextBridge.exposeInMainWorld('cbHarness', {
    setSize: (width, height) => ipcRenderer.invoke('cb-set-size', width, height),
    getSize: () => ipcRenderer.invoke('cb-get-size')
  });
}

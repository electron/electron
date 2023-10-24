const { contextBridge, ipcRenderer } = require('electron/renderer')

contextBridge.exposeInMainWorld('electronAPI', {
  onUpdateCounter: (callback) => ipcRenderer.on('update-counter', (event, value) => callback(value)),
  counterValue: (value) => ipcRenderer.send('counter-value', value)
})

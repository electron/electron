const { contextBridge, ipcRenderer } = require('electron/renderer')

contextBridge.exposeInMainWorld('electronAPI', {
  createDemoWindow: () => ipcRenderer.send('create-demo-window'),
  onBoundsChanged: (callback) => ipcRenderer.on('bounds-changed', () => callback())
})

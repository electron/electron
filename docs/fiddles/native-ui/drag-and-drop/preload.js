const { contextBridge, ipcRenderer } = require('electron/renderer')

contextBridge.exposeInMainWorld('electronAPI', {
  dragStart: () => ipcRenderer.send('ondragstart')
})

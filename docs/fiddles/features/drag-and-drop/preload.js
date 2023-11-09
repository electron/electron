const { contextBridge, ipcRenderer } = require('electron/renderer')

contextBridge.exposeInMainWorld('electron', {
  startDrag: (fileName) => ipcRenderer.send('ondragstart', fileName)
})

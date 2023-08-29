const { contextBridge, ipcRenderer } = require('electron/renderer')

contextBridge.exposeInMainWorld('electronAPI', {
  newWindow: (args) => ipcRenderer.send('new-window', args)
})

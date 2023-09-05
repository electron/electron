const { contextBridge, ipcRenderer } = require('electron/renderer')

contextBridge.exposeInMainWorld('electronAPI', {
  createFramelessWindow: (args) => ipcRenderer.send('create-frameless-window', args)
})

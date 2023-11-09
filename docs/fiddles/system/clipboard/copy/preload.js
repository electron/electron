const { contextBridge, ipcRenderer } = require('electron/renderer')

contextBridge.exposeInMainWorld('clipboard', {
  writeText: (text) => ipcRenderer.invoke('clipboard:writeText', text)
})

const { contextBridge, ipcRenderer } = require('electron')

contextBridge.exposeInMainWorld('clipboard', {
  writeText: (text) => ipcRenderer.invoke('clipboard:writeText', text)
})

const { contextBridge, ipcRenderer } = require('electron/renderer')

contextBridge.exposeInMainWorld('clipboard', {
  readText: () => ipcRenderer.invoke('clipboard:readText'),
  writeText: (text) => ipcRenderer.invoke('clipboard:writeText', text)
})

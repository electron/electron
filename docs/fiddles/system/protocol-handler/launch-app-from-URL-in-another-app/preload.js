const { contextBridge, ipcRenderer } = require('electron/renderer')

contextBridge.exposeInMainWorld('shell', {
  open: () => ipcRenderer.send('shell:open')
})

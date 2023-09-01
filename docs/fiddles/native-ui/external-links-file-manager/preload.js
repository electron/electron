const { contextBridge, ipcRenderer } = require('electron/renderer')

contextBridge.exposeInMainWorld('electronAPI', {
  openHomeDir: () => ipcRenderer.send('open-home-dir'),
  openExternal: (url) => ipcRenderer.send('open-external', url)
})

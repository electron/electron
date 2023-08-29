const { contextBridge, ipcRenderer } = require('electron/renderer')

contextBridge.exposeInMainWorld('electronAPI', {
  saveDialog: () => ipcRenderer.invoke('save-dialog')
})

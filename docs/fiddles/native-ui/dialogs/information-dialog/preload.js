const { contextBridge, ipcRenderer } = require('electron/renderer')

contextBridge.exposeInMainWorld('electronAPI', {
  openInformationDialog: () => ipcRenderer.invoke('open-information-dialog')
})

const { contextBridge, ipcRenderer } = require('electron/renderer')

contextBridge.exposeInMainWorld('electronAPI', {
  openErrorDialog: () => ipcRenderer.send('open-error-dialog')
})

const { contextBridge, ipcRenderer } = require('electron/renderer');

contextBridge.exposeInMainWorld('electronAPI', {
  toggleLeftSidebar: () => ipcRenderer.send('toggleLeftSidebar'),
  toggleRightSidebar: () => ipcRenderer.send('toggleRightSidebar')
})

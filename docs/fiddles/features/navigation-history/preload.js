const { contextBridge, ipcRenderer } = require('electron')

contextBridge.exposeInMainWorld('electronAPI', {
  goBack: () => ipcRenderer.invoke('nav:back'),
  goForward: () => ipcRenderer.invoke('nav:forward'),
  canGoBack: () => ipcRenderer.invoke('nav:canGoBack'),
  canGoForward: () => ipcRenderer.invoke('nav:canGoForward'),
  loadURL: (url) => ipcRenderer.invoke('nav:loadURL', url),
  getCurrentURL: () => ipcRenderer.invoke('nav:getCurrentURL'),
  getHistory: () => ipcRenderer.invoke('nav:getHistory'),
  onNavigationUpdate: (callback) => ipcRenderer.on('nav:updated', callback)
})

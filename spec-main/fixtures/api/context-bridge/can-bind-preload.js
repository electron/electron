const { contextBridge, ipcRenderer } = require('electron')

console.info(contextBridge)

let bound = false
try {
  contextBridge.bindAPIInMainWorld('test', {})
  bound = true
} catch {
  // Ignore
}

ipcRenderer.send('context-bridge-bound', bound)

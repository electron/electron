const { contextBridge } = require('electron/renderer')

contextBridge.exposeInMainWorld('electronVersion', process.versions.electron)

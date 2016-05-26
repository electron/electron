var ipcRenderer = require('electron').ipcRenderer
ipcRenderer.send('preload-script-1', true)

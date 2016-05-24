var ipcRenderer = require('electron').ipcRenderer
ipcRenderer.send('preload-script-2', true)

const { ipcRenderer, remote } = require('electron')

ipcRenderer.send('answer', remote.getCurrentWebContents().getWebPreferences())
window.close()

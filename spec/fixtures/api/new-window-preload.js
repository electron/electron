const {ipcRenderer, remote} = require('electron')

ipcRenderer.send('answer', process.argv, remote.getCurrentWindow().webContents.getWebPreferences())
window.close()

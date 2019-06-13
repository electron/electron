const { ipcRenderer } = require('electron')

if (process.isMainFrame) {
  ipcRenderer.send('pid-main', process.pid)
} else {
  ipcRenderer.send('pid-frame', process.pid)
}

const { remote, ipcRenderer } = require('electron')
remote.getCurrentWindow().rendererFunc = () => {
  ipcRenderer.send('done')
}
remote.getCurrentWindow().rendererFunc()

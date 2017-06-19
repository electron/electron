const { ipcRenderer } = require('electron')
if (process) {
  process.once('loaded', function () {
    ipcRenderer.send('processArgs', process.argv)
  })
} else {
  ipcRenderer.send('processArgs', 'unable to get process args')
}

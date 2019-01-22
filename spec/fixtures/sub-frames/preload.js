const { ipcRenderer, webFrame } = require('electron')

ipcRenderer.send('preload-ran', window.location.href, webFrame.routingId)

ipcRenderer.on('preload-ping', () => {
  ipcRenderer.send('preload-pong', webFrame.routingId)
})

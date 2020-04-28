const { webFrame, ipcRenderer } = require('electron');

ipcRenderer.send('ready');

ipcRenderer.on('getTextZoomFactor', (event) => {
  event.sender.send('textZoomFactor', webFrame.getTextZoomFactor());
});

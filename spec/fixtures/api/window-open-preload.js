const { ipcRenderer, webFrame } = require('electron');

setTimeout(function () {
  if (window.location.toString() === 'bar://page/') {
    const windowOpenerIsNull = window.opener == null;
    ipcRenderer.send('answer', {
      nodeIntegration: webFrame.getWebPreference('nodeIntegration'),
      typeofProcess: typeof global.process,
      windowOpenerIsNull
    });
    window.close();
  }
});

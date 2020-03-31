const { ipcRenderer } = require('electron');

setImmediate(function () {
  if (window.location.toString() === 'bar://page/') {
    const windowOpenerIsNull = window.opener == null;
    ipcRenderer.send('answer', process.argv, typeof global.process, windowOpenerIsNull);
    window.close();
  }
});

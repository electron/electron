(function () {
  const { ipcRenderer } = require('electron');
  // ipcRenderer -> IpcRenderer.prototype -> EventEmitter.prototype
  const eventEmitterPrototype = Object.getPrototypeOf(Object.getPrototypeOf(ipcRenderer));
  ipcRenderer.send('answer', Object.getOwnPropertyNames(eventEmitterPrototype).sort());
})();

(function () {
  const { EventEmitter } = require('events');
  const emitter = new EventEmitter();
  let rendererEventEmitterProperties = '';
  let currentObj = emitter;
  do {
    Object.getOwnPropertyNames(currentObj).map(property => { rendererEventEmitterProperties += property + ';'; });
  } while ((currentObj = Object.getPrototypeOf(currentObj)));
  const { ipcRenderer } = require('electron');
  ipcRenderer.send('answer', rendererEventEmitterProperties);
})();

(function () {
  const { EventEmitter } = require('node:events');
  const emitter = new EventEmitter();
  const rendererEventEmitterProperties = [];
  let currentObj = emitter;
  do {
    rendererEventEmitterProperties.push(...Object.getOwnPropertyNames(currentObj));
  } while ((currentObj = Object.getPrototypeOf(currentObj)));
  const { ipcRenderer } = require('electron');
  ipcRenderer.send('answer', rendererEventEmitterProperties);
})();

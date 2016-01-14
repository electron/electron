var EventEmitter, deprecate, ipc, ipcRenderer, method, ref,
  slice = [].slice;

ref = require('electron'), ipcRenderer = ref.ipcRenderer, deprecate = ref.deprecate;

EventEmitter = require('events').EventEmitter;

// This module is deprecated, we mirror everything from ipcRenderer.
deprecate.warn('ipc module', 'require("electron").ipcRenderer');

// Routes events of ipcRenderer.
ipc = new EventEmitter;

ipcRenderer.emit = function() {
  var args, channel, event;
  channel = arguments[0], event = arguments[1], args = 3 <= arguments.length ? slice.call(arguments, 2) : [];
  ipc.emit.apply(ipc, [channel].concat(slice.call(args)));
  return EventEmitter.prototype.emit.apply(ipcRenderer, arguments);
};

// Deprecated.
for (method in ipcRenderer) {
  if (method.startsWith('send')) {
    ipc[method] = ipcRenderer[method];
  }
}

deprecate.rename(ipc, 'sendChannel', 'send');

deprecate.rename(ipc, 'sendChannelSync', 'sendSync');

module.exports = ipc;

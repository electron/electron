var EventEmitter, binding, ipcRenderer, v8Util,
  slice = [].slice;

EventEmitter = require('events').EventEmitter;

binding = process.atomBinding('ipc');

v8Util = process.atomBinding('v8_util');


/* Created by init.coffee. */

ipcRenderer = v8Util.getHiddenValue(global, 'ipc');

ipcRenderer.send = function() {
  var args;
  args = 1 <= arguments.length ? slice.call(arguments, 0) : [];
  return binding.send('ipc-message', slice.call(args));
};

ipcRenderer.sendSync = function() {
  var args;
  args = 1 <= arguments.length ? slice.call(arguments, 0) : [];
  return JSON.parse(binding.sendSync('ipc-message-sync', slice.call(args)));
};

ipcRenderer.sendToHost = function() {
  var args;
  args = 1 <= arguments.length ? slice.call(arguments, 0) : [];
  return binding.send('ipc-message-host', slice.call(args));
};

module.exports = ipcRenderer;

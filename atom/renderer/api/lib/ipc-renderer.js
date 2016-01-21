const binding = process.atomBinding('ipc');
const v8Util = process.atomBinding('v8_util');

var slice = [].slice;

// Created by init.coffee.
const ipcRenderer = v8Util.getHiddenValue(global, 'ipc');

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

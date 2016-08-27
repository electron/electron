var atom = requireNative('atom').GetBinding()
var EventEmitter = require('event_emitter').EventEmitter
var ipc = atom.ipc

var ipcRenderer = atom.v8.getHiddenValue('ipc')
if (!ipcRenderer) {
  var slice = [].slice
  ipcRenderer = new EventEmitter

  ipcRenderer.send = function () {
    var args
    args = 1 <= arguments.length ? slice.call(arguments, 0) : []
    return ipc.send('ipc-message', slice.call(args))
  }

  ipcRenderer.sendSync = function () {
    var args
    args = 1 <= arguments.length ? slice.call(arguments, 0) : []
    return JSON.parse(ipc.sendSync('ipc-message-sync', slice.call(args)))
  }

  ipcRenderer.sendToHost = function () {
    var args
    args = 1 <= arguments.length ? slice.call(arguments, 0) : []
    return ipc.send('ipc-message-host', slice.call(args))
  }

  ipcRenderer.emit = function () {
    arguments[1].sender = ipcRenderer
    return EventEmitter.prototype.emit.apply(ipcRenderer, arguments)
  }
  atom.v8.setHiddenValue('ipc', ipcRenderer)
}

exports.on = ipcRenderer.on.bind(ipcRenderer);
exports.once = ipcRenderer.once.bind(ipcRenderer);
exports.send = ipcRenderer.send.bind(ipcRenderer);
exports.sendSync = ipcRenderer.sendSync.bind(ipcRenderer);
exports.sendToHost = ipcRenderer.sendToHost.bind(ipcRenderer);
exports.emit = ipcRenderer.emit.bind(ipcRenderer);

var atom = requireNative('atom').GetBinding();
var EventEmitter = require('event_emitter').EventEmitter;
var ipc = atom.ipc;
var webFrame = atom.web_frame.webFrame;

if (!atom.v8_util.getHiddenValue(atom, 'ipcRenderer')) {
  var slice = [].slice;

  var ipcRenderer = new EventEmitter;

  ipcRenderer.send = function () {
    var args;
    args = 1 <= arguments.length ? slice.call(arguments, 0) : [];
    return ipc.send('ipc-message', slice.call(args));
  };

  ipcRenderer.sendSync = function () {
    var args;
    args = 1 <= arguments.length ? slice.call(arguments, 0) : [];
    return JSON.parse(ipc.sendSync('ipc-message-sync', slice.call(args)));
  };

  ipcRenderer.sendToHost = function () {
    var args;
    args = 1 <= arguments.length ? slice.call(arguments, 0) : [];
    return ipc.send('ipc-message-host', slice.call(args));
  };

  ipcRenderer.emit = function () {
    arguments[1].sender = ipcRenderer;
    return EventEmitter.prototype.emit.apply(ipcRenderer, arguments);
  };

  ipcRenderer.on('ELECTRON_INTERNAL_RENDERER_WEB_FRAME_METHOD', function(event, method, args) {
    webFrame[method].apply(webFrame, args);
  });

  ipcRenderer.on('ELECTRON_INTERNAL_RENDERER_ASYNC_WEB_FRAME_METHOD', (event, requestId, method, args) => {
    const responseCallback = function(result) {
      event.sender.send(`ELECTRON_INTERNAL_BROWSER_ASYNC_WEB_FRAME_RESPONSE_${requestId}`, result);
    };
    args.push(responseCallback);
    webFrame[method].apply(webFrame, args);
  });

  atom.v8_util.setHiddenValue(atom, 'ipcRenderer', ipcRenderer);
}

var ipcRenderer = atom.v8_util.getHiddenValue(atom, 'ipcRenderer');

exports.on = ipcRenderer.on.bind(ipcRenderer);
exports.once = ipcRenderer.once.bind(ipcRenderer);
exports.send = ipcRenderer.send.bind(ipcRenderer);
exports.sendSync = ipcRenderer.sendSync.bind(ipcRenderer);
exports.sendToHost = ipcRenderer.sendToHost.bind(ipcRenderer);
exports.emit = ipcRenderer.emit.bind(ipcRenderer);

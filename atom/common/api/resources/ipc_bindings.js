var ipc = require('ipc_utils')

exports.didCreateDocumentElement = function() {
  window.alert = function(message, title) {
    return ipc.send('window-alert', message, title);
  };

  window.confirm = function(message, title) {
    return ipc.sendSync('window-confirm', message, title);
  };

  // copied from override.js
  // TODO(bridiver) is this even necessary? I don't think we need
  // or want these navigation controller hacks for nodeless renderers
  var sendHistoryOperation = function() {
    var args;
    args = 1 <= arguments.length ? slice.call(arguments, 0) : [];
    return ipc.send.apply(ipc, ['ATOM_SHELL_NAVIGATION_CONTROLLER'].concat(slice.call(args)));
  };

  window.history.back = function() {
    return sendHistoryOperation('goBack');
  };

  window.history.forward = function() {
    return sendHistoryOperation('goForward');
  };

  window.history.go = function(offset) {
    return sendHistoryOperation('goToOffset', offset);
  };
};

exports.binding = ipc;

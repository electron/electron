var ipc = require('ipc_utils');
var process = requireNative('process');
var extensionId = process.GetExtensionId();

var binding = {
  removeAll: function () {
    ipc.send('chrome-context-menu-remove-all', extensionId)
  },
  create: function (properties, cb) {
    //TODO(bridver) finish this
    ipc.send('chrome-context-menu-create', extensionId, properties)
    if (cb)
      cb();
  }
}

exports.binding = binding;

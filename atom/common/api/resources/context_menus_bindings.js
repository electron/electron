var ipc = require('ipc_utils');
var process = requireNative('process');
var extensionId = process.GetExtensionId();

var binding = {
  removeAll: function () {
    console.warn('chrome.contentMenus.removeAll is not supported yet')
  },
  create: function (properties, cb) {
    console.warn('chrome.contentMenus.create is not supported yet')
    cb && cb()
  }
}

exports.binding = binding;

var ipc = require('ipc_utils');
var runtimeNatives = requireNative('runtime');
var manifest = runtimeNatives.GetManifest();
var process = requireNative('process');
var extensionId = process.GetExtensionId();

var binding = {
  onClicked: {
    addListener: function (cb) {
      ipc.send('register-chrome-browser-action', extensionId,
                                    manifest.browser_action.default_title)
      ipc.on('chrome-browser-action-clicked', function(evt, tab) {
        cb(tab)
      })
    }
  },
  setPopup: function (details) {
    ipc.send('chrome-browser-action-set-popup', extensionId, details)
  },
  setTitle: function (details) {
    ipc.send('chrome-browser-action-set-title', extensionId, details)
  },
  setIcon: function (details, cb) {
    ipc.send('chrome-browser-action-set-icon', extensionId, details)
    cb && cb()
  },
  setBadgeText: function (details) {
    ipc.send('chrome-browser-action-set-title', extensionId, details)
  }
}

exports.binding = binding

var ipc = require('ipc_utils');
var runtimeNatives = requireNative('runtime');
var manifest = runtimeNatives.GetManifest();
var process = requireNative('process');
var extensionId = process.GetExtensionId();

var onClicked = {
  addListener: function(cb) {
    ipc.send('register-chrome-browser-action', extensionId,
                                  manifest.browser_action.default_title);
    ipc.on('chrome-browser-action-clicked', function(evt, tab) {
      cb(tab);
    });
  }
}

exports.binding = {
  onClicked: onClicked
}

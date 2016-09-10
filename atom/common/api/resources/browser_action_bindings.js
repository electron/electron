var ipc = require('ipc_utils');
var runtimeNatives = requireNative('runtime');
var manifest = runtimeNatives.GetManifest();
var process = requireNative('process');
var extensionId = process.GetExtensionId();

var id = 0

var binding = {
  onClicked: {
    addListener: function (cb) {
      ipc.on('chrome-browser-action-clicked', function(evt, tab) {
        cb(tab)
      })
    },
    removeListener: function (cb) {
      // TODO
      // ipc.off('chrome-browser-action-clicked', cb)
    }
  },
  setPopup: function (details) {
    ipc.send('chrome-browser-action-set-popup', extensionId, details)
  },
  getPopup: function (details, cb) {
    var responseId = ++id
    ipc.once('chrome-browser-action-get-popup-response-' + responseId, function(evt, details) {
      cb(details)
    })
    ipc.send('chrome-browser-action-get-popup', responseId, details)
  },
  setTitle: function (details) {
    ipc.send('chrome-browser-action-set-title', extensionId, details)
  },
  getTitle: function (details, cb) {
    var responseId = ++id
    ipc.once('chrome-browser-action-get-title-response-' + responseId, function(evt, result) {
      cb(result)
    })
    ipc.send('chrome-browser-action-get-title', responseId, details)
  },
  setIcon: function (details, cb) {
    if (details.imageData) {
      // TODO(bridiver) - support imageData somehow
      console.warn('chrome.browserAction.setIcon imageData is not supported yet')
      return
    }
    var responseId = ++id
    ipc.once('chrome-browser-action-set-icon-response-' + responseId, function(evt) {
      cb && cb()
    })
    ipc.send('chrome-browser-action-set-icon', responseId, extensionId, details)
  },
  setBadgeText: function (details) {
    ipc.send('chrome-browser-action-set-badge-text', extensionId, details)
  },
  getBadgeText: function (details, cb) {
    var responseId = ++id
    ipc.once('chrome-browser-action-get-badge-text-response-' + responseId, function(evt, details) {
      cb(details)
    })
    ipc.send('chrome-browser-action-get-badge-text', responseId, details)
  },
  setBadgeBackgroundColor: function (details) {
    ipc.send('chrome-browser-action-set-badge-background-color', extensionId, details)
  },
  getBadgeBackgroundColor: function (details, cb) {
    var responseId = ++id
    ipc.once('chrome-browser-action-get-badge-text-response-' + responseId, function(evt, details) {
      cb(details)
    })
    ipc.send('chrome-browser-action-get-badge-text', responseId, details)
  }
}

exports.binding = binding

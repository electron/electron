var binding = require('binding').Binding.create('tabs');

var atom = requireNative('atom').GetBinding();
var messaging = require('messaging');
var tabsNatives = requireNative('tabs');
var OpenChannelToTab = tabsNatives.OpenChannelToTab;
var process = requireNative('process');
var sendRequestIsDisabled = process.IsSendRequestDisabled();
var forEach = require('utils').forEach;
var extensionId = process.GetExtensionId();
var ipc = require('ipc_utils')

var id = 0;

var bindings = {
  connect: function(tabId, connectInfo) {
    var name = '';
    var frameId = -1;
    if (connectInfo) {
      name = connectInfo.name || name;
      frameId = connectInfo.frameId;
      if (typeof frameId == 'undefined' || frameId < 0)
        frameId = -1;
    }
    var portId = OpenChannelToTab(tabId, frameId, extensionId, name);
    return messaging.createPort(portId, name);
  },

  sendRequest: function(tabId, request, responseCallback) {
    if (sendRequestIsDisabled)
      throw new Error(sendRequestIsDisabled);
    var port = bindings.connect(tabId, {name: messaging.kRequestChannel});
    messaging.sendMessageImpl(port, request, responseCallback);
  },

  sendMessage: function(tabId, message, options, responseCallback) {
    var connectInfo = {
      name: messaging.kMessageChannel
    };
    if (options && typeof options === 'object') {
      forEach(options, function(k, v) {
        connectInfo[k] = v;
      });
    } else if (typeof options === 'function') {
      responseCallback = options;
    }

    var port = bindings.connect(tabId, connectInfo);
    messaging.sendMessageImpl(port, message, responseCallback);
  },

  onUpdated: {
    addListener: function (cb) {
      ipc.send('register-chrome-tabs-updated', extensionId);
      ipc.on('chrome-tabs-updated', function(evt, tabId, changeInfo, tab) {
        cb(tabId, changeInfo, tab);
      });
    }
  },

  onCreated: {
    addListener: function (cb) {
      ipc.send('register-chrome-tabs-created', extensionId);
      ipc.on('chrome-tabs-created', function(evt, tab) {
        cb(tab);
      });
    }
  },

  onRemoved: {
    addListener: function (cb) {
      ipc.send('register-chrome-tabs-removed', extensionId);
      ipc.on('chrome-tabs-removed', function (evt, tabId, removeInfo) {
        cb(tabId, removeInfo);
      });
    }
  },

  onActivated: {
    addListener: function (cb) {
      ipc.send('register-chrome-tabs-activated', extensionId)
      ipc.on('chrome-tabs-activated', function (evt, tabId, activeInfo) {
        cb(activeInfo)
      })
    }
  },

  onSelectionChanged: {
    addListener: function (cb) {
      ipc.send('register-chrome-tabs-activated', extensionId)
      ipc.on('chrome-tabs-activated', function (evt, tabId, selectInfo) {
        cb(tabId, selectInfo)
      })
    }
  },

  onActiveChanged: {
    addListener: function (cb) {
      ipc.send('register-chrome-tabs-activated', extensionId)
      ipc.on('chrome-tabs-activated', function (evt, tabId, selectInfo) {
        cb(tabId, selectInfo)
      })
    }
  },

  captureVisibleTab: function (windowId, options, cb) {
    if (typeof windowId === 'function')
      cb = windowId;
    if (typeof windowId === 'object')
      options = windowId
    if (typeof options === 'function')
      cb = options

    // return an empty image url
    cb('data:image/gif;base64,R0lGODlhAQABAAAAACH5BAEKAAEALAAAAAABAAEAAAICTAEAOw==')
  },

  getSelected: function (windowId, cb) {
    if (typeof windowId === 'function') {
      cb = windowId
      windowId = -2
    } else if (!windowId) {
      windowId = -2
    }

    var wrapper = function(tabs) {
      cb(tabs[0])
    }

    bindings.query({windowId: windowId, active: true}, wrapper)
  },

  get: function(tabId, cb) {
    var responseId = ++id
    ipc.once('chrome-tabs-get-response-' + responseId, function (evt, tab) {
      cb(tab)
    })
    ipc.send('chrome-tabs-get', responseId, {})
  },

  query: function(queryInfo, cb) {
    var responseId = ++id
    ipc.once('chrome-tabs-query-response-' + responseId, function (evt, tab) {
      cb(tab)
    })
    ipc.send('chrome-tabs-query', responseId, queryInfo)
  },

  update: function (tabId, updateProperties, cb) {
    var responseId = ++id
    cb && ipc.once('chrome-tabs-update-response-' + responseId, function (evt, tab) {
      cb(tab)
    })
    ipc.send('chrome-tabs-update', responseId, tabId, updateProperties)
  },

  remove: function (tabIds, cb) {
    var responseId = ++id
    cb && ipc.once('chrome-tabs-remove-response-' + responseId, function (evt) {
      cb()
    })
    ipc.send('chrome-tabs-remove', responseId, tabIds)
  },

  create: function (createProperties, cb) {
    var responseId = ++id
    cb && ipc.once('chrome-tabs-create-response-' + responseId, function (evt, tab) {
      cb(tab)
    })
    ipc.send('chrome-tabs-create', responseId, createProperties)
  },

  executeScript: function (tabId, details, cb) {
    if (typeof tabId !== 'number') {
      throw 'executeScript: must specify tab id'
    }
    if (cb) {
      // TODO(bridiver) implement callback
      throw 'executeScript: `callback` is not supported'
    }
    ipc.send('chrome-tabs-execute-script', extensionId, tabId, details)
  }
}

exports.binding = bindings;

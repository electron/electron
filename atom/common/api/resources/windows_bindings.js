var ipc = require('ipc_utils')

var id = 1;

var binding = {
  getCurrent: function () {
    var cb, getInfo;
    if (arguments.length == 1) {
      cb = arguments[0]
    } else {
      getInfo = arguments[0]
      cb = arguments[1]
    }

    var responseId = ++id
    ipc.once('chrome-windows-get-current-response-' + responseId, function(evt, win) {
      cb(win)
    })
    ipc.send('chrome-windows-get-current', responseId, getInfo)
  },

  getAll: function (getInfo, cb) {
    if (arguments.length == 1) {
      cb = arguments[0]
    } else {
      getInfo = arguments[0]
      cb = arguments[1]
    }

    var responseId = ++id
    ipc.once('chrome-windows-get-all-response-' + responseId, function(evt, win) {
      cb(win)
    })
    ipc.send('chrome-windows-get-all', responseId, getInfo)
  },

  create: function (createData, cb) {
    console.warn('chrome.windows.create is not supported yet')
  },

  update: function (windowId, updateInfo, cb) {
    var responseId = ++id
    cb && ipc.once('chrome-windows-update-response-' + responseId, function (evt, win) {
      cb(win)
    })
    ipc.send('chrome-windows-update', responseId, windowId, updateInfo)
  },

  WINDOW_ID_NONE: -1,
  WINDOW_ID_CURRENT: -2
};

exports.binding = binding;

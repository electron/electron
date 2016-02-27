
// var atom = requireNative('atom').GetBinding();

// var ipc = atom.v8_util.getHiddenValue(atom, 'ipcRenderer');

// chrome.windows.getCurrent({}
// create
var ipc = require('ipc_utils')

var id = 1;

var binding = {
  getCurrent: function() {
    var cb, getInfo;
    if (arguments.length == 1) {
      cb = arguments[0]
    } else {
      getInfo = arguments[0]
      cb = arguments[1]
    }

    var responseId = ++id;
    ipc.once('chrome-windows-get-current-response-' + responseId, function(evt, win) {
      cb(win);
    })
    ipc.send('chrome-windows-get-current', responseId, getInfo);
  },
  create: function(createData, cb) {
    var responseId = ++id;
    ipc.once('chrome-windows-create-response-' + responseId, function(evt, win) {
      cb && cb(win);
    })
    ipc.send('chrome-windows-create', responseId, tabId, updateProperties);
  },
  WINDOW_ID_CURRENT: -2
};

exports.binding = binding;

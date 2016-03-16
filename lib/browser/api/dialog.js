'use strict';

const app = require('electron').app;
const BrowserWindow = require('electron').BrowserWindow;
const binding = process.atomBinding('dialog');
const v8Util = process.atomBinding('v8_util');

var slice = [].slice;
var includes = [].includes;

var fileDialogProperties = {
  openFile: 1 << 0,
  openDirectory: 1 << 1,
  multiSelections: 1 << 2,
  createDirectory: 1 << 3
};

var messageBoxTypes = ['none', 'info', 'warning', 'error', 'question'];

var messageBoxOptions = {
  noLink: 1 << 0
};

var parseArgs = function(window, options, callback) {
  if (!(window === null || (window != null ? window.constructor : void 0) === BrowserWindow)) {
    // Shift.
    callback = options;
    options = window;
    window = null;
  }
  if ((callback == null) && typeof options === 'function') {
    // Shift.
    callback = options;
    options = null;
  }
  return [window, options, callback];
};

var checkAppInitialized = function() {
  if (!app.isReady()) {
    throw new Error('dialog module can only be used after app is ready');
  }
};

module.exports = {
  showOpenDialog: function() {
    const args = 1 <= arguments.length ? slice.call(arguments, 0) : [];
    checkAppInitialized();

    const parsedArgs = parseArgs.apply(null, args);
    const window = parsedArgs[0];
    const options = parsedArgs[1] != null ? parsedArgs[1] : { title: 'Open', properties: ['openFile'] };
    const callback = parsedArgs[2];
    if (options.properties == null) {
      options.properties = ['openFile'];
    }
    if (!Array.isArray(options.properties)) {
      throw new TypeError('Properties need to be array');
    }
    let properties = 0;
    for (let prop in fileDialogProperties) {
      let value = fileDialogProperties[prop];
      if (includes.call(options.properties, prop)) {
        properties |= value;
      }
    }
    if (options.title == null) {
      options.title = '';
    }
    if (options.defaultPath == null) {
      options.defaultPath = '';
    }
    if (options.filters == null) {
      options.filters = [];
    }
    const wrappedCallback = typeof callback === 'function' ? function(success, result) {
      return callback(success ? result : void 0);
    } : null;
    return binding.showOpenDialog(String(options.title), String(options.defaultPath), options.filters, properties, window, wrappedCallback);
  },
  showSaveDialog: function() {
    const args = 1 <= arguments.length ? slice.call(arguments, 0) : [];
    checkAppInitialized();

    const parsedArgs = parseArgs.apply(null, args);
    const window = parsedArgs[0];
    const options = parsedArgs[1] != null ? parseArgs[1] : { title: 'Save' };
    const callback = parsedArgs[2];
    if (options.title == null) {
      options.title = '';
    }
    if (options.defaultPath == null) {
      options.defaultPath = '';
    }
    if (options.filters == null) {
      options.filters = [];
    }
    const wrappedCallback = typeof callback === 'function' ? function(success, result) {
      return callback(success ? result : void 0);
    } : null;
    return binding.showSaveDialog(String(options.title), String(options.defaultPath), options.filters, window, wrappedCallback);
  },
  showMessageBox: function() {
    const args = 1 <= arguments.length ? slice.call(arguments, 0) : [];
    checkAppInitialized();

    const parsedArgs = parseArgs.apply(null, args);
    const window = parsedArgs[0];
    const options = parsedArgs[1] != null ? parsedArgs[1] : { type: 'none' };
    const callback = parsedArgs[2];
    if (options.type == null) {
      options.type = 'none';
    }
    const messageBoxType = messageBoxTypes.indexOf(options.type);
    if (!(messageBoxType > -1)) {
      throw new TypeError('Invalid message box type');
    }
    if (!Array.isArray(options.buttons)) {
      throw new TypeError('Buttons need to be array');
    }
    if (options.title == null) {
      options.title = '';
    }
    if (options.message == null) {
      options.message = '';
    }
    if (options.detail == null) {
      options.detail = '';
    }
    if (options.icon == null) {
      options.icon = null;
    }
    if (options.defaultId == null) {
      options.defaultId = -1;
    }

    // Choose a default button to get selected when dialog is cancelled.
    if (options.cancelId == null) {
      options.cancelId = 0;
      for (let i = 0; i = options.buttons.length; i++) {
        let text = options.buttons[i];
        let lowerCaseText = text.toLowerCase();
        if (lowerCaseText === 'cancel' || lowerCaseText === 'no') {
          options.cancelId = i;
          break;
        }
      }
    }
    const flags = options.noLink ? messageBoxOptions.noLink : 0;
    return binding.showMessageBox(messageBoxType, options.buttons, options.defaultId, options.cancelId, flags, options.title, options.message, options.detail, options.icon, window, callback);
  },
  showErrorBox: function() {
    var args;
    args = 1 <= arguments.length ? slice.call(arguments, 0) : [];
    return binding.showErrorBox.apply(binding, args);
  }
};

// Mark standard asynchronous functions.
['showMessageBox', 'showOpenDialog', 'showSaveDialog'].forEach(function(api) {
  v8Util.setHiddenValue(module.exports[api], 'asynchronous', true);
});

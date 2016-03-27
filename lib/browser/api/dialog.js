'use strict'

const app = require('electron').app
const BrowserWindow = require('electron').BrowserWindow
const binding = process.atomBinding('dialog')
const v8Util = process.atomBinding('v8_util')

var includes = [].includes

var fileDialogProperties = {
  openFile: 1 << 0,
  openDirectory: 1 << 1,
  multiSelections: 1 << 2,
  createDirectory: 1 << 3
}

var messageBoxTypes = ['none', 'info', 'warning', 'error', 'question']

var messageBoxOptions = {
  noLink: 1 << 0
}

var parseArgs = function (window, options, callback, ...args) {
  if (!(window === null || (window != null ? window.constructor : void 0) === BrowserWindow)) {
    // Shift.
    callback = options
    options = window
    window = null
  }

  if ((callback == null) && typeof options === 'function') {
    // Shift.
    callback = options
    options = null
  }

  // Fallback to using very last argument as the callback function
  if ((callback == null) && typeof args[args.length - 1] === 'function') {
    callback = args[args.length - 1]
  }

  return [window, options, callback]
}

var checkAppInitialized = function () {
  if (!app.isReady()) {
    throw new Error('dialog module can only be used after app is ready')
  }
}

module.exports = {
  setSheetOffset: function(window, offset) {
    return binding.setSheetOffset(window, offset)
  },

  showOpenDialog: function(...args) {
    var prop, properties, value, wrappedCallback
    checkAppInitialized()
    let [window, options, callback] = parseArgs.apply(null, args)
    if (options == null) {
      options = {
        title: 'Open',
        properties: ['openFile']
      }
    }
    if (options.properties == null) {
      options.properties = ['openFile']
    }
    if (!Array.isArray(options.properties)) {
      throw new TypeError('Properties must be an array')
    }
    properties = 0
    for (prop in fileDialogProperties) {
      value = fileDialogProperties[prop]
      if (includes.call(options.properties, prop)) {
        properties |= value
      }
    }
    if (options.title == null) {
      options.title = ''
    } else if (typeof options.title !== 'string') {
      throw new TypeError('Title must be a string')
    }
    if (options.defaultPath == null) {
      options.defaultPath = ''
    } else if (typeof options.defaultPath !== 'string') {
      throw new TypeError('Default path must be a string')
    }
    if (options.filters == null) {
      options.filters = []
    }
    wrappedCallback = typeof callback === 'function' ? function (success, result) {
      return callback(success ? result : void 0)
    } : null
    return binding.showOpenDialog(String(options.title), String(options.defaultPath), options.filters, properties, window, wrappedCallback)
  },

  showSaveDialog: function (...args) {
    var wrappedCallback
    checkAppInitialized()
    let [window, options, callback] = parseArgs.apply(null, args)
    if (options == null) {
      options = {
        title: 'Save'
      }
    }
    if (options.title == null) {
      options.title = ''
    } else if (typeof options.title !== 'string') {
      throw new TypeError('Title must be a string')
    }
    if (options.defaultPath == null) {
      options.defaultPath = ''
    } else if (typeof options.defaultPath !== 'string') {
      throw new TypeError('Default path must be a string')
    }
    if (options.filters == null) {
      options.filters = []
    }
    wrappedCallback = typeof callback === 'function' ? function (success, result) {
      return callback(success ? result : void 0)
    } : null
    return binding.showSaveDialog(String(options.title), String(options.defaultPath), options.filters, window, wrappedCallback)
  },

  showMessageBox: function (...args) {
    var flags, i, j, len, messageBoxType, ref2, ref3, text
    checkAppInitialized()
    let [window, options, callback] = parseArgs.apply(null, args)
    if (options == null) {
      options = {
        type: 'none'
      }
    }
    if (options.type == null) {
      options.type = 'none'
    }
    messageBoxType = messageBoxTypes.indexOf(options.type)
    if (!(messageBoxType > -1)) {
      throw new TypeError('Invalid message box type')
    }
    if (!Array.isArray(options.buttons)) {
      throw new TypeError('Buttons must be an array')
    }
    if (options.title == null) {
      options.title = ''
    } else if (typeof options.title !== 'string') {
      throw new TypeError('Title must be a string')
    }
    if (options.message == null) {
      options.message = ''
    } else if (typeof options.message !== 'string') {
      throw new TypeError('Message must be a string')
    }
    if (options.detail == null) {
      options.detail = ''
    } else if (typeof options.detail !== 'string') {
      throw new TypeError('Detail must be a string')
    }
    if (options.icon == null) {
      options.icon = null
    }
    if (options.defaultId == null) {
      options.defaultId = -1
    }

    // Choose a default button to get selected when dialog is cancelled.
    if (options.cancelId == null) {
      options.cancelId = 0
      ref2 = options.buttons
      for (i = j = 0, len = ref2.length; j < len; i = ++j) {
        text = ref2[i]
        if ((ref3 = text.toLowerCase()) === 'cancel' || ref3 === 'no') {
          options.cancelId = i
          break
        }
      }
    }
    flags = options.noLink ? messageBoxOptions.noLink : 0
    return binding.showMessageBox(messageBoxType, options.buttons, options.defaultId, options.cancelId, flags, options.title, options.message, options.detail, options.icon, window, callback)
  },

  showErrorBox: function (...args) {
    return binding.showErrorBox.apply(binding, args)
  }
}

// Mark standard asynchronous functions.
var ref1 = ['showMessageBox', 'showOpenDialog', 'showSaveDialog']
var j, len, api
for (j = 0, len = ref1.length; j < len; j++) {
  api = ref1[j]
  v8Util.setHiddenValue(module.exports[api], 'asynchronous', true)
}

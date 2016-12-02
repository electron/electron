'use strict'

const {app, BrowserWindow} = require('electron')
const binding = process.atomBinding('dialog')
const v8Util = process.atomBinding('v8_util')

const fileDialogProperties = {
  openFile: 1 << 0,
  openDirectory: 1 << 1,
  multiSelections: 1 << 2,
  createDirectory: 1 << 3,
  showHiddenFiles: 1 << 4
}

const messageBoxTypes = ['none', 'info', 'warning', 'error', 'question']

const messageBoxOptions = {
  noLink: 1 << 0
}

const parseArgs = function (window, options, callback, ...args) {
  if (window !== null && window.constructor !== BrowserWindow) {
    // Shift.
    [callback, options, window] = [options, window, null]
  }

  if ((callback == null) && typeof options === 'function') {
    // Shift.
    [callback, options] = [options, null]
  }

  // Fallback to using very last argument as the callback function
  const lastArgument = args[args.length - 1]
  if ((callback == null) && typeof lastArgument === 'function') {
    callback = lastArgument
  }

  return [window, options, callback]
}

const checkAppInitialized = function () {
  if (!app.isReady()) {
    throw new Error('dialog module can only be used after app is ready')
  }
}

module.exports = {
  showOpenDialog: function (...args) {
    checkAppInitialized()

    let [window, options, callback] = parseArgs(...args)

    if (options == null) {
      options = {
        title: 'Open',
        properties: ['openFile']
      }
    }

    let {buttonLabel, defaultPath, filters, properties, title} = options

    if (properties == null) {
      properties = ['openFile']
    }

    if (!Array.isArray(properties)) {
      throw new TypeError('Properties must be an array')
    }

    let dialogProperties = 0
    for (const prop in fileDialogProperties) {
      if (properties.includes(prop)) {
        dialogProperties |= fileDialogProperties[prop]
      }
    }

    if (title == null) {
      title = ''
    } else if (typeof title !== 'string') {
      throw new TypeError('Title must be a string')
    }

    if (buttonLabel == null) {
      buttonLabel = ''
    } else if (typeof buttonLabel !== 'string') {
      throw new TypeError('Button label must be a string')
    }

    if (defaultPath == null) {
      defaultPath = ''
    } else if (typeof defaultPath !== 'string') {
      throw new TypeError('Default path must be a string')
    }

    if (filters == null) {
      filters = []
    }

    const wrappedCallback = typeof callback === 'function' ? function (success, result) {
      return callback(success ? result : void 0)
    } : null
    return binding.showOpenDialog(title, buttonLabel, defaultPath, filters,
                                  dialogProperties, window, wrappedCallback)
  },

  showSaveDialog: function (...args) {
    checkAppInitialized()

    let [window, options, callback] = parseArgs(...args)

    if (options == null) {
      options = {
        title: 'Save'
      }
    }

    let {buttonLabel, defaultPath, filters, title} = options

    if (title == null) {
      title = ''
    } else if (typeof title !== 'string') {
      throw new TypeError('Title must be a string')
    }

    if (buttonLabel == null) {
      buttonLabel = ''
    } else if (typeof buttonLabel !== 'string') {
      throw new TypeError('Button label must be a string')
    }

    if (defaultPath == null) {
      defaultPath = ''
    } else if (typeof defaultPath !== 'string') {
      throw new TypeError('Default path must be a string')
    }

    if (filters == null) {
      filters = []
    }

    const wrappedCallback = typeof callback === 'function' ? function (success, result) {
      return callback(success ? result : void 0)
    } : null
    return binding.showSaveDialog(title, buttonLabel, defaultPath, filters,
                                  window, wrappedCallback)
  },

  showMessageBox: function (...args) {
    checkAppInitialized()

    let [window, options, callback] = parseArgs(...args)

    if (options == null) {
      options = {
        type: 'none'
      }
    }

    let {buttons, cancelId, defaultId, detail, icon, message, title, type} = options

    if (type == null) {
      type = 'none'
    }

    const messageBoxType = messageBoxTypes.indexOf(type)
    if (messageBoxType === -1) {
      throw new TypeError('Invalid message box type')
    }

    if (!Array.isArray(buttons)) {
      throw new TypeError('Buttons must be an array')
    }

    if (title == null) {
      title = ''
    } else if (typeof title !== 'string') {
      throw new TypeError('Title must be a string')
    }

    if (message == null) {
      message = ''
    } else if (typeof message !== 'string') {
      throw new TypeError('Message must be a string')
    }

    if (detail == null) {
      detail = ''
    } else if (typeof detail !== 'string') {
      throw new TypeError('Detail must be a string')
    }

    if (icon == null) {
      icon = null
    }

    if (defaultId == null) {
      defaultId = -1
    }

    // Choose a default button to get selected when dialog is cancelled.
    if (cancelId == null) {
      cancelId = 0
      for (let i = 0; i < buttons.length; i++) {
        const text = buttons[i].toLowerCase()
        if (text === 'cancel' || text === 'no') {
          cancelId = i
          break
        }
      }
    }

    const flags = options.noLink ? messageBoxOptions.noLink : 0
    return binding.showMessageBox(messageBoxType, buttons, defaultId, cancelId,
                                  flags, title, message, detail, icon, window,
                                  callback)
  },

  showErrorBox: function (...args) {
    return binding.showErrorBox(...args)
  }
}

// Mark standard asynchronous functions.
v8Util.setHiddenValue(module.exports.showMessageBox, 'asynchronous', true)
v8Util.setHiddenValue(module.exports.showOpenDialog, 'asynchronous', true)
v8Util.setHiddenValue(module.exports.showSaveDialog, 'asynchronous', true)

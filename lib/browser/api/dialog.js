'use strict'

const {app, BrowserWindow} = require('electron')
const binding = process.atomBinding('dialog')
const v8Util = process.atomBinding('v8_util')

const fileDialogProperties = {
  openFile: 1 << 0,
  openDirectory: 1 << 1,
  multiSelections: 1 << 2,
  createDirectory: 1 << 3,
  showHiddenFiles: 1 << 4,
  promptToCreate: 1 << 5,
  noResolveAliases: 1 << 6,
  treatPackageAsDirectory: 1 << 7
}

const messageBoxTypes = ['none', 'info', 'warning', 'error', 'question']

const messageBoxOptions = {
  noLink: 1 << 0
}

const parseArgs = function (window, options, callback, ...args) {
  if (window != null && window.constructor !== BrowserWindow) {
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

const normalizeAccessKey = (text) => {
  if (typeof text !== 'string') return text

  // macOS does not have access keys so remove single ampersands
  // and replace double ampersands with a single ampersand
  if (process.platform === 'darwin') {
    return text.replace(/&(&?)/g, '$1')
  }

  // Linux uses a single underscore as an access key prefix so escape
  // existing single underscores with a second underscore, replace double
  // ampersands with a single ampersand, and replace a single ampersand with
  // a single underscore
  if (process.platform === 'linux') {
    return text.replace(/_/g, '__').replace(/&(.?)/g, (match, after) => {
      if (after === '&') return after
      return `_${after}`
    })
  }

  return text
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

    let {buttonLabel, defaultPath, filters, properties, title, message, securityScopedBookmarks = false} = options

    if (properties == null) {
      properties = ['openFile']
    } else if (!Array.isArray(properties)) {
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

    if (message == null) {
      message = ''
    } else if (typeof message !== 'string') {
      throw new TypeError('Message must be a string')
    }

    const wrappedCallback = typeof callback === 'function' ? function (success, result, bookmarkData) {
      return success ? callback(result, bookmarkData) : callback()
    } : null
    const settings = {title, buttonLabel, defaultPath, filters, message, securityScopedBookmarks, window}
    settings.properties = dialogProperties
    return binding.showOpenDialog(settings, wrappedCallback)
  },

  showSaveDialog: function (...args) {
    checkAppInitialized()

    let [window, options, callback] = parseArgs(...args)

    if (options == null) {
      options = {
        title: 'Save'
      }
    }

    let {buttonLabel, defaultPath, filters, title, message, securityScopedBookmarks = false, nameFieldLabel, showsTagField} = options

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

    if (message == null) {
      message = ''
    } else if (typeof message !== 'string') {
      throw new TypeError('Message must be a string')
    }

    if (nameFieldLabel == null) {
      nameFieldLabel = ''
    } else if (typeof nameFieldLabel !== 'string') {
      throw new TypeError('Name field label must be a string')
    }

    if (showsTagField == null) {
      showsTagField = true
    }

    const wrappedCallback = typeof callback === 'function' ? function (success, result, bookmarkData) {
      return success ? callback(result, bookmarkData) : callback()
    } : null
    const settings = {title, buttonLabel, defaultPath, filters, message, securityScopedBookmarks, nameFieldLabel, showsTagField, window}
    return binding.showSaveDialog(settings, wrappedCallback)
  },

  showMessageBox: function (...args) {
    checkAppInitialized()

    let [window, options, callback] = parseArgs(...args)

    if (options == null) {
      options = {
        type: 'none'
      }
    }

    let {
      buttons, cancelId, checkboxLabel, checkboxChecked, defaultId, detail,
      icon, message, title, type
    } = options

    if (type == null) {
      type = 'none'
    }

    const messageBoxType = messageBoxTypes.indexOf(type)
    if (messageBoxType === -1) {
      throw new TypeError('Invalid message box type')
    }

    if (buttons == null) {
      buttons = []
    } else if (!Array.isArray(buttons)) {
      throw new TypeError('Buttons must be an array')
    }

    if (options.normalizeAccessKeys) {
      buttons = buttons.map(normalizeAccessKey)
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

    checkboxChecked = !!checkboxChecked

    if (checkboxLabel == null) {
      checkboxLabel = ''
    } else if (typeof checkboxLabel !== 'string') {
      throw new TypeError('checkboxLabel must be a string')
    }

    if (icon == null) {
      icon = null
    }

    if (defaultId == null) {
      defaultId = -1
    }

    // Choose a default button to get selected when dialog is cancelled.
    if (cancelId == null) {
      // If the defaultId is set to 0, ensure the cancel button is a different index (1)
      cancelId = (defaultId === 0 && buttons.length > 1) ? 1 : 0
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
                                  flags, title, message, detail, checkboxLabel,
                                  checkboxChecked, icon, window, callback)
  },

  showErrorBox: function (...args) {
    return binding.showErrorBox(...args)
  },

  showCertificateTrustDialog: function (...args) {
    let [window, options, callback] = parseArgs(...args)

    if (options == null || typeof options !== 'object') {
      throw new TypeError('options must be an object')
    }

    let {certificate, message} = options
    if (certificate == null || typeof certificate !== 'object') {
      throw new TypeError('certificate must be an object')
    }

    if (message == null) {
      message = ''
    } else if (typeof message !== 'string') {
      throw new TypeError('message must be a string')
    }

    return binding.showCertificateTrustDialog(window, certificate, message, callback)
  }
}

// Mark standard asynchronous functions.
v8Util.setHiddenValue(module.exports.showMessageBox, 'asynchronous', true)
v8Util.setHiddenValue(module.exports.showOpenDialog, 'asynchronous', true)
v8Util.setHiddenValue(module.exports.showSaveDialog, 'asynchronous', true)

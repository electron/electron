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

const Type = {
  OPEN: 'open',
  SAVE: 'save'
}

const messageBoxTypes = ['none', 'info', 'warning', 'error', 'question']

const messageBoxOptions = {
  noLink: 1 << 0
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

const setupDialog = (type, {
    window,
    buttonLabel = '',
    defaultPath = '',
    filters = [],
    properties = ['openFile'],
    title = '',
    message = '',
    securityScopedBookmarks = false,
    callback,
    nameFieldLabel = '',
    showsTagField = true
  }) => {
  if (!Array.isArray(properties)) {
    throw new TypeError('Properties must be an array')
  }

  let dialogProperties = 0
  // set up dialogProperties for showOpenDialog
  if (type === Type.OPEN) {
    for (const prop in fileDialogProperties) {
      if (properties.includes(prop)) {
        dialogProperties |= fileDialogProperties[prop]
      }
    }
  }

  // check parameter types
  if (typeof title !== 'string') {
    throw new TypeError('Title must be a string')
  }
  if (typeof buttonLabel !== 'string') {
    throw new TypeError('Button label must be a string')
  }
  if (typeof defaultPath !== 'string') {
    throw new TypeError('Default path must be a string')
  }
  if (typeof message !== 'string') {
    throw new TypeError('Message must be a string')
  }

  // set up extra parameters for showSaveDialog
  if (type === Type.SAVE) {
    if (typeof nameFieldLabel !== 'string') {
      throw new TypeError('Name field label must be a string')
    }
    if (typeof showsTagField !== 'boolean') {
      throw new TypeError('Shows tag option must be a boolean')
    }
  }

  let wrappedCallback = null
  if (typeof callback === 'function') {
    wrappedCallback = (success, result, bookmarkData) => success ? callback(result, bookmarkData) : callback()
  }

  let settings
  if (type === Type.OPEN) {
    settings = {window, buttonLabel, defaultPath, filters, properties, title, message, securityScopedBookmarks, callback}
  } else {
    settings = {window, buttonLabel, defaultPath, filters, title, message, securityScopedBookmarks, callback, nameFieldLabel, showsTagField}
  }

  return {settings, dialogProperties, wrappedCallback}
}

module.exports = {
  showOpenDialog: function (options = {
    title: 'Open',
    properties: ['openFile']
  }) {
    checkAppInitialized()

    const {settings, dialogProperties, wrappedCallback} = setupDialog(Type.OPEN, options)
    settings.properties = dialogProperties
    return binding.showOpenDialog(settings, wrappedCallback)
  },

  showSaveDialog: function (options = {title: 'Save'}) {
    checkAppInitialized()

    const {settings, wrappedCallback} = setupDialog(Type.SAVE, options)
    return binding.showSaveDialog(settings, wrappedCallback)
  },

  showMessageBox: ({
      window,
      buttons = [],
      cancelId,
      checkboxLabel = '',
      checkboxChecked,
      defaultId = -1,
      detail = '',
      icon,
      normalizeAccessKeys,
      noLink,
      message = '',
      title = '',
      type = 'none',
      callback
    }) => {
    checkAppInitialized()

    // check type value validity
    const messageBoxType = messageBoxTypes.indexOf(type)
    if (messageBoxType === -1) {
      throw new TypeError('Invalid message box type')
    }
    if (!Array.isArray(buttons)) {
      throw new TypeError('Buttons must be an array')
    }
    if (typeof title !== 'string') {
      throw new TypeError('Title must be a string')
    }
    if (typeof message !== 'string') {
      throw new TypeError('Message must be a string')
    }
    if (typeof detail !== 'string') {
      throw new TypeError('Detail must be a string')
    }
    if (typeof checkboxLabel !== 'string') {
      throw new TypeError('checkboxLabel must be a string')
    }

    if (normalizeAccessKeys) {
      buttons = buttons.map(normalizeAccessKey)
    }

    checkboxChecked = !!checkboxChecked

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

    const flags = noLink ? messageBoxOptions.noLink : 0

    return binding.showMessageBox(messageBoxType, buttons, defaultId, cancelId,
                                  flags, title, message, detail, checkboxLabel,
                                  checkboxChecked, icon, window, callback)
  },

  showErrorBox: function (...args) {
    return binding.showErrorBox(...args)
  },

  showCertificateTrustDialog: function (args) {
    if (args == null || typeof args !== 'object') {
      throw new TypeError('options must be an object')
    }

    let {window, certificate, message, callback} = args

    if (certificate == null || typeof certificate !== 'object') {
      throw new TypeError('certificate must be an object')
    }
    if (callback == null || typeof callback !== 'function') {
      throw new TypeError('You must pass in a valid callback')
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

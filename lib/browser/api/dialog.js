'use strict'

const {app} = require('electron')
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
  showOpenDialog: function (opts) {
    checkAppInitialized()

    // destructure params with default options value
    let {window, options = {title: 'Open', properties: ['openFile']}, callback} = opts

    // destructure individual options with defaults
    let {
      buttonLabel = '',
      defaultPath = '',
      filters = [],
      properties = ['openFile'],
      title = '',
      message = '',
      securityScopedBookmarks = false
    } = options

    // check correct options types
    if (!Array.isArray(properties)) {
      throw new TypeError('Properties must be an array')
    }

    for (const p in {buttonLabel, defaultPath, title, message}) {
      if (typeof p !== 'string') {
        throw new TypeError(`${p} must be a string`)
      }
    }

    // populate fileDialogProperties
    let dialogProperties = 0
    for (const prop in fileDialogProperties) {
      if (properties.includes(prop)) {
        dialogProperties |= fileDialogProperties[prop]
      }
    }

    // set up callback if one passed
    let wrappedCallback = null
    if (typeof callback === 'function') {
      wrappedCallback = (success, result, bookmarkData) => {
        return success ? callback(result, bookmarkData) : callback()
      }
    }

    const settings = {title, buttonLabel, defaultPath, filters, message, securityScopedBookmarks, window}
    settings.properties = dialogProperties
    return binding.showOpenDialog(settings, wrappedCallback)
  },

  showSaveDialog: function (opts) {
    checkAppInitialized()

    // destructure params with default options value
    let {window, options = {title: 'Save'}, callback} = opts

    // destructure individual options with defaults
    let {
      buttonLabel = '',
      defaultPath = '',
      filters = [],
      title = '',
      message = '',
      securityScopedBookmarks = false,
      nameFieldLabel = '',
      showsTagField = true
    } = options

    // check correct options types
    for (const p in {title, buttonLabel, defaultPath, message, nameFieldLabel}) {
      if (typeof p !== 'string') {
        throw new TypeError(`${p} must be a string`)
      }
    }

    // set up callback if one passed
    let wrappedCallback = null
    if (typeof callback === 'function') {
      wrappedCallback = (success, result, bookmarkData) => {
        return success ? callback(result, bookmarkData) : callback()
      }
    }

    const settings = {title, buttonLabel, defaultPath, filters, message, securityScopedBookmarks, nameFieldLabel, showsTagField, window}
    return binding.showSaveDialog(settings, wrappedCallback)
  },

  showMessageBox: function (opts) {
    checkAppInitialized()

    // destructure params with default options value
    let {window, options = {type: 'none'}, callback} = opts

    let {
      buttons = [],
      cancelId,
      checkboxLabel = '',
      checkboxChecked,
      defaultId = -1,
      detail = '',
      icon,
      message = '',
      title = '',
      type = 'none'
    } = options

    // check correct options types
    const messageBoxType = messageBoxTypes.indexOf(type)
    if (messageBoxType === -1) {
      throw new TypeError('Invalid message box type')
    }

    if (!Array.isArray(buttons)) {
      throw new TypeError('Buttons must be an array')
    }

    for (const prop in {title, message, detail, checkboxLabel}) {
      if (typeof prop !== 'string') {
        throw new TypeError(`${prop} must be a string`)
      }
    }

    if (options.normalizeAccessKeys) {
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

    const flags = options.noLink ? messageBoxOptions.noLink : 0
    return binding.showMessageBox(messageBoxType, buttons, defaultId, cancelId,
                                  flags, title, message, detail, checkboxLabel,
                                  checkboxChecked, icon, window, callback)
  },

  showErrorBox: function (...args) {
    return binding.showErrorBox(...args)
  },

  showCertificateTrustDialog: function (opts) {
    // destructure params with default options value
    let {window, options = {}, callback} = opts

    if (typeof options !== 'object') {
      throw new TypeError('options must be an object')
    }

    let {certificate, message = ''} = options

    // check correct options types
    if (typeof certificate !== 'object') {
      throw new TypeError('certificate must be an object')
    }

    if (typeof message !== 'string') {
      throw new TypeError('message must be a string')
    }

    return binding.showCertificateTrustDialog(window, certificate, message, callback)
  }
}

// Mark standard asynchronous functions.
v8Util.setHiddenValue(module.exports.showMessageBox, 'asynchronous', true)
v8Util.setHiddenValue(module.exports.showOpenDialog, 'asynchronous', true)
v8Util.setHiddenValue(module.exports.showSaveDialog, 'asynchronous', true)

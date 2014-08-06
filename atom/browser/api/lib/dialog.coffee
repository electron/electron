binding = process.atomBinding 'dialog'
v8Util = process.atomBinding 'v8_util'
BrowserWindow = require 'browser-window'

fileDialogProperties =
  openFile:        1 << 0
  openDirectory:   1 << 1
  multiSelections: 1 << 2
  createDirectory: 1 << 3

messageBoxTypes = ['none', 'info', 'warning']

module.exports =
  showOpenDialog: (window, options, callback) ->
    unless window?.constructor is BrowserWindow
      # Shift.
      callback = options
      options = window
      window = null

    options ?= title: 'Open', properties: ['openFile']
    options.properties ?= ['openFile']
    throw new TypeError('Properties need to be array') unless Array.isArray options.properties

    properties = 0
    for prop, value of fileDialogProperties
      properties |= value if prop in options.properties

    options.title ?= ''
    options.defaultPath ?= ''
    options.filters ?= []

    wrappedCallback =
      if typeof callback is 'function'
        (success, result) -> callback(if success then result)
      else
        null

    binding.showOpenDialog String(options.title),
                           String(options.defaultPath),
                           options.filters
                           properties,
                           window,
                           wrappedCallback

  showSaveDialog: (window, options, callback) ->
    unless window?.constructor is BrowserWindow
      # Shift.
      callback = options
      options = window
      window = null

    options ?= title: 'Save'
    options.title ?= ''
    options.defaultPath ?= ''
    options.filters ?= []

    wrappedCallback =
      if typeof callback is 'function'
        (success, result) -> callback(if success then result)
      else
        null

    binding.showSaveDialog String(options.title),
                           String(options.defaultPath),
                           options.filters
                           window,
                           wrappedCallback

  showMessageBox: (window, options, callback) ->
    unless window?.constructor is BrowserWindow
      # Shift.
      callback = options
      options = window
      window = null

    options ?= type: 'none'
    options.type ?= 'none'
    options.type = messageBoxTypes.indexOf options.type
    throw new TypeError('Invalid message box type') unless options.type > -1

    throw new TypeError('Buttons need to be array') unless Array.isArray options.buttons

    options.title ?= ''
    options.message ?= ''
    options.detail ?= ''

    binding.showMessageBox options.type,
                           options.buttons,
                           String(options.title),
                           String(options.message),
                           String(options.detail),
                           window,
                           callback

# Mark standard asynchronous functions.
v8Util.setHiddenValue f, 'asynchronous', true for k, f of module.exports

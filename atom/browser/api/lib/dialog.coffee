binding = process.atomBinding 'dialog'
v8Util = process.atomBinding 'v8_util'
app = require 'app'
BrowserWindow = require 'browser-window'

fileDialogProperties =
  openFile:        1 << 0
  openDirectory:   1 << 1
  multiSelections: 1 << 2
  createDirectory: 1 << 3

messageBoxTypes = ['none', 'info', 'warning', 'error', 'question']

messageBoxOptions =
  noLink: 1 << 0

parseArgs = (window, options, callback) ->
  unless window is null or window?.constructor is BrowserWindow
    # Shift.
    callback = options
    options = window
    window = null
  if not callback? and typeof options is 'function'
    # Shift.
    callback = options
    options = null
  [window, options, callback]

checkAppInitialized = ->
  throw new Error('dialog module can only be used after app is ready') unless app.isReady()

module.exports =
  showOpenDialog: (args...) ->
    checkAppInitialized()
    [window, options, callback] = parseArgs args...

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

  showSaveDialog: (args...) ->
    checkAppInitialized()
    [window, options, callback] = parseArgs args...

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

  showMessageBox: (args...) ->
    checkAppInitialized()
    [window, options, callback] = parseArgs args...

    options ?= type: 'none'
    options.type ?= 'none'
    messageBoxType = messageBoxTypes.indexOf options.type
    throw new TypeError('Invalid message box type') unless messageBoxType > -1

    throw new TypeError('Buttons need to be array') unless Array.isArray options.buttons

    options.title ?= ''
    options.message ?= ''
    options.detail ?= ''
    options.icon ?= null

    # Choose a default button to get selected when dialog is cancelled.
    unless options.cancelId?
      options.cancelId = 0
      for text, i in options.buttons
        if text.toLowerCase() in ['cancel', 'no']
          options.cancelId = i
          break

    flags = if options.noLink then messageBoxOptions.noLink else 0

    binding.showMessageBox messageBoxType,
                           options.buttons,
                           options.cancelId,
                           flags,
                           options.title,
                           options.message,
                           options.detail,
                           options.icon,
                           window,
                           callback

  showErrorBox: (args...) ->
    binding.showErrorBox args...

# Mark standard asynchronous functions.
for api in ['showMessageBox', 'showOpenDialog', 'showSaveDialog']
  v8Util.setHiddenValue module.exports[api], 'asynchronous', true

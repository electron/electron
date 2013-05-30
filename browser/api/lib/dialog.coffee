binding = process.atomBinding 'dialog'
BrowserWindow = require 'browser-window'

fileDialogProperties =
  openFile: 1, openDirectory: 2, multiSelections: 4, createDirectory: 8

messageBoxTypes = ['none', 'info', 'warning']

module.exports =
  showOpenDialog: (options) ->
    options = title: 'Open', properties: ['openFile'] unless options?
    options.properties = options.properties ? ['openFile']
    throw new TypeError('Properties need to be array') unless Array.isArray options.properties

    properties = 0
    for prop, value of fileDialogProperties
      properties |= value if prop in options.properties

    options.title = options.title ? ''
    options.defaultPath = options.defaultPath ? ''

    binding.showOpenDialog options.title, options.defaultPath, properties

  showSaveDialog: (window, options) ->
    throw new TypeError('Invalid window') unless window?.constructor is BrowserWindow
    options = title: 'Save' unless options?

    options.title = options.title ? ''
    options.defaultPath = options.defaultPath ? ''

    binding.showSaveDialog window, options.title, options.defaultPath

  showMessageBox: (options) ->
    options = type: 'none' unless options?
    options.type = options.type ? 'none'
    options.type = messageBoxTypes.indexOf options.type
    throw new TypeError('Invalid message box type') unless options.type > -1

    throw new TypeError('Buttons need to be array') unless Array.isArray options.buttons

    options.title = options.title ? ''
    options.message = options.message ? ''
    options.detail = options.detail ? ''

    binding.showMessageBox options.type, options.buttons,
                           String(options.title),
                           String(options.message),
                           String(options.detail)

binding = process.atomBinding 'dialog'
BrowserWindow = require 'browser_window'
CallbacksRegistry = require 'callbacks_registry'
EventEmitter = require('events').EventEmitter
ipc = require 'ipc'

FileDialog = binding.FileDialog
FileDialog.prototype.__proto__ = EventEmitter.prototype

callbacksRegistry = new CallbacksRegistry

fileDialog = new FileDialog

fileDialog.on 'selected', (event, callbackId, paths...) ->
  callbacksRegistry.call callbackId, 'selected', paths...
  callbacksRegistry.remove callbackId

fileDialog.on 'cancelled', (event, callbackId) ->
  callbacksRegistry.call callbackId, 'cancelled'
  callbacksRegistry.remove callbackId

validateOptions = (options) ->
  return false unless typeof options is 'object'

  options.fileTypes = [] unless Array.isArray options.fileTypes
  for type in options.fileTypes
    return false unless typeof type is 'object' and
                        typeof type.description is 'string'
                        Array.isArray type.extensions

  options.defaultPath = '' unless options.defaultPath?
  options.fileTypeIndex = 0 unless options.fileTypeIndex?
  options.defaultExtension = '' unless options.defaultExtension?
  true

selectFileWrap = (window, options, callback, type, title) ->
  throw new TypeError('Need BrowserWindow object') unless window.constructor is BrowserWindow

  options = {} unless options?
  options.type = type
  options.title = title unless options.title?

  throw new TypeError('Bad arguments') unless validateOptions options

  callbackId = callbacksRegistry.add callback

  fileDialog.selectFile window,
                        options.type,
                        options.title,
                        options.defaultPath,
                        options.fileTypes,
                        options.fileTypeIndex,
                        options.defaultExtension,
                        callbackId

messageBoxTypes = ['none', 'info', 'warning']

module.exports =
  openFolder: (args...) ->
    selectFileWrap args..., 1, 'Open Folder'

  saveAs: (args...) ->
    selectFileWrap args..., 2, 'Save As'

  openFile: (args...) ->
    selectFileWrap args..., 3, 'Open File'

  openMultiFiles: (args...) ->
    selectFileWrap args..., 4, 'Open Files'

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

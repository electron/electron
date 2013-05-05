binding = process.atomBinding 'dialog'
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
  throw new TypeError('Need Window object') unless window.constructor?.name is 'Window'

  options = {} unless options?
  options.type = type
  options.title = title unless options.title?

  throw new TypeError('Bad arguments') unless validateOptions options

  callbackId = callbacksRegistry.add callback

  fileDialog.selectFile window.getProcessId(), window.getRoutingId(),
                        options.type,
                        options.title,
                        options.defaultPath,
                        options.fileTypes,
                        options.fileTypeIndex,
                        options.defaultExtension,
                        callbackId

module.exports =
  openFolder: (args...) ->
    selectFileWrap args..., 1, 'Open Folder'

  saveAs: (args...) ->
    selectFileWrap args..., 2, 'Save As'

  openFile: (args...) ->
    selectFileWrap args..., 3, 'Open File'

  openMultiFiles: (args...) ->
    selectFileWrap args..., 4, 'Open Files'

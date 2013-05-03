ipc = require 'ipc'

callbackId = 0
callbacks = {}

storeCallback = (callback) ->
  throw new TypeError('Bad argument') unless typeof callback is 'function'

  ++callbackId
  callbacks[callbackId] = callback
  callbackId

makeCallback = (id, args...) ->
  callbacks[id].call global, args...
  delete callbacks[id]

# Force loading dialog code in browser.
remote.require 'dialog'

ipc.on 'ATOM_RENDERER_DIALOG', (id, args...) ->
  makeCallback(id, args...)

callFileDialogs = (options, callback, args...) ->
  if typeof options is 'function'
    callback = options
    options = {}

  ipc.sendChannel 'ATOM_BROWSER_FILE_DIALOG', storeCallback(callback), args..., options

module.exports =
  openFolder: (options, callback) ->
    callFileDialogs options, callback, 1, 'Open Folder'

  saveAs: (options, callback) ->
    callFileDialogs options, callback, 2, 'Save As'

  openFile: (options, callback) ->
    callFileDialogs options, callback, 3, 'Open File'

  openMultiFiles: (options, callback) ->
    callFileDialogs options, callback, 4, 'Open Files'

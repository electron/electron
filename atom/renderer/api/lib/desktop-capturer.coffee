ipc = require 'ipc'
remote = require 'remote'
NativeImage = require 'native-image'

EventEmitter = require('events').EventEmitter
desktopCapturer = new EventEmitter

# Tells main process the renderer is requiring 'desktop-capture' module.
ipc.sendSync 'ATOM_BROWSER_DESKTOP_CAPTURER_REQUIRED'

desktopCapturer.startUpdating = (args) ->
  ipc.send 'ATOM_BROWSER_DESKTOP_CAPTURER_START_UPDATING', args

desktopCapturer.stopUpdating = () ->
  ipc.send 'ATOM_BROWSER_DESKTOP_CAPTURER_STOP_UPDATING'

for event_name in ['source-added', 'source-removed', 'source-name-changed', "source-thumbnail-changed"]
  do (event_name) ->
    ipc.on event_name, (source) ->
      desktopCapturer.emit event_name, { id: source.id, name: source.name, thumbnail: NativeImage.createFromDataUrl source.dataUrl }

module.exports = desktopCapturer

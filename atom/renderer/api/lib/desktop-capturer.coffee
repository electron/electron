ipc = require 'ipc'
remote = require 'remote'
NativeImage = require 'native-image'

EventEmitter = require('events').EventEmitter
desktopCapturer = new EventEmitter

desktopCapturer.startUpdating = (args) ->
  ipc.send 'ATOM_BROWSER_DESKTOP_CAPTURER_START_UPDATING', args

desktopCapturer.stopUpdating = () ->
  ipc.send 'ATOM_BROWSER_DESKTOP_CAPTURER_STOP_UPDATING'

ipc.on 'ATOM_RENDERER_DESKTOP_CAPTURER', (event_name, id, name, thumbnail) ->
  if not thumbnail
    return desktopCapturer.emit event_name, id, name
  desktopCapturer.emit event_name, id, name, NativeImage.createFromDataUrl thumbnail

module.exports = desktopCapturer

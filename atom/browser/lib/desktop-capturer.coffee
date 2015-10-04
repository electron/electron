ipc = require 'ipc'
BrowserWindow = require 'browser-window'
EventEmitter = require('events').EventEmitter

# The browser module manages all desktop-capturer moduels in renderer process.
desktopCapturer = process.atomBinding('desktop_capturer').desktopCapturer
desktopCapturer.__proto__ = EventEmitter.prototype

getWebContentsFromId = (id) ->
  windows = BrowserWindow.getAllWindows()
  return window.webContents for window in windows when window.webContents?.getId() == id

# The set for tracking id of webContents.
webContentsIds = new Set

stopDesktopCapture = (id) ->
  webContentsIds.delete id
  if webContentsIds.size is 0
    desktopCapturer.stopUpdating()

ipc.on 'ATOM_BROWSER_DESKTOP_CAPTURER_REQUIRED', (event) ->
  id = event.sender.getId()
  # Remove the tracked webContents when it is destroyed.
  getWebContentsFromId(id).on 'destroyed', ()->
    stopDesktopCapture id
  event.returnValue = 'done'

# Handle `desktopCapturer.startUpdating` API.
ipc.on 'ATOM_BROWSER_DESKTOP_CAPTURER_START_UPDATING', (event, args) ->
  id = event.sender.getId()
  webContentsIds.add id
  desktopCapturer.startUpdating args

# Handle `desktopCapturer.stopUpdating` API.
ipc.on 'ATOM_BROWSER_DESKTOP_CAPTURER_STOP_UPDATING', (event) ->
  stopDesktopCapture event.sender.getId()

for event_name in ['source-added', 'source-removed', 'source-name-changed', "source-thumbnail-changed"]
  do (event_name) ->
    desktopCapturer.on event_name, (event, source) ->
      webContentsIds.forEach (id) ->
        getWebContentsFromId(id).send event_name, { id: source.id, name: source.name, dataUrl: source.thumbnail.toDataUrl() }

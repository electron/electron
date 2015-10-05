ipc = require 'ipc'
BrowserWindow = require 'browser-window'

# The browser module manages all desktop-capturer moduels in renderer process.
desktopCapturer = process.atomBinding('desktop_capturer').desktopCapturer

getWebContentsFromId = (id) ->
  windows = BrowserWindow.getAllWindows()
  return window.webContents for window in windows when window.webContents?.getId() == id

# The set for tracking id of webContents.
webContentsIds = new Set

stopDesktopCapture = (id) ->
  webContentsIds.delete id
  # Stop updating if no renderer process listens the desktop capturer.
  if webContentsIds.size is 0
    desktopCapturer.stopUpdating()

# Handle `desktopCapturer.startUpdating` API.
ipc.on 'ATOM_BROWSER_DESKTOP_CAPTURER_START_UPDATING', (event, args) ->
  id = event.sender.getId()
  if not webContentsIds.has id
    # Stop sending desktop capturer events to the destroyed webContents.
    event.sender.on 'destroyed', ()->
      stopDesktopCapture id
  # Start updating the desktopCapturer if it doesn't.
  if webContentsIds.size is 0
    desktopCapturer.startUpdating args
  webContentsIds.add id

# Handle `desktopCapturer.stopUpdating` API.
ipc.on 'ATOM_BROWSER_DESKTOP_CAPTURER_STOP_UPDATING', (event) ->
  stopDesktopCapture event.sender.getId()

desktopCapturer.emit = (event_name, event, desktopId, name, thumbnail) ->
  webContentsIds.forEach (id) ->
    getWebContentsFromId(id).send 'ATOM_RENDERER_DESKTOP_CAPTURER', event_name, desktopId, name, thumbnail?.toDataUrl()

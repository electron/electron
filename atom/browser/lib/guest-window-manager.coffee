ipc = require 'ipc'
v8Util = process.atomBinding 'v8_util'
BrowserWindow = require 'browser-window'

guestWindows = new WeakMap

# Callback that registered to "closed" event of guest.
guestUserCloseCallback = ->
  embedderId = v8Util.getHiddenValue this, 'embedderId'
  removeGuest embedderId, @id

# Get all guests created in a window.
getGuestsFromEmbedder = (embedderWindow) ->
  unless guestWindows.has embedderWindow
    guests = []
    guestWindows.set embedderWindow, guests
    # Close all guests when window is closed.
    embedderWindow.on 'closed', ->
      for guest in guests
        # Avoid double removing window from guests.
        guest.removeListener 'closed', guestUserCloseCallback
        # Just close without emitting "beforeunload" event.
        guest.destroy()
  guestWindows.get embedderWindow

# Remove a guest window.
removeGuest = (embedderId, guestId) ->
  guests = getGuestsFromEmbedder BrowserWindow.windows.get(embedderId)
  for guest, i in guests
    if guest.id == guestId
      guests.splice i, 1
      return guest

# Create a new guest created by |embedder| with |options|.
createGuest = (embedder, url, options) ->
  embedderWindow = BrowserWindow.fromWebContents embedder
  guests = getGuestsFromEmbedder embedderWindow

  guest = new BrowserWindow(options)
  guest.loadUrl url

  # Remove self from guest list when user closes guest window.
  v8Util.setHiddenValue guest, 'embedderId', embedderWindow.id
  guest.on 'closed', guestUserCloseCallback

  guests.push guest
  [embedderWindow.id, guest.id]

# Routed window.open messages.
ipc.on 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_OPEN', (event, args...) ->
  event.sender.emit 'new-window', event, args...
  if event.sender.isGuest() or event.defaultPrevented
    event.returnValue = null
  else
    [url, frameName, options] = args
    event.returnValue = createGuest event.sender, url, options

ipc.on 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_CLOSE', (event, args...) ->
  guest = removeGuest args...
  guest.destroy()

ipc.on 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_METHOD', (event, guestId, method, args...) ->
  guest = BrowserWindow.windows.get guestId
  guest[method] args...

ipc.on 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', (event, guestId, method, args...) ->
  guest = BrowserWindow.windows.get guestId
  guest.webContents?[method] args...

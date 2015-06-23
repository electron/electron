ipc = require 'ipc'
v8Util = process.atomBinding 'v8_util'
BrowserWindow = require 'browser-window'

frameToGuest = {}

# Create a new guest created by |embedder| with |options|.
createGuest = (embedder, url, frameName, options) ->
  guest = frameToGuest[frameName]
  if frameName and guest?
    guest.loadUrl url
    return guest.id

  guest = new BrowserWindow(options)
  guest.loadUrl url

  # Remember the embedder, will be used by window.opener methods.
  v8Util.setHiddenValue guest.webContents, 'embedder', embedder

  # When |embedder| is destroyed we should also destroy attached guest, and if
  # guest is closed by user then we should prevent |embedder| from double
  # closing guest.
  closedByEmbedder = ->
    embedder.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_CLOSED', guest.id
    guest.removeListener 'closed', closedByUser
    guest.destroy() unless guest.isClosed()
  closedByUser = ->
    embedder.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_CLOSED', guest.id
    embedder.removeListener 'render-view-deleted', closedByEmbedder
  embedder.once 'render-view-deleted', closedByEmbedder
  guest.once 'closed', closedByUser

  if frameName
    frameToGuest[frameName] = guest
    guest.frameName = frameName
    guest.once 'closed', ->
      delete frameToGuest[frameName]

  guest.id

# Routed window.open messages.
ipc.on 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_OPEN', (event, args...) ->
  [url, frameName, options] = args
  event.sender.emit 'new-window', event, url, frameName, 'new-window'
  if event.sender.isGuest() or event.defaultPrevented
    event.returnValue = null
  else
    event.returnValue = createGuest event.sender, args...

ipc.on 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_CLOSE', (event, guestId) ->
  return unless BrowserWindow.windows.has guestId
  BrowserWindow.windows.get(guestId).destroy()

ipc.on 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_METHOD', (event, guestId, method, args...) ->
  return unless BrowserWindow.windows.has guestId
  BrowserWindow.windows.get(guestId)[method] args...

ipc.on 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_POSTMESSAGE', (event, guestId, message, targetOrigin) ->
  return unless BrowserWindow.windows.has guestId
  guestContents = BrowserWindow.windows.get(guestId).webContents
  if guestContents.getUrl().indexOf(targetOrigin) is 0 or targetOrigin is '*'
    guestContents.send 'ATOM_SHELL_GUEST_WINDOW_POSTMESSAGE', message, targetOrigin

ipc.on 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_OPENER_POSTMESSAGE', (event, message, targetOrigin) ->
  embedder = v8Util.getHiddenValue event.sender, 'embedder'
  if embedder?.getUrl().indexOf(targetOrigin) is 0 or targetOrigin is '*'
    embedder.send 'ATOM_SHELL_GUEST_WINDOW_POSTMESSAGE', message, targetOrigin

ipc.on 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', (event, guestId, method, args...) ->
  return unless BrowserWindow.windows.has guestId
  BrowserWindow.windows.get(guestId).webContents?[method] args...

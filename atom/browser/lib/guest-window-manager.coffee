ipc = require 'ipc'
v8Util = process.atomBinding 'v8_util'
BrowserWindow = require 'browser-window'

frameToGuest = {}

# Merge |options| with the |embedder|'s window's options.
mergeBrowserWindowOptions = (embedder, options) ->
  if embedder.browserWindowOptions?
    # Inherit the original options if it is a BrowserWindow.
    options.__proto__ = embedder.browserWindowOptions
  else
    # Or only inherit web-preferences if it is a webview.
    options['web-preferences'] ?= {}
    options['web-preferences'].__proto__ = embedder.getWebPreferences()
  options

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
    guest.removeListener 'closed', closedByUser
    guest.destroy()
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
  options = mergeBrowserWindowOptions event.sender, options
  event.sender.emit 'new-window', event, url, frameName, 'new-window', options
  if (event.sender.isGuest() and not event.sender.allowPopups) or event.defaultPrevented
    event.returnValue = null
  else
    event.returnValue = createGuest event.sender, url, frameName, options

ipc.on 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_CLOSE', (event, guestId) ->
  BrowserWindow.fromId(guestId)?.destroy()

ipc.on 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_METHOD', (event, guestId, method, args...) ->
  BrowserWindow.fromId(guestId)?[method] args...

ipc.on 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_POSTMESSAGE', (event, guestId, message, targetOrigin) ->
  guestContents = BrowserWindow.fromId(guestId)?.webContents
  if guestContents?.getUrl().indexOf(targetOrigin) is 0 or targetOrigin is '*'
    guestContents.send 'ATOM_SHELL_GUEST_WINDOW_POSTMESSAGE', guestId, message, targetOrigin

ipc.on 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_OPENER_POSTMESSAGE', (event, guestId, message, targetOrigin, sourceOrigin) ->
  embedder = v8Util.getHiddenValue event.sender, 'embedder'
  if embedder?.getUrl().indexOf(targetOrigin) is 0 or targetOrigin is '*'
    embedder.send 'ATOM_SHELL_GUEST_WINDOW_POSTMESSAGE', guestId, message, sourceOrigin

ipc.on 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', (event, guestId, method, args...) ->
  BrowserWindow.fromId(guestId)?.webContents?[method] args...

ipc.on 'ATOM_SHELL_GUEST_WINDOW_MANAGER_GET_GUEST_ID', (event) ->
  embedder = v8Util.getHiddenValue event.sender, 'embedder'
  if embedder?
    guest = BrowserWindow.fromWebContents event.sender
    if guest?
      event.returnValue = guest.id
      return
  event.returnValue = null

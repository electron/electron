process = global.process
ipc = require 'ipc'
remote = require 'remote'

# Window object returned by "window.open".
class BrowserWindowProxy
  constructor: (@guestId) ->
    ipc.on 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_CLOSED', (guestId) =>
      if guestId is @guestId
        @closed = true

  close: ->
    ipc.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_CLOSE', @guestId

  focus: ->
    ipc.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_METHOD', @guestId, 'focus'

  blur: ->
    ipc.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_METHOD', @guestId, 'blur'

  postMessage: (message, targetOrigin='*') ->
    ipc.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_POSTMESSAGE', @guestId, message, targetOrigin

  eval: (args...) ->
    ipc.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', @guestId, 'executeJavaScript', args...

unless process.guestInstanceId?
  # Override default window.close.
  window.close = ->
    remote.getCurrentWindow().close()

# Make the browser window or guest view emit "new-window" event.
window.open = (url, frameName='', features='') ->
  options = {}
  ints = [ 'x', 'y', 'width', 'height', 'min-width', 'max-width', 'min-height', 'max-height', 'zoom-factor' ]
  # Make sure to get rid of excessive whitespace in the property name
  for feature in features.split /,\s*/
    [name, value] = feature.split /\s*=/
    options[name] =
      if value is 'yes' or value is '1'
        true
      else if value is 'no' or value is '0'
        false
      else
        value
  options.x ?= options.left if options.left
  options.y ?= options.top if options.top
  options.title ?= name
  options.width ?= 800
  options.height ?= 600

  (options[name] = parseInt(options[name], 10) if options[name]?) for name in ints

  guestId = ipc.sendSync 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_OPEN', url, frameName, options
  if guestId
    new BrowserWindowProxy(guestId)
  else
    console.error 'It is not allowed to open new window from this WebContents'
    null

# Use the dialog API to implement alert().
window.alert = (message, title='') ->
  dialog = remote.require 'dialog'
  buttons = ['OK']
  message = message.toString()
  dialog.showMessageBox remote.getCurrentWindow(), {message, title, buttons}

# And the confirm().
window.confirm = (message, title='') ->
  dialog = remote.require 'dialog'
  buttons = ['OK', 'Cancel']
  not dialog.showMessageBox remote.getCurrentWindow(), {message, title, buttons}

# But we do not support prompt().
window.prompt = ->
  throw new Error('prompt() is and will not be supported.')

# Simple implementation of postMessage.
unless process.guestInstanceId?
  window.opener =
    postMessage: (message, targetOrigin='*') ->
      ipc.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_OPENER_POSTMESSAGE', message, targetOrigin

ipc.on 'ATOM_SHELL_GUEST_WINDOW_POSTMESSAGE', (message, targetOrigin) ->
  window.postMessage message, targetOrigin

# Forward history operations to browser.
sendHistoryOperation = (args...) ->
  ipc.send 'ATOM_SHELL_NAVIGATION_CONTROLLER', args...

getHistoryOperation = (args...) ->
  ipc.sendSync 'ATOM_SHELL_SYNC_NAVIGATION_CONTROLLER', args...

window.history.back = -> sendHistoryOperation 'goBack'
window.history.forward = -> sendHistoryOperation 'goForward'
window.history.go = (offset) -> sendHistoryOperation 'goToOffset', offset
Object.defineProperty window.history, 'length',
  get: ->
    getHistoryOperation 'length'

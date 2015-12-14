{ipcRenderer, remote} = require 'electron'

# Helper function to resolve relative url.
a = window.top.document.createElement 'a'
resolveURL = (url) ->
  a.href = url
  a.href

# Window object returned by "window.open".
class BrowserWindowProxy
  @proxies: {}

  @getOrCreate: (guestId) ->
    @proxies[guestId] ?= new BrowserWindowProxy(guestId)

  @remove: (guestId) ->
    delete @proxies[guestId]

  constructor: (@guestId) ->
    @closed = false
    ipcRenderer.once "ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_CLOSED_#{@guestId}", =>
      BrowserWindowProxy.remove(@guestId)
      @closed = true

  close: ->
    ipcRenderer.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_CLOSE', @guestId

  focus: ->
    ipcRenderer.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_METHOD', @guestId, 'focus'

  blur: ->
    ipcRenderer.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_METHOD', @guestId, 'blur'

  postMessage: (message, targetOrigin='*') ->
    ipcRenderer.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_POSTMESSAGE', @guestId, message, targetOrigin, location.origin

  eval: (args...) ->
    ipcRenderer.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', @guestId, 'executeJavaScript', args...

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
  options.title ?= frameName
  options.width ?= 800
  options.height ?= 600

  # Resolve relative urls.
  url = resolveURL url

  (options[name] = parseInt(options[name], 10) if options[name]?) for name in ints

  guestId = ipcRenderer.sendSync 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_OPEN', url, frameName, options
  if guestId
    BrowserWindowProxy.getOrCreate(guestId)
  else
    null

# Use the dialog API to implement alert().
window.alert = (message, title='') ->
  buttons = ['OK']
  message = message.toString()
  remote.dialog.showMessageBox remote.getCurrentWindow(), {message, title, buttons}
  # Alert should always return undefined.
  return

# And the confirm().
window.confirm = (message, title='') ->
  buttons = ['OK', 'Cancel']
  cancelId = 1
  not remote.dialog.showMessageBox remote.getCurrentWindow(), {message, title, buttons, cancelId}

# But we do not support prompt().
window.prompt = ->
  throw new Error('prompt() is and will not be supported.')

guestId = ipcRenderer.sendSync 'ATOM_SHELL_GUEST_WINDOW_MANAGER_GET_GUEST_ID'
if guestId?
  window.opener = BrowserWindowProxy.getOrCreate(guestId)
  # Remove BrowserWindowProxy API and give it a custom postMessage method
  Object.setPrototypeOf(window.opener, null)
  window.opener.postMessage = (message, targetOrigin='*') ->
    ipcRenderer.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_OPENER_POSTMESSAGE', guestId, message, targetOrigin, location.origin

ipcRenderer.on 'ATOM_SHELL_GUEST_WINDOW_POSTMESSAGE', (event, guestId, message, sourceOrigin) ->
  # Manually dispatch event instead of using postMessage because we also need to
  # set event.source.
  event = document.createEvent 'Event'
  event.initEvent 'message', false, false
  event.data = message
  event.origin = sourceOrigin
  event.source = BrowserWindowProxy.getOrCreate(guestId)
  window.dispatchEvent event

# Forward history operations to browser.
sendHistoryOperation = (args...) ->
  ipcRenderer.send 'ATOM_SHELL_NAVIGATION_CONTROLLER', args...

getHistoryOperation = (args...) ->
  ipcRenderer.sendSync 'ATOM_SHELL_SYNC_NAVIGATION_CONTROLLER', args...

window.history.back = -> sendHistoryOperation 'goBack'
window.history.forward = -> sendHistoryOperation 'goForward'
window.history.go = (offset) -> sendHistoryOperation 'goToOffset', offset
Object.defineProperty window.history, 'length',
  get: ->
    getHistoryOperation 'length'

# Make document.hidden return the correct value.
Object.defineProperty document, 'hidden',
  get: -> !remote.getCurrentWindow().isVisible()

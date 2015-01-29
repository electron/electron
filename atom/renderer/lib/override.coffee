process = global.process
ipc = require 'ipc'
remote = require 'remote'

# Window object returned by "window.open".
class FakeWindow
  constructor: (@guestId) ->

  close: ->
    ipc.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_CLOSE', @guestId

  focus: ->
    ipc.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_METHOD', @guestId, 'focus'

  blur: ->
    ipc.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_METHOD', @guestId, 'blur'

  eval: (args...) ->
    ipc.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', @guestId, 'executeJavaScript', args...

unless process.guestInstanceId?
  # Override default window.close.
  window.close = ->
    remote.getCurrentWindow().close()

# Make the browser window or guest view emit "new-window" event.
window.open = (url, frameName='', features='') ->
  options = {}
  for feature in features.split ','
    [name, value] = feature.split '='
    options[name] =
      if value is 'yes'
        true
      else if value is 'no'
        false
      else
        value
  options.x ?= options.left
  options.y ?= options.top
  options.title ?= name
  options.width ?= 800
  options.height ?= 600

  guestId = ipc.sendSync 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_OPEN', url, frameName, options
  if guestId
    new FakeWindow(guestId)
  else
    console.error 'It is not allowed to open new window from this WebContents'
    null

# Use the dialog API to implement alert().
window.alert = (message, title='') ->
  dialog = remote.require 'dialog'
  buttons = ['OK']
  dialog.showMessageBox remote.getCurrentWindow(), {message, title, buttons}

# And the confirm().
window.confirm = (message, title='') ->
  dialog = remote.require 'dialog'
  buttons = ['OK', 'Cancel']
  not dialog.showMessageBox remote.getCurrentWindow(), {message, title, buttons}

# But we do not support prompt().
window.prompt = ->
  throw new Error('prompt() is and will not be supported in atom-shell.')

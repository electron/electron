ipc = require 'ipc'
webContents = require 'web-contents'

nextInstanceId = 0
guestInstances = {}

# Generate guestInstanceId.
getNextInstanceId = (webContents) ->
  ++nextInstanceId

# Create a new guest instance.
createGuest = (embedder, params) ->
  id = getNextInstanceId embedder
  guestInstances[id] = webContents.create isGuest: true, guestInstanceId: id
  id

# Destroy an existing guest instance.
destroyGuest = (id) ->
  guestInstances[id].destroy()
  delete guestInstances[id]

ipc.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_CREATE_GUEST', (event, type, params, requestId) ->
  event.sender.send "ATOM_SHELL_RESPONSE_#{requestId}", createGuest(event.sender)

ipc.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_DESTROY_GUEST', (event, guestInstanceId) ->
  destroyGuest guestInstanceId

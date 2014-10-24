ipc = require 'ipc'
webContents = require 'web-contents'
webViewManager = null  # Doesn't exist in early initialization.

nextInstanceId = 0
guestInstances = {}

# Generate guestInstanceId.
getNextInstanceId = (webContents) ->
  ++nextInstanceId

# Create a new guest instance.
createGuest = (embedder, params) ->
  webViewManager ?= process.atomBinding 'web_view_manager'

  id = getNextInstanceId embedder
  guestInstances[id] = webContents.create
    isGuest: true
    guestInstanceId: id
    storagePartitionId: params.storagePartitionId
  webViewManager.addGuest id, embedder, guestInstances[id]
  id

# Destroy an existing guest instance.
destroyGuest = (id) ->
  webViewManager.removeGuest id
  guestInstances[id].destroy()
  delete guestInstances[id]

ipc.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_CREATE_GUEST', (event, type, params, requestId) ->
  event.sender.send "ATOM_SHELL_RESPONSE_#{requestId}", createGuest(event.sender, params)

ipc.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_DESTROY_GUEST', (event, guestInstanceId) ->
  destroyGuest guestInstanceId

ipc.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_SET_AUTO_SIZE', (event, guestInstanceId, params) ->
  guestInstances[id]?.setAutoSize params.enableAutoSize, params.min, params.max

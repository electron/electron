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
  guest = webContents.create
    isGuest: true
    guestInstanceId: id
    storagePartitionId: params.storagePartitionId
  guestInstances[id] = guest
  webViewManager.addGuest id, embedder, guest

  # Destroy guest when the embedder is gone.
  embedder.once 'render-view-deleted', ->
    destroyGuest id

  # Init guest web view after attached.
  guest.once 'internal-did-attach', (event, params) ->
    min = width: params.minwidth, height: params.minheight
    max = width: params.maxwidth, height: params.maxheight
    @setAutoSize params.autosize, min, max
    if params.src
      @loadUrl params.src
    if params.allowtransparency?
      @setAllowTransparency params.allowtransparency

  id

# Destroy an existing guest instance.
destroyGuest = (id) ->
  webViewManager.removeGuest id
  guestInstances[id].destroy()
  delete guestInstances[id]

ipc.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_CREATE_GUEST', (event, type, params, requestId) ->
  event.sender.send "ATOM_SHELL_RESPONSE_#{requestId}", createGuest(event.sender, params)

ipc.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_DESTROY_GUEST', (event, id) ->
  destroyGuest id

ipc.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_SET_AUTO_SIZE', (event, id, params) ->
  guestInstances[id]?.setAutoSize params.enableAutoSize, params.min, params.max

ipc.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_SET_ALLOW_TRANSPARENCY', (event, id, allowtransparency) ->
  guestInstances[id]?.setAllowTransparency allowtransparency

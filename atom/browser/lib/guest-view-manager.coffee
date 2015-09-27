ipc = require 'ipc'
webContents = require 'web-contents'
webViewManager = null  # Doesn't exist in early initialization.

supportedWebViewEvents = [
  'load-commit'
  'did-finish-load'
  'did-fail-load'
  'did-frame-finish-load'
  'did-start-loading'
  'did-stop-loading'
  'did-get-response-details'
  'did-get-redirect-request'
  'dom-ready'
  'console-message'
  'new-window'
  'close'
  'crashed'
  'gpu-crashed'
  'plugin-crashed'
  'destroyed'
  'page-title-set'
  'page-favicon-updated'
  'enter-html-full-screen'
  'leave-html-full-screen'
]

nextInstanceId = 0
guestInstances = {}
embedderElementsMap = {}
reverseEmbedderElementsMap = {}

# Moves the last element of array to the first one.
moveLastToFirst = (list) ->
  list.unshift list.pop()

# Generate guestInstanceId.
getNextInstanceId = (webContents) ->
  ++nextInstanceId

# Create a new guest instance.
createGuest = (embedder, params) ->
  webViewManager ?= process.atomBinding 'web_view_manager'

  id = getNextInstanceId embedder
  guest = webContents.create {isGuest: true, partition: params.partition, embedder}
  guestInstances[id] = {guest, embedder}

  # Destroy guest when the embedder is gone or navigated.
  destroyEvents = ['destroyed', 'crashed', 'did-navigate-to-different-page']
  destroy = ->
    destroyGuest embedder, id if guestInstances[id]?
  for event in destroyEvents
    embedder.once event, destroy
    # Users might also listen to the crashed event, so We must ensure the guest
    # is destroyed before users' listener gets called. It is done by moving our
    # listener to the first one in queue.
    listeners = embedder._events[event]
    moveLastToFirst listeners if Array.isArray listeners
  guest.once 'destroyed', ->
    embedder.removeListener event, destroy for event in destroyEvents

  # Init guest web view after attached.
  guest.once 'did-attach', ->
    params = @attachParams
    delete @attachParams

    @viewInstanceId = params.instanceId
    @setSize
      normal:
        width: params.elementWidth, height: params.elementHeight
      enableAutoSize: params.autosize
      min:
        width: params.minwidth, height: params.minheight
      max:
        width: params.maxwidth, height: params.maxheight

    if params.src
      opts = {}
      opts.httpReferrer = params.httpreferrer if params.httpreferrer
      opts.userAgent = params.useragent if params.useragent
      @loadUrl params.src, opts

    if params.allowtransparency?
      @setAllowTransparency params.allowtransparency

    guest.allowPopups = params.allowpopups

  # Dispatch events to embedder.
  for event in supportedWebViewEvents
    do (event) ->
      guest.on event, (_, args...) ->
        embedder.send "ATOM_SHELL_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-#{guest.viewInstanceId}", event, args...

  # Dispatch guest's IPC messages to embedder.
  guest.on 'ipc-message-host', (_, packed) ->
    [channel, args...] = packed
    embedder.send "ATOM_SHELL_GUEST_VIEW_INTERNAL_IPC_MESSAGE-#{guest.viewInstanceId}", channel, args...

  # Autosize.
  guest.on 'size-changed', (_, args...) ->
    embedder.send "ATOM_SHELL_GUEST_VIEW_INTERNAL_SIZE_CHANGED-#{guest.viewInstanceId}", args...

  id

# Attach the guest to an element of embedder.
attachGuest = (embedder, elementInstanceId, guestInstanceId, params) ->
  guest = guestInstances[guestInstanceId].guest

  # Destroy the old guest when attaching.
  key = "#{embedder.getId()}-#{elementInstanceId}"
  oldGuestInstanceId = embedderElementsMap[key]
  if oldGuestInstanceId?
    # Reattachment to the same guest is not currently supported.
    return unless oldGuestInstanceId != guestInstanceId

    return unless guestInstances[oldGuestInstanceId]?
    destroyGuest embedder, oldGuestInstanceId

  webPreferences =
    'guest-instance-id': guestInstanceId
    'node-integration': params.nodeintegration ? false
    'plugins': params.plugins
    'web-security': !params.disablewebsecurity
  webPreferences['preload-url'] = params.preload if params.preload
  webViewManager.addGuest guestInstanceId, elementInstanceId, embedder, guest, webPreferences

  guest.attachParams = params
  embedderElementsMap[key] = guestInstanceId
  reverseEmbedderElementsMap[guestInstanceId] = key

# Destroy an existing guest instance.
destroyGuest = (embedder, id) ->
  webViewManager.removeGuest embedder, id
  guestInstances[id].guest.destroy()
  delete guestInstances[id]

  key = reverseEmbedderElementsMap[id]
  if key?
    delete reverseEmbedderElementsMap[id]
    delete embedderElementsMap[key]

ipc.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_CREATE_GUEST', (event, params, requestId) ->
  event.sender.send "ATOM_SHELL_RESPONSE_#{requestId}", createGuest(event.sender, params)

ipc.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_ATTACH_GUEST', (event, elementInstanceId, guestInstanceId, params) ->
  attachGuest event.sender, elementInstanceId, guestInstanceId, params

ipc.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_DESTROY_GUEST', (event, id) ->
  destroyGuest event.sender, id

ipc.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_SET_SIZE', (event, id, params) ->
  guestInstances[id]?.guest.setSize params

ipc.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_SET_ALLOW_TRANSPARENCY', (event, id, allowtransparency) ->
  guestInstances[id]?.guest.setAllowTransparency allowtransparency

# Returns WebContents from its guest id.
exports.getGuest = (id) ->
  guestInstances[id]?.guest

# Returns the embedder of the guest.
exports.getEmbedder = (id) ->
  guestInstances[id]?.embedder

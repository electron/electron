ipc = require 'ipc'

requestId = 0

WEB_VIEW_EVENTS =
  'did-finish-load': []
  'did-fail-load': ['errorCode', 'errorDescription']
  'did-frame-finish-load': ['isMainFrame']
  'did-start-loading': []
  'did-stop-loading': []
  'did-get-redirect-request': ['oldUrl', 'newUrl', 'isMainFrame']
  'console-message': ['level', 'message', 'line', 'sourceId']
  'new-window': ['url', 'frameName']
  'close': []
  'crashed': []
  'destroyed': []

dispatchEvent = (webView, event, args...) ->
  throw new Error("Unkown event #{event}") unless WEB_VIEW_EVENTS[event]?
  domEvent = new Event(event)
  for f, i in WEB_VIEW_EVENTS[event]
    domEvent[f] = args[i]
  webView.dispatchEvent domEvent

module.exports =
  registerEvents: (webView, viewInstanceId) ->
    ipc.on "ATOM_SHELL_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-#{viewInstanceId}", (event, args...) ->
      dispatchEvent webView, event, args...

    ipc.on 'ATOM_SHELL_GUEST_VIEW_INTERNAL_SIZE_CHANGED', (args...) ->
      domEvent = new Event('size-changed')
      for f, i in ['oldWidth', 'oldHeight', 'newWidth', 'newHeight']
        domEvent[f] = args[i]
      webView.onSizeChanged domEvent

  createGuest: (type, params, callback) ->
    requestId++
    ipc.send 'ATOM_SHELL_GUEST_VIEW_MANAGER_CREATE_GUEST', type, params, requestId
    ipc.on "ATOM_SHELL_RESPONSE_#{requestId}", callback

  destroyGuest: (guestInstanceId) ->
    ipc.send 'ATOM_SHELL_GUEST_VIEW_MANAGER_DESTROY_GUEST', guestInstanceId

  setAutoSize: (guestInstanceId, params) ->
    ipc.send 'ATOM_SHELL_GUEST_VIEW_MANAGER_SET_AUTO_SIZE', guestInstanceId, params

  setAllowTransparency: (guestInstanceId, allowtransparency) ->
    ipc.send 'ATOM_SHELL_GUEST_VIEW_MANAGER_SET_ALLOW_TRANSPARENCY', guestInstanceId, allowtransparency

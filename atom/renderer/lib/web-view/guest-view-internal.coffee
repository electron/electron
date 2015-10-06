ipc = require 'ipc'
webFrame = require 'web-frame'

requestId = 0

WEB_VIEW_EVENTS =
  'load-commit': ['url', 'isMainFrame']
  'did-finish-load': []
  'did-fail-load': ['errorCode', 'errorDescription', 'validatedUrl']
  'did-frame-finish-load': ['isMainFrame']
  'did-start-loading': []
  'did-stop-loading': []
  'did-get-response-details': ['status', 'newUrl', 'originalUrl',
                               'httpResponseCode', 'requestMethod', 'referrer',
                               'headers']
  'did-get-redirect-request': ['oldUrl', 'newUrl', 'isMainFrame']
  'dom-ready': []
  'console-message': ['level', 'message', 'line', 'sourceId']
  'new-window': ['url', 'frameName', 'disposition', 'options']
  'close': []
  'crashed': []
  'gpu-crashed': []
  'plugin-crashed': ['name', 'version']
  'destroyed': []
  'page-title-set': ['title', 'explicitSet']
  'page-favicon-updated': ['favicons']
  'enter-html-full-screen': []
  'leave-html-full-screen': []

dispatchEvent = (webView, event, args...) ->
  throw new Error("Unknown event #{event}") unless WEB_VIEW_EVENTS[event]?
  domEvent = new Event(event)
  for f, i in WEB_VIEW_EVENTS[event]
    domEvent[f] = args[i]
  webView.dispatchEvent domEvent
  webView.onLoadCommit domEvent if event == 'load-commit'

module.exports =
  registerEvents: (webView, viewInstanceId) ->
    ipc.on "ATOM_SHELL_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-#{viewInstanceId}", (event, args...) ->
      dispatchEvent webView, event, args...

    ipc.on "ATOM_SHELL_GUEST_VIEW_INTERNAL_IPC_MESSAGE-#{viewInstanceId}", (channel, args...) ->
      domEvent = new Event('ipc-message')
      domEvent.channel = channel
      domEvent.args = [args...]
      webView.dispatchEvent domEvent

    ipc.on "ATOM_SHELL_GUEST_VIEW_INTERNAL_SIZE_CHANGED-#{viewInstanceId}", (args...) ->
      domEvent = new Event('size-changed')
      for f, i in ['oldWidth', 'oldHeight', 'newWidth', 'newHeight']
        domEvent[f] = args[i]
      webView.onSizeChanged domEvent

  deregisterEvents: (viewInstanceId) ->
    ipc.removeAllListeners "ATOM_SHELL_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-#{viewInstanceId}"
    ipc.removeAllListeners "ATOM_SHELL_GUEST_VIEW_INTERNAL_IPC_MESSAGE-#{viewInstanceId}"
    ipc.removeAllListeners "ATOM_SHELL_GUEST_VIEW_INTERNAL_SIZE_CHANGED-#{viewInstanceId}"

  createGuest: (params, callback) ->
    requestId++
    ipc.send 'ATOM_SHELL_GUEST_VIEW_MANAGER_CREATE_GUEST', params, requestId
    ipc.once "ATOM_SHELL_RESPONSE_#{requestId}", callback

  attachGuest: (elementInstanceId, guestInstanceId, params) ->
    ipc.send 'ATOM_SHELL_GUEST_VIEW_MANAGER_ATTACH_GUEST', elementInstanceId, guestInstanceId, params
    webFrame.attachGuest elementInstanceId

  destroyGuest: (guestInstanceId) ->
    ipc.send 'ATOM_SHELL_GUEST_VIEW_MANAGER_DESTROY_GUEST', guestInstanceId

  setSize: (guestInstanceId, params) ->
    ipc.send 'ATOM_SHELL_GUEST_VIEW_MANAGER_SET_SIZE', guestInstanceId, params

  setAllowTransparency: (guestInstanceId, allowtransparency) ->
    ipc.send 'ATOM_SHELL_GUEST_VIEW_MANAGER_SET_ALLOW_TRANSPARENCY', guestInstanceId, allowtransparency

{ipcRenderer, webFrame} = require 'electron'

requestId = 0

WEB_VIEW_EVENTS =
  'load-commit': ['url', 'isMainFrame']
  'did-finish-load': []
  'did-fail-load': ['errorCode', 'errorDescription', 'validatedURL']
  'did-frame-finish-load': ['isMainFrame']
  'did-start-loading': []
  'did-stop-loading': []
  'did-get-response-details': ['status', 'newURL', 'originalURL',
                               'httpResponseCode', 'requestMethod', 'referrer',
                               'headers']
  'did-get-redirect-request': ['oldURL', 'newURL', 'isMainFrame']
  'dom-ready': []
  'console-message': ['level', 'message', 'line', 'sourceId']
  'new-window': ['url', 'frameName', 'disposition', 'options']
  'will-navigate': ['url']
  'did-navigate-to-different-page': ['url']
  'did-navigate-in-page': ['url']
  'close': []
  'crashed': []
  'gpu-crashed': []
  'plugin-crashed': ['name', 'version']
  'media-started-playing': []
  'media-paused': []
  'did-change-theme-color': ['themeColor']
  'destroyed': []
  'page-title-updated': ['title', 'explicitSet']
  'page-favicon-updated': ['favicons']
  'enter-html-full-screen': []
  'leave-html-full-screen': []
  'found-in-page': ['result']

DEPRECATED_EVENTS =
  'page-title-updated': 'page-title-set'

dispatchEvent = (webView, eventName, eventKey, args...) ->
  if DEPRECATED_EVENTS[eventName]?
    dispatchEvent webView, DEPRECATED_EVENTS[eventName], eventKey, args...
  domEvent = new Event(eventName)
  for f, i in WEB_VIEW_EVENTS[eventKey]
    domEvent[f] = args[i]
  webView.dispatchEvent domEvent
  webView.onLoadCommit domEvent if eventName is 'load-commit'

module.exports =
  registerEvents: (webView, viewInstanceId) ->
    ipcRenderer.on "ATOM_SHELL_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-#{viewInstanceId}", (event, eventName, args...) ->
      dispatchEvent webView, eventName, eventName, args...

    ipcRenderer.on "ATOM_SHELL_GUEST_VIEW_INTERNAL_IPC_MESSAGE-#{viewInstanceId}", (event, channel, args...) ->
      domEvent = new Event('ipc-message')
      domEvent.channel = channel
      domEvent.args = [args...]
      webView.dispatchEvent domEvent

    ipcRenderer.on "ATOM_SHELL_GUEST_VIEW_INTERNAL_SIZE_CHANGED-#{viewInstanceId}", (event, args...) ->
      domEvent = new Event('size-changed')
      for f, i in ['oldWidth', 'oldHeight', 'newWidth', 'newHeight']
        domEvent[f] = args[i]
      webView.onSizeChanged domEvent

  deregisterEvents: (viewInstanceId) ->
    ipcRenderer.removeAllListeners "ATOM_SHELL_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-#{viewInstanceId}"
    ipcRenderer.removeAllListeners "ATOM_SHELL_GUEST_VIEW_INTERNAL_IPC_MESSAGE-#{viewInstanceId}"
    ipcRenderer.removeAllListeners "ATOM_SHELL_GUEST_VIEW_INTERNAL_SIZE_CHANGED-#{viewInstanceId}"

  createGuest: (params, callback) ->
    requestId++
    ipcRenderer.send 'ATOM_SHELL_GUEST_VIEW_MANAGER_CREATE_GUEST', params, requestId
    ipcRenderer.once "ATOM_SHELL_RESPONSE_#{requestId}", callback

  attachGuest: (elementInstanceId, guestInstanceId, params) ->
    ipcRenderer.send 'ATOM_SHELL_GUEST_VIEW_MANAGER_ATTACH_GUEST', elementInstanceId, guestInstanceId, params
    webFrame.attachGuest elementInstanceId

  destroyGuest: (guestInstanceId) ->
    ipcRenderer.send 'ATOM_SHELL_GUEST_VIEW_MANAGER_DESTROY_GUEST', guestInstanceId

  setSize: (guestInstanceId, params) ->
    ipcRenderer.send 'ATOM_SHELL_GUEST_VIEW_MANAGER_SET_SIZE', guestInstanceId, params

  setAllowTransparency: (guestInstanceId, allowtransparency) ->
    ipcRenderer.send 'ATOM_SHELL_GUEST_VIEW_MANAGER_SET_ALLOW_TRANSPARENCY', guestInstanceId, allowtransparency

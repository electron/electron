/* globals Event */

'use strict'

const ipcRenderer = require('electron').ipcRenderer
const webFrame = require('electron').webFrame

var requestId = 0

var WEB_VIEW_EVENTS = {
  'load-start': ['url', 'isMainFrame', 'isErrorPage', 'isFrameSrcDoc'],
  'load-commit': ['url', 'isMainFrame'],
  'did-finish-load': [],
  'did-fail-load': ['errorCode', 'errorDescription', 'validatedURL'],
  'did-frame-finish-load': ['isMainFrame'],
  'did-start-loading': [],
  'did-stop-loading': [],
  'did-get-response-details': ['status', 'newURL', 'originalURL', 'httpResponseCode', 'requestMethod', 'referrer', 'headers'],
  'did-get-redirect-request': ['oldURL', 'newURL', 'isMainFrame'],
  'dom-ready': [],
  'console-message': ['level', 'message', 'line', 'sourceId'],
  'devtools-opened': [],
  'devtools-closed': [],
  'devtools-focused': [],
  'new-window': ['url', 'frameName', 'disposition', 'options'],
  'will-navigate': ['url'],
  'did-navigate': ['url'],
  'did-navigate-in-page': ['url'],
  'close': [],
  'crashed': [],
  'gpu-crashed': [],
  'plugin-crashed': ['name', 'version'],
  'media-started-playing': [],
  'media-paused': [],
  'did-change-theme-color': ['themeColor'],
  'destroyed': [],
  'page-title-updated': ['title', 'explicitSet'],
  'page-favicon-updated': ['favicons'],
  'enter-html-full-screen': [],
  'leave-html-full-screen': [],
  'found-in-page': ['result'],
  'set-active': []
}

var DEPRECATED_EVENTS = {
  'page-title-updated': 'page-title-set'
}

var dispatchEvent = function (webView, eventName, eventKey, ...args) {
  var domEvent, f, i, j, len, ref1
  if (DEPRECATED_EVENTS[eventName] != null) {
    dispatchEvent.apply(null, [webView, DEPRECATED_EVENTS[eventName], eventKey].concat(args))
  }
  domEvent = new Event(eventName)
  ref1 = WEB_VIEW_EVENTS[eventKey]
  for (i = j = 0, len = ref1.length; j < len; i = ++j) {
    f = ref1[i]
    domEvent[f] = args[i]
  }
  webView.dispatchEvent(domEvent)
  if (eventName === 'load-commit') {
    return webView.onLoadCommit(domEvent)
  }
}

module.exports = {
  registerEvents: function (webView, viewInstanceId) {
    ipcRenderer.on('ATOM_SHELL_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-' + viewInstanceId, function (event, eventName, ...args) {
      return dispatchEvent.apply(null, [webView, eventName, eventName].concat(args))
    })

    ipcRenderer.on('ATOM_SHELL_GUEST_VIEW_INTERNAL_IPC_MESSAGE-' + viewInstanceId, function (event, channel, ...args) {
      var domEvent = new Event('ipc-message')
      domEvent.channel = channel
      domEvent.args = args
      return webView.dispatchEvent(domEvent)
    })

    return ipcRenderer.on('ATOM_SHELL_GUEST_VIEW_INTERNAL_SIZE_CHANGED-' + viewInstanceId, function (event, ...args) {
      var domEvent, f, i, j, len, ref1
      domEvent = new Event('size-changed')
      ref1 = ['oldWidth', 'oldHeight', 'newWidth', 'newHeight']
      for (i = j = 0, len = ref1.length; j < len; i = ++j) {
        f = ref1[i]
        domEvent[f] = args[i]
      }
      return webView.onSizeChanged(domEvent)
    })
  },
  deregisterEvents: function (viewInstanceId) {
    ipcRenderer.removeAllListeners('ATOM_SHELL_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-' + viewInstanceId)
    ipcRenderer.removeAllListeners('ATOM_SHELL_GUEST_VIEW_INTERNAL_IPC_MESSAGE-' + viewInstanceId)
    return ipcRenderer.removeAllListeners('ATOM_SHELL_GUEST_VIEW_INTERNAL_SIZE_CHANGED-' + viewInstanceId)
  },
  addGuest: function (params, callback) {
    requestId++
    ipcRenderer.send('ATOM_SHELL_GUEST_VIEW_MANAGER_ADD_GUEST', params, requestId)
    return ipcRenderer.once('ATOM_SHELL_RESPONSE_' + requestId, callback)
  },
  createGuest: function (params, callback) {
    requestId++
    ipcRenderer.send('ATOM_SHELL_GUEST_VIEW_MANAGER_CREATE_GUEST', params, requestId)
    return ipcRenderer.once('ATOM_SHELL_RESPONSE_' + requestId, callback)
  },
  attachGuest: function (elementInstanceId, guestInstanceId, params) {
    ipcRenderer.send('ATOM_SHELL_GUEST_VIEW_MANAGER_ATTACH_GUEST', elementInstanceId, guestInstanceId, params)
    return webFrame.attachGuest(elementInstanceId)
  },
  destroyGuest: function (guestInstanceId) {
    return ipcRenderer.send('ATOM_SHELL_GUEST_VIEW_MANAGER_DESTROY_GUEST', guestInstanceId)
  },
  setSize: function (guestInstanceId, params) {
    return ipcRenderer.send('ATOM_SHELL_GUEST_VIEW_MANAGER_SET_SIZE', guestInstanceId, params)
  }
}

'use strict'

const { webFrame } = require('electron')
const { ipcRendererInternal } = require('@electron/internal/renderer/ipc-renderer-internal')

let requestId = 0

const WEB_VIEW_EVENTS = {
  'load-commit': ['url', 'isMainFrame'],
  'did-attach': [],
  'did-finish-load': [],
  'did-fail-load': ['errorCode', 'errorDescription', 'validatedURL', 'isMainFrame', 'frameProcessId', 'frameRoutingId'],
  'did-frame-finish-load': ['isMainFrame', 'frameProcessId', 'frameRoutingId'],
  'did-start-loading': [],
  'did-stop-loading': [],
  'dom-ready': [],
  'console-message': ['level', 'message', 'line', 'sourceId'],
  'context-menu': ['params'],
  'devtools-opened': [],
  'devtools-closed': [],
  'devtools-focused': [],
  'new-window': ['url', 'frameName', 'disposition', 'options'],
  'will-navigate': ['url'],
  'did-start-navigation': ['url', 'isInPlace', 'isMainFrame', 'frameProcessId', 'frameRoutingId'],
  'did-navigate': ['url', 'httpResponseCode', 'httpStatusText'],
  'did-frame-navigate': ['url', 'httpResponseCode', 'httpStatusText', 'isMainFrame', 'frameProcessId', 'frameRoutingId'],
  'did-navigate-in-page': ['url', 'isMainFrame', 'frameProcessId', 'frameRoutingId'],
  'focus-change': ['focus', 'guestInstanceId'],
  'close': [],
  'crashed': [],
  'gpu-crashed': [],
  'plugin-crashed': ['name', 'version'],
  'destroyed': [],
  'page-title-updated': ['title', 'explicitSet'],
  'page-favicon-updated': ['favicons'],
  'enter-html-full-screen': [],
  'leave-html-full-screen': [],
  'media-started-playing': [],
  'media-paused': [],
  'found-in-page': ['result'],
  'did-change-theme-color': ['themeColor'],
  'update-target-url': ['url']
}

const DEPRECATED_EVENTS = {
  'page-title-updated': 'page-title-set'
}

const dispatchEvent = function (webView, eventName, eventKey, ...args) {
  if (DEPRECATED_EVENTS[eventName] != null) {
    dispatchEvent(webView, DEPRECATED_EVENTS[eventName], eventKey, ...args)
  }
  const domEvent = new Event(eventName)
  WEB_VIEW_EVENTS[eventKey].forEach((prop, index) => {
    domEvent[prop] = args[index]
  })
  webView.dispatchEvent(domEvent)
  if (eventName === 'load-commit') {
    webView.onLoadCommit(domEvent)
  } else if (eventName === 'focus-change') {
    webView.onFocusChange(domEvent)
  }
}

module.exports = {
  registerEvents: function (webView, viewInstanceId) {
    ipcRendererInternal.on(`ELECTRON_GUEST_VIEW_INTERNAL_DESTROY_GUEST-${viewInstanceId}`, function () {
      webView.guestInstanceId = undefined
      webView.reset()
      const domEvent = new Event('destroyed')
      webView.dispatchEvent(domEvent)
    })

    ipcRendererInternal.on(`ELECTRON_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-${viewInstanceId}`, function (event, eventName, ...args) {
      dispatchEvent(webView, eventName, eventName, ...args)
    })

    ipcRendererInternal.on(`ELECTRON_GUEST_VIEW_INTERNAL_IPC_MESSAGE-${viewInstanceId}`, function (event, channel, ...args) {
      const domEvent = new Event('ipc-message')
      domEvent.channel = channel
      domEvent.args = args
      webView.dispatchEvent(domEvent)
    })
  },
  deregisterEvents: function (viewInstanceId) {
    ipcRendererInternal.removeAllListeners(`ELECTRON_GUEST_VIEW_INTERNAL_DESTROY_GUEST-${viewInstanceId}`)
    ipcRendererInternal.removeAllListeners(`ELECTRON_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-${viewInstanceId}`)
    ipcRendererInternal.removeAllListeners(`ELECTRON_GUEST_VIEW_INTERNAL_IPC_MESSAGE-${viewInstanceId}`)
  },
  createGuest: function (params, callback) {
    requestId++
    ipcRendererInternal.send('ELECTRON_GUEST_VIEW_MANAGER_CREATE_GUEST', params, requestId)
    ipcRendererInternal.once(`ELECTRON_RESPONSE_${requestId}`, callback)
  },
  createGuestSync: function (params) {
    return ipcRendererInternal.sendSync('ELECTRON_GUEST_VIEW_MANAGER_CREATE_GUEST_SYNC', params)
  },
  destroyGuest: function (guestInstanceId) {
    ipcRendererInternal.send('ELECTRON_GUEST_VIEW_MANAGER_DESTROY_GUEST', guestInstanceId)
  },
  attachGuest: function (elementInstanceId, guestInstanceId, params, contentWindow) {
    const embedderFrameId = webFrame.getWebFrameId(contentWindow)
    if (embedderFrameId < 0) { // this error should not happen.
      throw new Error('Invalid embedder frame')
    }
    ipcRendererInternal.send('ELECTRON_GUEST_VIEW_MANAGER_ATTACH_GUEST', embedderFrameId, elementInstanceId, guestInstanceId, params)
  }
}

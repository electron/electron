// This file should have no requires since it is used by the isolated context
// preload bundle. Instead arguments should be passed in for everything it
// needs.

'use strict'

const {defineProperty} = Object

// Helper function to resolve relative url.
const a = window.top.document.createElement('a')
const resolveURL = function (url) {
  a.href = url
  return a.href
}

const windowProxies = {}

module.exports = (ipcRenderer, guestInstanceId, openerId, hiddenPage) => {
  const getOrCreateProxy = (guestId) => {
    let proxy = windowProxies[guestId]
    if (proxy == null) {
      proxy = new BrowserWindowProxy(guestId)
      windowProxies[guestId] = proxy
    }
    return proxy
  }

  const removeProxy = (guestId) => {
    delete windowProxies[guestId]
  }

  function BrowserWindowProxy (guestId) {
    this.closed = false

    ipcRenderer.once(`ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_CLOSED_${guestId}`, () => {
      removeProxy(this.guestId)
      this.closed = true
    })

    this.close = () => {
      ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_CLOSE', guestId)
    }

    this.focus = () => {
      ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_METHOD', guestId, 'focus')
    }

    this.blur = () => {
      ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_METHOD', guestId, 'blur')
    }

    this.print = () => {
      ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', guestId, 'print')
    }

    this.postMessage = (message, targetOrigin) => {
      ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_POSTMESSAGE', guestId, message, targetOrigin, window.location.origin)
    }

    this.eval = (...args) => {
      ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', guestId, 'executeJavaScript', ...args)
    }
  }

  if (guestInstanceId == null) {
    // Override default window.close.
    window.close = function () {
      ipcRenderer.sendSync('ELECTRON_BROWSER_WINDOW_CLOSE')
    }
  }

  // Make the browser window or guest view emit "new-window" event.
  window.open = function (url, frameName, features) {
    if (url != null && url.length > 0) {
      url = resolveURL(url)
    }
    const guestId = ipcRenderer.sendSync('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_OPEN', url, frameName, features)
    if (guestId != null) {
      return getOrCreateProxy(guestId)
    } else {
      return null
    }
  }

  window.alert = function (message, title) {
    ipcRenderer.sendSync('ELECTRON_BROWSER_WINDOW_ALERT', message, title)
  }

  window.confirm = function (message, title) {
    return ipcRenderer.sendSync('ELECTRON_BROWSER_WINDOW_CONFIRM', message, title)
  }

  // But we do not support prompt().
  window.prompt = function () {
    throw new Error('prompt() is and will not be supported.')
  }

  if (openerId != null) {
    window.opener = getOrCreateProxy(openerId)
  }

  ipcRenderer.on('ELECTRON_GUEST_WINDOW_POSTMESSAGE', function (event, sourceId, message, sourceOrigin) {
    // Manually dispatch event instead of using postMessage because we also need to
    // set event.source.
    event = document.createEvent('Event')
    event.initEvent('message', false, false)
    event.data = message
    event.origin = sourceOrigin
    event.source = BrowserWindowProxy.getOrCreate(sourceId)
    window.dispatchEvent(event)
  })

  // Forward history operations to browser.
  const sendHistoryOperation = function (...args) {
    ipcRenderer.send('ELECTRON_NAVIGATION_CONTROLLER', ...args)
  }

  const getHistoryOperation = function (...args) {
    return ipcRenderer.sendSync('ELECTRON_SYNC_NAVIGATION_CONTROLLER', ...args)
  }

  window.history.back = function () {
    sendHistoryOperation('goBack')
  }

  window.history.forward = function () {
    sendHistoryOperation('goForward')
  }

  window.history.go = function (offset) {
    sendHistoryOperation('goToOffset', offset)
  }

  defineProperty(window.history, 'length', {
    get: function () {
      return getHistoryOperation('length')
    }
  })

  // The initial visibilityState.
  let cachedVisibilityState = hiddenPage ? 'hidden' : 'visible'

  // Subscribe to visibilityState changes.
  ipcRenderer.on('ELECTRON_RENDERER_WINDOW_VISIBILITY_CHANGE', function (event, visibilityState) {
    if (cachedVisibilityState !== visibilityState) {
      cachedVisibilityState = visibilityState
      document.dispatchEvent(new Event('visibilitychange'))
    }
  })

  // Make document.hidden and document.visibilityState return the correct value.
  defineProperty(document, 'hidden', {
    get: function () {
      return cachedVisibilityState !== 'visible'
    }
  })

  defineProperty(document, 'visibilityState', {
    get: function () {
      return cachedVisibilityState
    }
  })
}

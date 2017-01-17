// This file should have no requires since it is used by the isolated context
// preload bundle. Instead arguments should be passed in for everything it
// needs.

// This file implements the following APIs:
// - window.alert()
// - window.confirm()
// - window.history.back()
// - window.history.forward()
// - window.history.go()
// - window.history.length
// - window.open()
// - window.opener.blur()
// - window.opener.close()
// - window.opener.eval()
// - window.opener.focus()
// - window.opener.location
// - window.opener.print()
// - window.opener.postMessage()
// - window.prompt()
// - document.hidden
// - document.visibilityState

'use strict'

const {defineProperty} = Object

// Helper function to resolve relative url.
const a = window.top.document.createElement('a')
const resolveURL = function (url) {
  a.href = url
  return a.href
}

const windowProxies = {}

const getOrCreateProxy = (ipcRenderer, guestId) => {
  let proxy = windowProxies[guestId]
  if (proxy == null) {
    proxy = new BrowserWindowProxy(ipcRenderer, guestId)
    windowProxies[guestId] = proxy
  }
  return proxy
}

const removeProxy = (guestId) => {
  delete windowProxies[guestId]
}

function BrowserWindowProxy (ipcRenderer, guestId) {
  this.closed = false

  defineProperty(this, 'location', {
    get: function () {
      return ipcRenderer.sendSync('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD_SYNC', guestId, 'getURL')
    },
    set: function (url) {
      url = resolveURL(url)
      return ipcRenderer.sendSync('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD_SYNC', guestId, 'loadURL', url)
    }
  })

  ipcRenderer.once(`ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_CLOSED_${guestId}`, () => {
    removeProxy(guestId)
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

// Forward history operations to browser.
const sendHistoryOperation = function (ipcRenderer, ...args) {
  ipcRenderer.send('ELECTRON_NAVIGATION_CONTROLLER', ...args)
}

const getHistoryOperation = function (ipcRenderer, ...args) {
  return ipcRenderer.sendSync('ELECTRON_SYNC_NAVIGATION_CONTROLLER', ...args)
}

module.exports = (ipcRenderer, guestInstanceId, openerId, hiddenPage) => {
  if (guestInstanceId == null) {
    // Override default window.close.
    window.close = function () {
      ipcRenderer.sendSync('ELECTRON_BROWSER_WINDOW_CLOSE')
    }
  }

  // Make the browser window or guest view emit "new-window" event.
  window.open = function (url, frameName, features) {
    if (url != null && url !== '') {
      url = resolveURL(url)
    }
    const guestId = ipcRenderer.sendSync('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_OPEN', url, frameName, features)
    if (guestId != null) {
      return getOrCreateProxy(ipcRenderer, guestId)
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
    window.opener = getOrCreateProxy(ipcRenderer, openerId)
  }

  ipcRenderer.on('ELECTRON_GUEST_WINDOW_POSTMESSAGE', function (event, sourceId, message, sourceOrigin) {
    // Manually dispatch event instead of using postMessage because we also need to
    // set event.source.
    event = document.createEvent('Event')
    event.initEvent('message', false, false)
    event.data = message
    event.origin = sourceOrigin
    event.source = getOrCreateProxy(ipcRenderer, sourceId)
    window.dispatchEvent(event)
  })

  window.history.back = function () {
    sendHistoryOperation(ipcRenderer, 'goBack')
  }

  window.history.forward = function () {
    sendHistoryOperation(ipcRenderer, 'goForward')
  }

  window.history.go = function (offset) {
    sendHistoryOperation(ipcRenderer, 'goToOffset', offset)
  }

  defineProperty(window.history, 'length', {
    get: function () {
      return getHistoryOperation(ipcRenderer, 'length')
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

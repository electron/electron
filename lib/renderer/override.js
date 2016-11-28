'use strict'

const ipcRenderer = require('electron').ipcRenderer
const remote = require('electron').remote
const parseFeaturesString = require('../common/parse-features-string')

// Helper function to resolve relative url.
var a = window.top.document.createElement('a')
var resolveURL = function (url) {
  a.href = url
  return a.href
}

// Window object returned by "window.open".
var BrowserWindowProxy = (function () {
  BrowserWindowProxy.proxies = {}

  BrowserWindowProxy.getOrCreate = function (guestId) {
    var base = this.proxies
    base[guestId] != null ? base[guestId] : base[guestId] = new BrowserWindowProxy(guestId)
    return base[guestId]
  }

  BrowserWindowProxy.remove = function (guestId) {
    delete this.proxies[guestId]
  }

  function BrowserWindowProxy (guestId1) {
    Object.defineProperty(this, 'guestId', {
      configurable: false,
      enumerable: true,
      writeable: false,
      value: guestId1
    })

    this.closed = false
    ipcRenderer.once('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_CLOSED_' + this.guestId, () => {
      BrowserWindowProxy.remove(this.guestId)
      this.closed = true
    })
  }

  BrowserWindowProxy.prototype.close = function () {
    ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_CLOSE', this.guestId)
  }

  BrowserWindowProxy.prototype.focus = function () {
    ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_METHOD', this.guestId, 'focus')
  }

  BrowserWindowProxy.prototype.blur = function () {
    ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_METHOD', this.guestId, 'blur')
  }

  BrowserWindowProxy.prototype.print = function () {
    ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', this.guestId, 'print')
  }

  Object.defineProperty(BrowserWindowProxy.prototype, 'location', {
    get: function () {
      return ipcRenderer.sendSync('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD_SYNC', this.guestId, 'getURL')
    },
    set: function (url) {
      url = resolveURL(url)
      return ipcRenderer.sendSync('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD_SYNC', this.guestId, 'loadURL', url)
    }
  })

  BrowserWindowProxy.prototype.postMessage = function (message, targetOrigin) {
    if (targetOrigin == null) {
      targetOrigin = '*'
    }
    ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_POSTMESSAGE', this.guestId, message, targetOrigin, window.location.origin)
  }

  BrowserWindowProxy.prototype['eval'] = function (...args) {
    ipcRenderer.send.apply(ipcRenderer, ['ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', this.guestId, 'executeJavaScript'].concat(args))
  }

  return BrowserWindowProxy
})()

if (process.guestInstanceId == null) {
  // Override default window.close.
  window.close = function () {
    return remote.getCurrentWindow().close()
  }
}

// Make the browser window or guest view emit "new-window" event.
window.open = function (url, frameName, features) {
  var guestId, j, len1, name, options, additionalFeatures
  if (frameName == null) {
    frameName = ''
  }
  if (features == null) {
    features = ''
  }
  options = {}

  const ints = ['x', 'y', 'width', 'height', 'minWidth', 'maxWidth', 'minHeight', 'maxHeight', 'zoomFactor']
  const webPreferences = ['zoomFactor', 'nodeIntegration', 'preload']
  const disposition = 'new-window'

  // Used to store additional features
  additionalFeatures = []

  // Parse the features
  parseFeaturesString(features, function (key, value) {
    if (value === undefined) {
      additionalFeatures.push(key)
    } else {
      if (webPreferences.includes(key)) {
        if (options.webPreferences == null) {
          options.webPreferences = {}
        }
        options.webPreferences[key] = value
      } else {
        options[key] = value
      }
    }
  })
  if (options.left) {
    if (options.x == null) {
      options.x = options.left
    }
  }
  if (options.top) {
    if (options.y == null) {
      options.y = options.top
    }
  }
  if (options.title == null) {
    options.title = frameName
  }
  if (options.width == null) {
    options.width = 800
  }
  if (options.height == null) {
    options.height = 600
  }

  // Resolve relative urls.
  if (url == null || url === '') {
    url = 'about:blank'
  } else {
    url = resolveURL(url)
  }
  for (j = 0, len1 = ints.length; j < len1; j++) {
    name = ints[j]
    if (options[name] != null) {
      options[name] = parseInt(options[name], 10)
    }
  }
  guestId = ipcRenderer.sendSync('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_OPEN', url, frameName, disposition, options, additionalFeatures)
  if (guestId) {
    return BrowserWindowProxy.getOrCreate(guestId)
  } else {
    return null
  }
}

// Use the dialog API to implement alert().
window.alert = function (message = '', title = '') {
  remote.dialog.showMessageBox(remote.getCurrentWindow(), {
    message: String(message),
    title: String(title),
    buttons: ['OK']
  })
}

// And the confirm().
window.confirm = function (message, title) {
  var buttons, cancelId
  if (title == null) {
    title = ''
  }
  buttons = ['OK', 'Cancel']
  cancelId = 1
  return !remote.dialog.showMessageBox(remote.getCurrentWindow(), {
    message: message,
    title: title,
    buttons: buttons,
    cancelId: cancelId
  })
}

// But we do not support prompt().
window.prompt = function () {
  throw new Error('prompt() is and will not be supported.')
}

if (process.openerId != null) {
  window.opener = BrowserWindowProxy.getOrCreate(process.openerId)
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
var sendHistoryOperation = function (...args) {
  ipcRenderer.send.apply(ipcRenderer, ['ELECTRON_NAVIGATION_CONTROLLER'].concat(args))
}

var getHistoryOperation = function (...args) {
  return ipcRenderer.sendSync.apply(ipcRenderer, ['ELECTRON_SYNC_NAVIGATION_CONTROLLER'].concat(args))
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

Object.defineProperty(window.history, 'length', {
  get: function () {
    return getHistoryOperation('length')
  }
})

// The initial visibilityState.
let cachedVisibilityState = process.argv.includes('--hidden-page') ? 'hidden' : 'visible'

// Subscribe to visibilityState changes.
ipcRenderer.on('ELECTRON_RENDERER_WINDOW_VISIBILITY_CHANGE', function (event, visibilityState) {
  if (cachedVisibilityState !== visibilityState) {
    cachedVisibilityState = visibilityState
    document.dispatchEvent(new Event('visibilitychange'))
  }
})

// Make document.hidden and document.visibilityState return the correct value.
Object.defineProperty(document, 'hidden', {
  get: function () {
    return cachedVisibilityState !== 'visible'
  }
})

Object.defineProperty(document, 'visibilityState', {
  get: function () {
    return cachedVisibilityState
  }
})

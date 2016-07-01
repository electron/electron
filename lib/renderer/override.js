'use strict'

const ipcRenderer = require('electron').ipcRenderer
const remote = require('electron').remote

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
    return delete this.proxies[guestId]
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
    return ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_CLOSE', this.guestId)
  }

  BrowserWindowProxy.prototype.focus = function () {
    return ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_METHOD', this.guestId, 'focus')
  }

  BrowserWindowProxy.prototype.blur = function () {
    return ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_METHOD', this.guestId, 'blur')
  }

  BrowserWindowProxy.prototype.print = function () {
    return ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', this.guestId, 'print')
  }

  Object.defineProperty(BrowserWindowProxy.prototype, 'location', {
    get: function () {
      return ipcRenderer.sendSync('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_METHOD', this.guestId, 'getURL')
    },
    set: function (url) {
      return ipcRenderer.sendSync('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_METHOD', this.guestId, 'loadURL', url)
    }
  })

  BrowserWindowProxy.prototype.postMessage = function (message, targetOrigin) {
    if (targetOrigin == null) {
      targetOrigin = '*'
    }
    return ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_POSTMESSAGE', this.guestId, message, targetOrigin, window.location.origin)
  }

  BrowserWindowProxy.prototype['eval'] = function (...args) {
    return ipcRenderer.send.apply(ipcRenderer, ['ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', this.guestId, 'executeJavaScript'].concat(args))
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
  var feature, guestId, i, j, len, len1, name, options, ref1, ref2, value
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

  // Make sure to get rid of excessive whitespace in the property name
  ref1 = features.split(/,\s*/)
  for (i = 0, len = ref1.length; i < len; i++) {
    feature = ref1[i]
    ref2 = feature.split(/\s*=/)
    name = ref2[0]
    value = ref2[1]
    value = value === 'yes' || value === '1' ? true : value === 'no' || value === '0' ? false : value
    if (webPreferences.includes(name)) {
      if (options.webPreferences == null) {
        options.webPreferences = {}
      }
      options.webPreferences[name] = value
    } else {
      options[name] = value
    }
  }
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
  url = resolveURL(url)
  for (j = 0, len1 = ints.length; j < len1; j++) {
    name = ints[j]
    if (options[name] != null) {
      options[name] = parseInt(options[name], 10)
    }
  }
  guestId = ipcRenderer.sendSync('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_OPEN', url, frameName, disposition, options)
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
  return ipcRenderer.send.apply(ipcRenderer, ['ELECTRON_NAVIGATION_CONTROLLER'].concat(args))
}

var getHistoryOperation = function (...args) {
  return ipcRenderer.sendSync.apply(ipcRenderer, ['ELECTRON_SYNC_NAVIGATION_CONTROLLER'].concat(args))
}

window.history.back = function () {
  return sendHistoryOperation('goBack')
}

window.history.forward = function () {
  return sendHistoryOperation('goForward')
}

window.history.go = function (offset) {
  return sendHistoryOperation('goToOffset', offset)
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

// Make notifications silent if the the current webContents is muted
const NativeNotification = window.Notification
class Notification extends NativeNotification {
  constructor (title, opts) {
    super(title, Object.assign({
      silent: remote.getCurrentWebContents().isAudioMuted()
    }, opts))
  }
}
window.Notification = Notification

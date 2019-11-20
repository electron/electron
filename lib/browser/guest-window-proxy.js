const { BrowserWindow, webContents } = require('electron')
const { isSameOrigin } = process.electronBinding('v8_util')
const { ipcMainInternal } = require('@electron/internal/browser/ipc-main-internal')
const ipcMainUtils = require('@electron/internal/browser/ipc-main-internal-utils')
const { convertFeaturesString } = require('@electron/internal/common/parse-features-string')

const getGuestWindow = function (guestContents) {
  let guestWindow = BrowserWindow.fromWebContents(guestContents)
  if (guestWindow == null) {
    const hostContents = guestContents.hostWebContents
    if (hostContents != null) {
      guestWindow = BrowserWindow.fromWebContents(hostContents)
    }
  }
  if (!guestWindow) {
    throw new Error('getGuestWindow failed')
  }
  return guestWindow
}

const isChildWindow = function (sender, target) {
  return target.getLastWebPreferences().openerId === sender.id
}

const isRelatedWindow = function (sender, target) {
  return isChildWindow(sender, target) || isChildWindow(target, sender)
}

const isScriptableWindow = function (sender, target) {
  return isRelatedWindow(sender, target) && isSameOrigin(sender.getURL(), target.getURL())
}

const isNodeIntegrationEnabled = function (sender) {
  return sender.getLastWebPreferences().nodeIntegration === true
}

// Checks whether |sender| can access the |target|:
const canAccessWindow = function (sender, target) {
  return isChildWindow(sender, target) ||
         isScriptableWindow(sender, target) ||
         isNodeIntegrationEnabled(sender)
}

// Routed window.open messages with raw options
ipcMainInternal.on('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_OPEN', (event, url, frameName, features) => {
  if (url == null || url === '') url = 'about:blank'
  if (frameName == null) frameName = ''
  if (features == null) features = ''

  const disposition = 'new-window'
  const { options, additionalFeatures } = convertFeaturesString(features, frameName)
  const referrer = { url: '', policy: 'default' }
  ipcMainInternal.emit('ELECTRON_GUEST_WINDOW_MANAGER_INTERNAL_WINDOW_OPEN', event,
    url, referrer, frameName, disposition, options, additionalFeatures, null)
})

// Routed window.open messages with fully parsed options
ipcMainInternal.on('ELECTRON_GUEST_WINDOW_MANAGER_INTERNAL_WINDOW_OPEN', function (event, url, referrer,
  frameName, disposition, options, additionalFeatures, postData) {
  options = mergeBrowserWindowOptions(event.sender, options)
  const postBody = postData ? {
    data: postData,
    headers: headersForPostData(postData)
  } : null

  event.sender.emit('new-window', event, url, frameName, disposition, options, additionalFeatures, referrer, postBody)
  const { newGuest } = event
  if ((event.sender.getType() === 'webview' && event.sender.getLastWebPreferences().disablePopups) || event.defaultPrevented) {
    if (newGuest != null) {
      if (options.webContents === newGuest.webContents) {
        // the webContents is not changed, so set defaultPrevented to false to
        // stop the callers of this event from destroying the webContents.
        event.defaultPrevented = false
      }
      event.returnValue = setupGuest(event.sender, frameName, newGuest, options)
    } else {
      event.returnValue = null
    }
  } else {
    event.returnValue = createGuest(event.sender, url, referrer, frameName, options, postData)
  }
})

const makeSafeHandler = function (handler) {
  return (event, guestId, ...args) => {
    const guestContents = webContents.fromId(guestId)
    if (!guestContents) {
      throw new Error(`Invalid guestId: ${guestId}`)
    }

    return handler(event, guestContents, ...args)
  }
}

const handleMessage = function (channel, handler) {
  ipcMainInternal.handle(channel, makeSafeHandler(handler))
}

const handleMessageSync = function (channel, handler) {
  ipcMainUtils.handleSync(channel, makeSafeHandler(handler))
}

const assertCanAccessWindow = function (contents, guestContents) {
  if (!canAccessWindow(contents, guestContents)) {
    console.error(`Blocked ${contents.getURL()} from accessing guestId: ${guestContents.id}`)
    throw new Error(`Access denied to guestId: ${guestContents.id}`)
  }
}

const windowMethods = new Set([
  'destroy',
  'focus',
  'blur'
])

handleMessage('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_METHOD', (event, guestContents, method, ...args) => {
  assertCanAccessWindow(event.sender, guestContents)

  if (!windowMethods.has(method)) {
    console.error(`Blocked ${event.sender.getURL()} from calling method: ${method}`)
    throw new Error(`Invalid method: ${method}`)
  }

  return getGuestWindow(guestContents)[method](...args)
})

handleMessage('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_POSTMESSAGE', (event, guestContents, message, targetOrigin, sourceOrigin) => {
  if (targetOrigin == null) {
    targetOrigin = '*'
  }

  // The W3C does not seem to have word on how postMessage should work when the
  // origins do not match, so we do not do |canAccessWindow| check here since
  // postMessage across origins is useful and not harmful.
  if (targetOrigin === '*' || isSameOrigin(guestContents.getURL(), targetOrigin)) {
    const sourceId = event.sender.id
    guestContents._sendInternal('ELECTRON_GUEST_WINDOW_POSTMESSAGE', sourceId, message, sourceOrigin)
  }
})

const webContentsMethodsAsync = new Set([
  'loadURL',
  'executeJavaScript',
  'print'
])

handleMessage('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', (event, guestContents, method, ...args) => {
  assertCanAccessWindow(event.sender, guestContents)

  if (!webContentsMethodsAsync.has(method)) {
    console.error(`Blocked ${event.sender.getURL()} from calling method: ${method}`)
    throw new Error(`Invalid method: ${method}`)
  }

  return guestContents[method](...args)
})

const webContentsMethodsSync = new Set([
  'getURL'
])

handleMessageSync('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', (event, guestContents, method, ...args) => {
  assertCanAccessWindow(event.sender, guestContents)

  if (!webContentsMethodsSync.has(method)) {
    console.error(`Blocked ${event.sender.getURL()} from calling method: ${method}`)
    throw new Error(`Invalid method: ${method}`)
  }

  return guestContents[method](...args)
})

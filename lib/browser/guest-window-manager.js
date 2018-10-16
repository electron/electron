'use strict'

const { BrowserWindow, webContents } = require('electron')
const { isSameOrigin } = process.atomBinding('v8_util')
const ipcMain = require('@electron/internal/browser/ipc-main-internal')
const parseFeaturesString = require('@electron/internal/common/parse-features-string')

const hasProp = {}.hasOwnProperty
const frameToGuest = new Map()

// Security options that child windows will always inherit from parent windows
const inheritedWebPreferences = new Map([
  ['contextIsolation', true],
  ['javascript', false],
  ['nativeWindowOpen', true],
  ['nodeIntegration', false],
  ['enableRemoteModule', false],
  ['sandbox', true],
  ['webviewTag', false]
])

// Copy attribute of |parent| to |child| if it is not defined in |child|.
const mergeOptions = function (child, parent, visited) {
  // Check for circular reference.
  if (visited == null) visited = new Set()
  if (visited.has(parent)) return

  visited.add(parent)
  for (const key in parent) {
    if (key === 'isBrowserView') continue
    if (!hasProp.call(parent, key)) continue
    if (key in child && key !== 'webPreferences') continue

    const value = parent[key]
    if (typeof value === 'object') {
      child[key] = mergeOptions(child[key] || {}, value, visited)
    } else {
      child[key] = value
    }
  }
  visited.delete(parent)

  return child
}

// Merge |options| with the |embedder|'s window's options.
const mergeBrowserWindowOptions = function (embedder, options) {
  if (options.webPreferences == null) {
    options.webPreferences = {}
  }
  if (embedder.browserWindowOptions != null) {
    let parentOptions = embedder.browserWindowOptions

    // if parent's visibility is available, that overrides 'show' flag (#12125)
    const win = BrowserWindow.fromWebContents(embedder.webContents)
    if (win != null) {
      parentOptions = { ...embedder.browserWindowOptions, show: win.isVisible() }
    }

    // Inherit the original options if it is a BrowserWindow.
    mergeOptions(options, parentOptions)
  } else {
    // Or only inherit webPreferences if it is a webview.
    mergeOptions(options.webPreferences, embedder.getLastWebPreferences())
  }

  // Inherit certain option values from parent window
  const webPreferences = embedder.getLastWebPreferences()
  for (const [name, value] of inheritedWebPreferences) {
    if (webPreferences[name] === value) {
      options.webPreferences[name] = value
    }
  }

  // Sets correct openerId here to give correct options to 'new-window' event handler
  options.webPreferences.openerId = embedder.id

  return options
}

// Setup a new guest with |embedder|
const setupGuest = function (embedder, frameName, guest, options) {
  // When |embedder| is destroyed we should also destroy attached guest, and if
  // guest is closed by user then we should prevent |embedder| from double
  // closing guest.
  const guestId = guest.webContents.id
  const closedByEmbedder = function () {
    guest.removeListener('closed', closedByUser)
    guest.destroy()
  }
  const closedByUser = function () {
    embedder._sendInternal('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_CLOSED_' + guestId)
    embedder.removeListener('render-view-deleted', closedByEmbedder)
  }
  embedder.once('render-view-deleted', closedByEmbedder)
  guest.once('closed', closedByUser)
  if (frameName) {
    frameToGuest.set(frameName, guest)
    guest.frameName = frameName
    guest.once('closed', function () {
      frameToGuest.delete(frameName)
    })
  }
  return guestId
}

// Create a new guest created by |embedder| with |options|.
const createGuest = function (embedder, url, referrer, frameName, options, postData) {
  let guest = frameToGuest.get(frameName)
  if (frameName && (guest != null)) {
    guest.loadURL(url)
    return guest.webContents.id
  }

  // Remember the embedder window's id.
  if (options.webPreferences == null) {
    options.webPreferences = {}
  }

  guest = new BrowserWindow(options)
  if (!options.webContents || url !== 'about:blank') {
    // We should not call `loadURL` if the window was constructed from an
    // existing webContents(window.open in a sandboxed renderer) and if the url
    // is not 'about:blank'.
    //
    // Navigating to the url when creating the window from an existing
    // webContents would not be necessary(it will navigate there anyway), but
    // apparently there's a bug that allows the child window to be scripted by
    // the opener, even when the child window is from another origin.
    //
    // That's why the second condition(url !== "about:blank") is required: to
    // force `OverrideSiteInstanceForNavigation` to be called and consequently
    // spawn a new renderer if the new window is targeting a different origin.
    //
    // If the URL is "about:blank", then it is very likely that the opener just
    // wants to synchronously script the popup, for example:
    //
    //     let popup = window.open()
    //     popup.document.body.write('<h1>hello</h1>')
    //
    // The above code would not work if a navigation to "about:blank" is done
    // here, since the window would be cleared of all changes in the next tick.
    const loadOptions = {
      httpReferrer: referrer
    }
    if (postData != null) {
      loadOptions.postData = postData
      loadOptions.extraHeaders = 'content-type: application/x-www-form-urlencoded'
      if (postData.length > 0) {
        const postDataFront = postData[0].bytes.toString()
        const boundary = /^--.*[^-\r\n]/.exec(postDataFront)
        if (boundary != null) {
          loadOptions.extraHeaders = `content-type: multipart/form-data; boundary=${boundary[0].substr(2)}`
        }
      }
    }
    guest.loadURL(url, loadOptions)
  }

  return setupGuest(embedder, frameName, guest, options)
}

const getGuestWindow = function (guestContents) {
  let guestWindow = BrowserWindow.fromWebContents(guestContents)
  if (guestWindow == null) {
    const hostContents = guestContents.hostWebContents
    if (hostContents != null) {
      guestWindow = BrowserWindow.fromWebContents(hostContents)
    }
  }
  return guestWindow
}

// Checks whether |sender| can access the |target|:
// 1. Check whether |sender| is the parent of |target|.
// 2. Check whether |sender| has node integration, if so it is allowed to
//    do anything it wants.
// 3. Check whether the origins match.
//
// However it allows a child window without node integration but with same
// origin to do anything it wants, when its opener window has node integration.
// The W3C does not have anything on this, but from my understanding of the
// security model of |window.opener|, this should be fine.
const canAccessWindow = function (sender, target) {
  return (target.getLastWebPreferences().openerId === sender.id) ||
         (sender.getLastWebPreferences().nodeIntegration === true) ||
         isSameOrigin(sender.getURL(), target.getURL())
}

// Routed window.open messages with raw options
ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_OPEN', (event, url, frameName, features) => {
  if (url == null || url === '') url = 'about:blank'
  if (frameName == null) frameName = ''
  if (features == null) features = ''

  const options = {}

  const ints = ['x', 'y', 'width', 'height', 'minWidth', 'maxWidth', 'minHeight', 'maxHeight', 'zoomFactor']
  const webPreferences = ['zoomFactor', 'nodeIntegration', 'enableRemoteModule', 'preload', 'javascript', 'contextIsolation', 'webviewTag']
  const disposition = 'new-window'

  // Used to store additional features
  const additionalFeatures = []

  // Parse the features
  parseFeaturesString(features, function (key, value) {
    if (value === undefined) {
      additionalFeatures.push(key)
    } else {
      // Don't allow webPreferences to be set since it must be an object
      // that cannot be directly overridden
      if (key === 'webPreferences') return

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

  for (const name of ints) {
    if (options[name] != null) {
      options[name] = parseInt(options[name], 10)
    }
  }

  const referrer = { url: '', policy: 'default' }
  ipcMain.emit('ELECTRON_GUEST_WINDOW_MANAGER_INTERNAL_WINDOW_OPEN', event,
    url, referrer, frameName, disposition, options, additionalFeatures)
})

// Routed window.open messages with fully parsed options
ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_INTERNAL_WINDOW_OPEN', function (event, url, referrer,
  frameName, disposition, options,
  additionalFeatures, postData) {
  options = mergeBrowserWindowOptions(event.sender, options)
  event.sender.emit('new-window', event, url, frameName, disposition, options, additionalFeatures, referrer)
  const { newGuest } = event
  if ((event.sender.isGuest() && !event.sender.allowPopups) || event.defaultPrevented) {
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

ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_CLOSE', function (event, guestId) {
  const guestContents = webContents.fromId(guestId)
  if (guestContents == null) return

  if (!canAccessWindow(event.sender, guestContents)) {
    console.error(`Blocked ${event.sender.getURL()} from closing its opener.`)
    return
  }

  const guestWindow = getGuestWindow(guestContents)
  if (guestWindow != null) guestWindow.destroy()
})

ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_METHOD', function (event, guestId, method, ...args) {
  const guestContents = webContents.fromId(guestId)
  if (guestContents == null) {
    event.returnValue = null
    return
  }

  if (!canAccessWindow(event.sender, guestContents)) {
    console.error(`Blocked ${event.sender.getURL()} from calling ${method} on its opener.`)
    event.returnValue = null
    return
  }

  const guestWindow = getGuestWindow(guestContents)
  if (guestWindow != null) {
    event.returnValue = guestWindow[method](...args)
  } else {
    event.returnValue = null
  }
})

ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_POSTMESSAGE', function (event, guestId, message, targetOrigin, sourceOrigin) {
  if (targetOrigin == null) {
    targetOrigin = '*'
  }

  const guestContents = webContents.fromId(guestId)
  if (guestContents == null) return

  // The W3C does not seem to have word on how postMessage should work when the
  // origins do not match, so we do not do |canAccessWindow| check here since
  // postMessage across origins is useful and not harmful.
  if (targetOrigin === '*' || isSameOrigin(guestContents.getURL(), targetOrigin)) {
    const sourceId = event.sender.id
    guestContents._sendInternal('ELECTRON_GUEST_WINDOW_POSTMESSAGE', sourceId, message, sourceOrigin)
  }
})

ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', function (event, guestId, method, ...args) {
  const guestContents = webContents.fromId(guestId)
  if (guestContents == null) return

  if (canAccessWindow(event.sender, guestContents)) {
    guestContents[method](...args)
  } else {
    console.error(`Blocked ${event.sender.getURL()} from calling ${method} on its opener.`)
  }
})

ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD_SYNC', function (event, guestId, method, ...args) {
  const guestContents = webContents.fromId(guestId)
  if (guestContents == null) {
    event.returnValue = null
    return
  }

  if (canAccessWindow(event.sender, guestContents)) {
    event.returnValue = guestContents[method](...args)
  } else {
    console.error(`Blocked ${event.sender.getURL()} from calling ${method} on its opener.`)
    event.returnValue = null
  }
})

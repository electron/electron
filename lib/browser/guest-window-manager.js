'use strict'

const {BrowserWindow, ipcMain, webContents} = require('electron')
const {isSameOrigin} = process.atomBinding('v8_util')

const hasProp = {}.hasOwnProperty
const frameToGuest = {}

// Copy attribute of |parent| to |child| if it is not defined in |child|.
const mergeOptions = function (child, parent) {
  let key, value
  for (key in parent) {
    if (!hasProp.call(parent, key)) continue
    value = parent[key]
    if (!(key in child)) {
      if (typeof value === 'object') {
        child[key] = mergeOptions({}, value)
      } else {
        child[key] = value
      }
    }
  }
  return child
}

// Merge |options| with the |embedder|'s window's options.
const mergeBrowserWindowOptions = function (embedder, options) {
  if (options.webPreferences == null) {
    options.webPreferences = {}
  }
  if (embedder.browserWindowOptions != null) {
    // Inherit the original options if it is a BrowserWindow.
    mergeOptions(options, embedder.browserWindowOptions)
  } else {
    // Or only inherit web-preferences if it is a webview.
    mergeOptions(options.webPreferences, embedder.getWebPreferences())
  }

  // Disable node integration on child window if disabled on parent window
  if (embedder.getWebPreferences().nodeIntegration === false) {
    options.webPreferences.nodeIntegration = false
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
    embedder.send('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_CLOSED_' + guestId)
    embedder.removeListener('render-view-deleted', closedByEmbedder)
  }
  if (!options.webPreferences.sandbox) {
    // These events should only be handled when the guest window is opened by a
    // non-sandboxed renderer for two reasons:
    //
    // - `render-view-deleted` is emitted when the popup is closed by the user,
    //   and that will eventually result in NativeWindow::NotifyWindowClosed
    //   using a dangling pointer since `destroy()` would have been called by
    //   `closeByEmbedded`
    // - No need to emit `ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_CLOSED_` since
    //   there's no renderer code listening to it.,
    embedder.once('render-view-deleted', closedByEmbedder)
    guest.once('closed', closedByUser)
  }
  if (frameName) {
    frameToGuest[frameName] = guest
    guest.frameName = frameName
    guest.once('closed', function () {
      delete frameToGuest[frameName]
    })
  }
  return guestId
}

// Create a new guest created by |embedder| with |options|.
const createGuest = function (embedder, url, frameName, options, postData) {
  let guest = frameToGuest[frameName]
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
    const loadOptions = {}
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

const getGuestWindow = function (guestId) {
  const guestContents = webContents.fromId(guestId)
  if (guestContents == null) return

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
  return (target.getWebPreferences().openerId === sender.webContents.id) ||
         (sender.getWebPreferences().nodeIntegration === true) ||
         isSameOrigin(sender.getURL(), target.getURL())
}

// Routed window.open messages.
ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_OPEN', function (event, url, frameName,
                                                                  disposition, options,
                                                                  additionalFeatures, postData) {
  options = mergeBrowserWindowOptions(event.sender, options)
  event.sender.emit('new-window', event, url, frameName, disposition, options, additionalFeatures)
  const newGuest = event.newGuest
  if ((event.sender.isGuest() && !event.sender.allowPopups) || event.defaultPrevented) {
    if (newGuest !== undefined && newGuest !== null) {
      event.returnValue = setupGuest(event.sender, frameName, newGuest, options)
    } else {
      event.returnValue = null
    }
  } else {
    event.returnValue = createGuest(event.sender, url, frameName, options, postData)
  }
})

ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_CLOSE', function (event, guestId) {
  const guestWindow = getGuestWindow(guestId)
  if (guestWindow != null && canAccessWindow(event.sender, guestWindow.webContents)) {
    guestWindow.destroy()
  }
})

ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_METHOD', function (event, guestId, method, ...args) {
  const guestWindow = getGuestWindow(guestId)
  if (guestWindow != null && canAccessWindow(event.sender, guestWindow.webContents)) {
    event.returnValue = guestWindow[method].apply(guestWindow, args)
  } else {
    event.returnValue = null
  }
})

ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_POSTMESSAGE', function (event, guestId, message, targetOrigin, sourceOrigin) {
  const guestContents = webContents.fromId(guestId)
  if (guestContents == null) return

  // The W3C does not seem to have word on how postMessage should work when the
  // origins do not match, so we do not do |canAccessWindow| check here since
  // postMessage across origins is usefull and not harmful.
  if (guestContents.getURL().indexOf(targetOrigin) === 0 || targetOrigin === '*') {
    const sourceId = event.sender.id
    guestContents.send('ELECTRON_GUEST_WINDOW_POSTMESSAGE', sourceId, message, sourceOrigin)
  }
})

ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', function (event, guestId, method, ...args) {
  const guestContents = webContents.fromId(guestId)
  if (guestContents != null && canAccessWindow(event.sender, guestContents)) {
    guestContents[method].apply(guestContents, args)
  }
})

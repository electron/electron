'use strict'

const {BrowserWindow, ipcMain, webContents} = require('electron')

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
  if (embedder.browserWindowOptions != null) {
    // Inherit the original options if it is a BrowserWindow.
    mergeOptions(options, embedder.browserWindowOptions)
  } else {
    // Or only inherit web-preferences if it is a webview.
    if (options.webPreferences == null) {
      options.webPreferences = {}
    }
    mergeOptions(options.webPreferences, embedder.getWebPreferences())
  }

  // Disable node integration on child window if disabled on parent window
  if (embedder.getWebPreferences().nodeIntegration === false) {
    options.webPreferences.nodeIntegration = false
  }

  return options
}

// Setup a new guest with |embedder|
const setupGuest = function (embedder, frameName, guest) {
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
    guest.loadURL(url)
  }
  // When |embedder| is destroyed we should also destroy attached guest, and if
  // guest is closed by user then we should prevent |embedder| from double
  // closing guest.
  const guestId = guest.id
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
  return guest.id
}	

// Create a new guest created by |embedder| with |options|.
const createGuest = function (embedder, url, frameName, options) {
  const guest = frameToGuest[frameName]
  if (frameName && (guest != null)) {
    guest.loadURL(url)
    return guest.id
  }

  // Remember the embedder window's id.
  if (options.webPreferences == null) {
    options.webPreferences = {}
  }
  options.webPreferences.openerId = embedder.id
  guest = new BrowserWindow(options)
  guest.loadURL(url)

  return setupGuest(embedder, frameName, guest)
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

// Routed window.open messages.
ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_OPEN', function (event, url, frameName, disposition, options, additionalFeatures) {
  options = mergeBrowserWindowOptions(event.sender, options)
  event.sender.emit('new-window', event, url, frameName, disposition, options, additionalFeatures)
  var newGuest = event.newGuest
  if ((event.sender.isGuest() && !event.sender.allowPopups) || event.defaultPrevented) {
	if (newGuest != undefined && newGuest != null) {
      event.returnValue = setupGuest(event.sender, frameName, newGuest)
	} else {
      event.returnValue = null
	}
  } else {
    event.returnValue = createGuest(event.sender, url, frameName, options)
  }
})

ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_CLOSE', function (event, guestId) {
  const guestWindow = getGuestWindow(guestId)
  if (guestWindow != null) guestWindow.destroy()
})

ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_METHOD', function (event, guestId, method, ...args) {
  const guestWindow = getGuestWindow(guestId)
  event.returnValue = guestWindow != null ? guestWindow[method].apply(guestWindow, args) : null
})

ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_POSTMESSAGE', function (event, guestId, message, targetOrigin, sourceOrigin) {
  const guestContents = webContents.fromId(guestId)
  if (guestContents == null) return

  if (guestContents.getURL().indexOf(targetOrigin) === 0 || targetOrigin === '*') {
    const sourceId = event.sender.id
    guestContents.send('ELECTRON_GUEST_WINDOW_POSTMESSAGE', sourceId, message, sourceOrigin)
  }
})

ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', function (event, guestId, method, ...args) {
  const guestContents = webContents.fromId(guestId)
  if (guestContents != null) guestContents[method].apply(guestContents, args)
})

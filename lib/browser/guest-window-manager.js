'use strict'

const ipcMain = require('electron').ipcMain
const BrowserWindow = require('electron').BrowserWindow

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

// Create a new guest created by |embedder| with |options|.
const createGuest = function (embedder, url, frameName, options) {
  let guest = frameToGuest[frameName]
  if (frameName && (guest != null)) {
    guest.loadURL(url)
    return guest.id
  }

  // Remember the embedder window's id.
  if (options.webPreferences == null) {
    options.webPreferences = {}
  }
  const embedderWindow = BrowserWindow.fromWebContents(embedder)
  options.webPreferences.openerId = embedderWindow != null ? embedderWindow.id : void 0
  guest = new BrowserWindow(options)
  guest.loadURL(url)

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
  embedder.once('render-view-deleted', closedByEmbedder)
  guest.once('closed', closedByUser)

  if (frameName) {
    frameToGuest[frameName] = guest
    guest.frameName = frameName
    guest.once('closed', function () {
      delete frameToGuest[frameName]
    })
  }

  return guest.id
}

// Routed window.open messages.
ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_OPEN', function (event, url, frameName, disposition, options) {
  options = mergeBrowserWindowOptions(event.sender, options)
  event.sender.emit('new-window', event, url, frameName, disposition, options)
  if ((event.sender.isGuest() && !event.sender.allowPopups) || event.defaultPrevented) {
    event.returnValue = null
  } else {
    event.returnValue = createGuest(event.sender, url, frameName, options)
  }
})

ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_CLOSE', function (event, guestId) {
  const guestWindow = BrowserWindow.fromId(guestId)
  if (guestWindow != null) guestWindow.destroy()
})

ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_METHOD', function (event, guestId, method, ...args) {
  const guestWindow = BrowserWindow.fromId(guestId)
  event.returnValue = guestWindow != null ? guestWindow[method].apply(guestWindow, args) : void 0
})

ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_POSTMESSAGE', function (event, guestId, message, targetOrigin, sourceOrigin) {
  const sourceContents = BrowserWindow.fromWebContents(event.sender)
  const sourceId = sourceContents != null ? sourceContents.id : void 0
  if (sourceId == null) {
    return
  }

  const guestWindow = BrowserWindow.fromId(guestId)
  const guestContents = guestWindow != null ? guestWindow.webContents : void 0
  if ((guestContents != null ? guestContents.getURL().indexOf(targetOrigin) : void 0) === 0 || targetOrigin === '*') {
    guestContents != null ? guestContents.send('ELECTRON_GUEST_WINDOW_POSTMESSAGE', sourceId, message, sourceOrigin) : void 0
  }
})

ipcMain.on('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', function (event, guestId, method, ...args) {
  const guestWindow = BrowserWindow.fromId(guestId)
  if (guestWindow != null) {
    const guestContents = guestWindow.webContents
    if (guestContents != null) {
      guestContents[method].apply(guestContents, args)
    }
  }
})

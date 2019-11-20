'use strict'

const { BrowserWindow } = require('electron')

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
  ['webviewTag', false],
  ['nodeIntegrationInSubFrames', false]
])

// Copy attribute of |parent| to |child| if it is not defined in |child|.
const mergeOptions = function (child, parent, visited) {
  // Check for circular reference.
  if (visited == null) visited = new Set()
  if (visited.has(parent)) return

  visited.add(parent)
  for (const key in parent) {
    if (key === 'type') continue
    if (!hasProp.call(parent, key)) continue
    if (key in child && key !== 'webPreferences') continue

    const value = parent[key]
    if (typeof value === 'object' && !Array.isArray(value)) {
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
      parentOptions = {
        ...win.getBounds(),
        ...embedder.browserWindowOptions,
        show: win.isVisible()
      }
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

// Figure out appropriate headers for post data
const headersForPostData = function (postData) {
  if (postData.length > 0) {
    const postDataFront = postData[0].bytes.toString()
    const boundary = /^--.*[^-\r\n]/.exec(postDataFront)
    if (boundary != null) {
      return `content-type: multipart/form-data; boundary=${boundary[0].substr(2)}`
    }
  }
  return 'content-type: application/x-www-form-urlencoded'
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
    embedder.removeListener('current-render-view-deleted', closedByEmbedder)
  }
  embedder.once('current-render-view-deleted', closedByEmbedder)
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
  if (!options.webContents) {
    // We should not call `loadURL` if the window was constructed from an
    // existing webContents (window.open in a sandboxed renderer).
    //
    // Navigating to the url when creating the window from an existing
    // webContents is not necessary (it will navigate there anyway).
    const loadOptions = {
      httpReferrer: referrer
    }
    if (postData != null) {
      loadOptions.postData = postData
      loadOptions.extraHeaders = headersForPostData(postData)
    }
    guest.loadURL(url, loadOptions)
  }

  return setupGuest(embedder, frameName, guest, options)
}

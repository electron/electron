'use strict'

const ipcMain = require('electron').ipcMain
const webContents = require('electron').webContents

// Doesn't exist in early initialization.
let webViewManager = null

const supportedWebViewEvents = [
  'load-commit',
  'did-attach',
  'did-finish-load',
  'did-fail-load',
  'did-frame-finish-load',
  'did-start-loading',
  'did-stop-loading',
  'did-get-response-details',
  'did-get-redirect-request',
  'dom-ready',
  'console-message',
  'devtools-opened',
  'devtools-closed',
  'devtools-focused',
  'new-window',
  'will-navigate',
  'did-navigate',
  'did-navigate-in-page',
  'close',
  'crashed',
  'gpu-crashed',
  'plugin-crashed',
  'destroyed',
  'page-title-updated',
  'page-favicon-updated',
  'enter-html-full-screen',
  'leave-html-full-screen',
  'media-started-playing',
  'media-paused',
  'found-in-page',
  'did-change-theme-color',
  'update-target-url'
]

let nextGuestInstanceId = 0
const guestInstances = {}
const embedderElementsMap = {}

// Moves the last element of array to the first one.
const moveLastToFirst = function (list) {
  return list.unshift(list.pop())
}

// Generate guestInstanceId.
const getNextGuestInstanceId = function () {
  return ++nextGuestInstanceId
}

// Create a new guest instance.
const createGuest = function (embedder, params) {
  if (webViewManager == null) {
    webViewManager = process.atomBinding('web_view_manager')
  }

  const guestInstanceId = getNextGuestInstanceId(embedder)
  const guest = webContents.create({
    isGuest: true,
    partition: params.partition,
    embedder: embedder
  })
  guestInstances[guestInstanceId] = {
    guest: guest,
    embedder: embedder
  }

  watchEmbedder(embedder)

  // Init guest web view after attached.
  guest.on('did-attach', function () {
    let opts
    params = this.attachParams
    delete this.attachParams
    this.viewInstanceId = params.instanceId
    this.setSize({
      normal: {
        width: params.elementWidth,
        height: params.elementHeight
      },
      enableAutoSize: params.autosize,
      min: {
        width: params.minwidth,
        height: params.minheight
      },
      max: {
        width: params.maxwidth,
        height: params.maxheight
      }
    })
    if (params.src) {
      opts = {}
      if (params.httpreferrer) {
        opts.httpReferrer = params.httpreferrer
      }
      if (params.useragent) {
        opts.userAgent = params.useragent
      }
      this.loadURL(params.src, opts)
    }
    guest.allowPopups = params.allowpopups
  })

  // Dispatch events to embedder.
  const fn = function (event) {
    guest.on(event, function (_, ...args) {
      const embedder = getEmbedder(guestInstanceId)
      if (!embedder) {
        return
      }
      embedder.send.apply(embedder, ['ELECTRON_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-' + guest.viewInstanceId, event].concat(args))
    })
  }
  for (const event of supportedWebViewEvents) {
    fn(event)
  }

  // Dispatch guest's IPC messages to embedder.
  guest.on('ipc-message-host', function (_, [channel, ...args]) {
    const embedder = getEmbedder(guestInstanceId)
    if (!embedder) {
      return
    }
    embedder.send.apply(embedder, ['ELECTRON_GUEST_VIEW_INTERNAL_IPC_MESSAGE-' + guest.viewInstanceId, channel].concat(args))
  })

  // Autosize.
  guest.on('size-changed', function (_, ...args) {
    const embedder = getEmbedder(guestInstanceId)
    if (!embedder) {
      return
    }
    embedder.send.apply(embedder, ['ELECTRON_GUEST_VIEW_INTERNAL_SIZE_CHANGED-' + guest.viewInstanceId].concat(args))
  })

  return guestInstanceId
}

// Attach the guest to an element of embedder.
const attachGuest = function (embedder, elementInstanceId, guestInstanceId, params) {
  let guest, guestInstance, key, oldKey, oldGuestInstanceId, ref1, webPreferences

  // Destroy the old guest when attaching.
  key = (embedder.getId()) + '-' + elementInstanceId
  oldGuestInstanceId = embedderElementsMap[key]
  if (oldGuestInstanceId != null) {
    // Reattachment to the same guest is just a no-op.
    if (oldGuestInstanceId === guestInstanceId) {
      return
    }

    destroyGuest(embedder, oldGuestInstanceId)
  }

  guestInstance = guestInstances[guestInstanceId]
  // If this isn't a valid guest instance then do nothing.
  if (!guestInstance) {
    return
  }
  guest = guestInstance.guest

  // If this guest is already attached to an element then remove it
  if (guestInstance.elementInstanceId) {
    oldKey = (guestInstance.embedder.getId()) + '-' + guestInstance.elementInstanceId
    delete embedderElementsMap[oldKey]
    webViewManager.removeGuest(guestInstance.embedder, guestInstanceId)
    guestInstance.embedder.send('ELECTRON_GUEST_VIEW_INTERNAL_DESTROY_GUEST-' + guest.viewInstanceId)
  }

  webPreferences = {
    guestInstanceId: guestInstanceId,
    nodeIntegration: (ref1 = params.nodeintegration) != null ? ref1 : false,
    plugins: params.plugins,
    zoomFactor: params.zoomFactor,
    webSecurity: !params.disablewebsecurity,
    blinkFeatures: params.blinkfeatures,
    disableBlinkFeatures: params.disableblinkfeatures
  }

  // parse the 'webpreferences' attribute string, if set
  // this uses the same parsing rules as window.open uses for its features
  if (typeof params.webpreferences === 'string') {
    // split the attribute's value by ','
    let webpreferencesTokens = params.webpreferences.split(/,\s*/)
    for (i = 0, len = webpreferencesTokens.length; i < len; i++) {
      // expected form is either a name by itself (true boolean flag)
      // or a key/value, in the form of 'name=value'
      // split the tokens by '='
      let pref = webpreferencesTokens[i]
      let prefTokens = pref.split(/\s*=/)
      name = prefTokens[0]
      value = prefTokens[1]
      if (!name) continue

      // interpret the value as a boolean, if possible 
      value = (value === 'yes' || value === '1') ? true : (value === 'no' || value === '0') ? false : value
      if (value === undefined) {
        // no value was specified, default it to true
        value = true
      }
      webPreferences[name] = value
    }
  }

  if (params.preload) {
    webPreferences.preloadURL = params.preload
  }
  webViewManager.addGuest(guestInstanceId, elementInstanceId, embedder, guest, webPreferences)
  guest.attachParams = params
  embedderElementsMap[key] = guestInstanceId

  guest.setEmbedder(embedder)
  guestInstance.embedder = embedder
  guestInstance.elementInstanceId = elementInstanceId

  watchEmbedder(embedder)
}

// Destroy an existing guest instance.
const destroyGuest = function (embedder, guestInstanceId) {
  if (!(guestInstanceId in guestInstances)) {
    return
  }

  let guestInstance = guestInstances[guestInstanceId]
  if (embedder !== guestInstance.embedder) {
    return
  }

  webViewManager.removeGuest(embedder, guestInstanceId)
  guestInstance.guest.destroy()
  delete guestInstances[guestInstanceId]

  const key = embedder.getId() + '-' + guestInstance.elementInstanceId
  return delete embedderElementsMap[key]
}

// Once an embedder has had a guest attached we watch it for destruction to
// destroy any remaining guests.
const watchedEmbedders = new Set()
const watchEmbedder = function (embedder) {
  if (watchedEmbedders.has(embedder)) {
    return
  }
  watchedEmbedders.add(embedder)

  const destroyEvents = ['will-destroy', 'crashed', 'did-navigate']
  const destroy = function () {
    for (const guestInstanceId of Object.keys(guestInstances)) {
      if (guestInstances[guestInstanceId].embedder === embedder) {
        destroyGuest(embedder, parseInt(guestInstanceId))
      }
    }

    for (const event of destroyEvents) {
      embedder.removeListener(event, destroy)
    }

    watchedEmbedders.delete(embedder)
  }

  for (const event of destroyEvents) {
    embedder.once(event, destroy)

    // Users might also listen to the crashed event, so we must ensure the guest
    // is destroyed before users' listener gets called. It is done by moving our
    // listener to the first one in queue.
    const listeners = embedder._events[event]
    if (Array.isArray(listeners)) {
      moveLastToFirst(listeners)
    }
  }
}

ipcMain.on('ELECTRON_GUEST_VIEW_MANAGER_CREATE_GUEST', function (event, params, requestId) {
  event.sender.send('ELECTRON_RESPONSE_' + requestId, createGuest(event.sender, params))
})

ipcMain.on('ELECTRON_GUEST_VIEW_MANAGER_ATTACH_GUEST', function (event, elementInstanceId, guestInstanceId, params) {
  attachGuest(event.sender, elementInstanceId, guestInstanceId, params)
})

ipcMain.on('ELECTRON_GUEST_VIEW_MANAGER_DESTROY_GUEST', function (event, guestInstanceId) {
  destroyGuest(event.sender, guestInstanceId)
})

ipcMain.on('ELECTRON_GUEST_VIEW_MANAGER_SET_SIZE', function (event, guestInstanceId, params) {
  const guestInstance = guestInstances[guestInstanceId]
  return guestInstance != null ? guestInstance.guest.setSize(params) : void 0
})

// Returns WebContents from its guest id.
exports.getGuest = function (guestInstanceId) {
  const guestInstance = guestInstances[guestInstanceId]
  return guestInstance != null ? guestInstance.guest : void 0
}

// Returns the embedder of the guest.
const getEmbedder = function (guestInstanceId) {
  const guestInstance = guestInstances[guestInstanceId]
  return guestInstance != null ? guestInstance.embedder : void 0
}
exports.getEmbedder = getEmbedder

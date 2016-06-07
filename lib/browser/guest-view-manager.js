'use strict'

const ipcMain = require('electron').ipcMain
const webContents = require('electron').webContents

// Doesn't exist in early initialization.
var webViewManager = null

var supportedWebViewEvents = [
  'load-commit',
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

var nextInstanceId = 0
var guestInstances = {}
var embedderElementsMap = {}
var reverseEmbedderElementsMap = {}

// Moves the last element of array to the first one.
var moveLastToFirst = function (list) {
  return list.unshift(list.pop())
}

// Generate guestInstanceId.
var getNextInstanceId = function () {
  return ++nextInstanceId
}

// Create a new guest instance.
var createGuest = function (embedder, params) {
  var destroy, destroyEvents, event, fn, guest, i, id, j, len, len1, listeners
  if (webViewManager == null) {
    webViewManager = process.atomBinding('web_view_manager')
  }
  id = getNextInstanceId(embedder)
  guest = webContents.create({
    isGuest: true,
    partition: params.partition,
    embedder: embedder
  })
  guestInstances[id] = {
    guest: guest,
    embedder: embedder
  }

  // Destroy guest when the embedder is gone or navigated.
  destroyEvents = ['will-destroy', 'crashed', 'did-navigate']
  destroy = function () {
    if (guestInstances[id] != null) {
      return destroyGuest(embedder, id)
    }
  }
  for (i = 0, len = destroyEvents.length; i < len; i++) {
    event = destroyEvents[i]
    embedder.once(event, destroy)

    // Users might also listen to the crashed event, so We must ensure the guest
    // is destroyed before users' listener gets called. It is done by moving our
    // listener to the first one in queue.
    listeners = embedder._events[event]
    if (Array.isArray(listeners)) {
      moveLastToFirst(listeners)
    }
  }
  guest.once('destroyed', function () {
    var j, len1, results
    results = []
    for (j = 0, len1 = destroyEvents.length; j < len1; j++) {
      event = destroyEvents[j]
      results.push(embedder.removeListener(event, destroy))
    }
    return results
  })

  // Init guest web view after attached.
  guest.once('did-attach', function () {
    var opts
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
  fn = function (event) {
    return guest.on(event, function (_, ...args) {
      embedder.send.apply(embedder, ['ELECTRON_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-' + guest.viewInstanceId, event].concat(args))
    })
  }
  for (j = 0, len1 = supportedWebViewEvents.length; j < len1; j++) {
    event = supportedWebViewEvents[j]
    fn(event)
  }

  // Dispatch guest's IPC messages to embedder.
  guest.on('ipc-message-host', function (_, [channel, ...args]) {
    embedder.send.apply(embedder, ['ELECTRON_GUEST_VIEW_INTERNAL_IPC_MESSAGE-' + guest.viewInstanceId, channel].concat(args))
  })

  // Autosize.
  guest.on('size-changed', function (_, ...args) {
    embedder.send.apply(embedder, ['ELECTRON_GUEST_VIEW_INTERNAL_SIZE_CHANGED-' + guest.viewInstanceId].concat(args))
  })
  return id
}

// Attach the guest to an element of embedder.
var attachGuest = function (embedder, elementInstanceId, guestInstanceId, params) {
  var guest, key, oldGuestInstanceId, ref1, webPreferences
  guest = guestInstances[guestInstanceId].guest

  // Destroy the old guest when attaching.
  key = (embedder.getId()) + '-' + elementInstanceId
  oldGuestInstanceId = embedderElementsMap[key]
  if (oldGuestInstanceId != null) {
    // Reattachment to the same guest is not currently supported.
    if (oldGuestInstanceId === guestInstanceId) {
      return
    }
    if (guestInstances[oldGuestInstanceId] == null) {
      return
    }
    destroyGuest(embedder, oldGuestInstanceId)
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

  if (params.preload) {
    webPreferences.preloadURL = params.preload
  }
  webViewManager.addGuest(guestInstanceId, elementInstanceId, embedder, guest, webPreferences)
  guest.attachParams = params
  embedderElementsMap[key] = guestInstanceId
  reverseEmbedderElementsMap[guestInstanceId] = key
}

// Destroy an existing guest instance.
var destroyGuest = function (embedder, id) {
  var key
  webViewManager.removeGuest(embedder, id)
  guestInstances[id].guest.destroy()
  delete guestInstances[id]
  key = reverseEmbedderElementsMap[id]
  if (key != null) {
    delete reverseEmbedderElementsMap[id]
    return delete embedderElementsMap[key]
  }
}

ipcMain.on('ELECTRON_GUEST_VIEW_MANAGER_CREATE_GUEST', function (event, params, requestId) {
  event.sender.send('ELECTRON_RESPONSE_' + requestId, createGuest(event.sender, params))
})

ipcMain.on('ELECTRON_GUEST_VIEW_MANAGER_ATTACH_GUEST', function (event, elementInstanceId, guestInstanceId, params) {
  attachGuest(event.sender, elementInstanceId, guestInstanceId, params)
})

ipcMain.on('ELECTRON_GUEST_VIEW_MANAGER_DESTROY_GUEST', function (event, id) {
  destroyGuest(event.sender, id)
})

ipcMain.on('ELECTRON_GUEST_VIEW_MANAGER_SET_SIZE', function (event, id, params) {
  var ref1
  return (ref1 = guestInstances[id]) != null ? ref1.guest.setSize(params) : void 0
})

// Returns WebContents from its guest id.
exports.getGuest = function (id) {
  var ref1
  return (ref1 = guestInstances[id]) != null ? ref1.guest : void 0
}

// Returns the embedder of the guest.
exports.getEmbedder = function (id) {
  var ref1
  return (ref1 = guestInstances[id]) != null ? ref1.embedder : void 0
}

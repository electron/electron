'use strict'

const ipcMain = require('electron').ipcMain
const webContents = require('electron').webContents
const BrowserWindow = require('electron').BrowserWindow
const dialog = require('electron').dialog

// Doesn't exist in early initialization.
var webViewManager = null

var supportedWebViewEvents = [
  'load-start',
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
  'set-active'
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

var createWebContents = function (embedder, params) {
  return webContents.create({
    isGuest: true,
    partition: params.partition,
    embedder: embedder
  })
}

// Create a new guest instance.
var addGuest = function (embedder, guestWebContents, guestInstanceId) {
  var destroy, destroyEvents, event, fn, guest, i, id, j, len, len1, listeners
  if (webViewManager == null) {
    webViewManager = process.atomBinding('web_view_manager')
  }
  id = guestInstanceId || getNextInstanceId(embedder)
  guest = guestWebContents
  guestInstances[id] = {
    guest,
    embedder
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
    var opts, params
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
      return embedder.send.apply(embedder, ['ATOM_SHELL_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-' + guest.viewInstanceId, event].concat(args))
    })
  }
  for (j = 0, len1 = supportedWebViewEvents.length; j < len1; j++) {
    event = supportedWebViewEvents[j]
    fn(event)
  }

  // Dispatch guest's IPC messages to embedder.
  guest.on('ipc-message-host', function (_, [channel, ...args]) {
    return embedder.send.apply(embedder, ['ATOM_SHELL_GUEST_VIEW_INTERNAL_IPC_MESSAGE-' + guest.viewInstanceId, channel].concat(args))
  })

  // Autosize.
  guest.on('size-changed', function (_, ...args) {
    return embedder.send.apply(embedder, ['ATOM_SHELL_GUEST_VIEW_INTERNAL_SIZE_CHANGED-' + guest.viewInstanceId].concat(args))
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
    allowDisplayingInsecureContent: (ref1 = params.allowDisplayingInsecureContent) != null ? ref1 : false,
    allowRunningInsecureContent: (ref1 = params.allowRunningInsecureContent) != null ? ref1 : false,
    webSecurity: !params.disablewebsecurity,
    blinkFeatures: params.blinkfeatures
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
  var guest, key
  webViewManager.removeGuest(embedder, id)
  guest = guestInstances[id].guest
  if (guest == null) {
    return
  }
  guest.destroy()
  delete guestInstances[id]
  key = reverseEmbedderElementsMap[id]
  if (key != null) {
    delete reverseEmbedderElementsMap[id]
    return delete embedderElementsMap[key]
  }
}

process.on('ATOM_SHELL_GUEST_VIEW_MANAGER_TAB_OPEN', function () {
  var args, event, frameName, options, url, disposition
  event = arguments[0], args = 2 <= arguments.length ? [].slice.call(arguments, 1) : []
  url = args[0], frameName = args[1], disposition = args[2], options = args[3]
  event.sender.emit('new-window', event, url, frameName, disposition, options)
  if ((event.sender.isGuest() && !event.sender.allowPopups) || event.defaultPrevented) {
    return event.returnValue = null
  } else {
    return event.returnValue = addGuest(event.sender, createWebContents(event.sender, options))
  }
})

process.on('ATOM_SHELL_GUEST_VIEW_MANAGER_NEXT_INSTANCE_ID', function (event) {
  return event.returnValue = getNextInstanceId(event.sender)
})

process.on('ATOM_SHELL_GUEST_VIEW_MANAGER_REGISTER_GUEST', function (event, guest, id) {
  guestInstances[id] = { guest }
})

ipcMain.on('window-alert', function (event, message, title) {
  var buttons
  if (title == null) {
    title = ''
  }
  buttons = ['OK']
  message = message.toString()
  dialog.showMessageBox(BrowserWindow.getFocusedWindow(), {
    message: message,
    title: title,
    buttons: buttons
  })
  // Alert should always return undefined.
})

ipcMain.on('window-confirm', function (event, message, title) {
  var buttons, cancelId
  if (title == null) {
    title = ''
  }
  buttons = ['OK', 'Cancel']
  cancelId = 1
  return event.returnValue = !dialog.showMessageBox(BrowserWindow.getFocusedWindow(), {
    message: message,
    title: title,
    buttons: buttons,
    cancelId: cancelId
  })
})

ipcMain.on('ATOM_SHELL_GUEST_VIEW_MANAGER_ADD_GUEST', function (event, id, requestId) {
  return event.sender.send('ATOM_SHELL_RESPONSE_' + requestId, addGuest(event.sender, guestInstances[id].guest, id))
})

ipcMain.on('ATOM_SHELL_GUEST_VIEW_MANAGER_CREATE_GUEST', function (event, params, requestId) {
  return event.sender.send('ATOM_SHELL_RESPONSE_' + requestId, addGuest(event.sender, createWebContents(event.sender, params)))
})

ipcMain.on('ATOM_SHELL_GUEST_VIEW_MANAGER_ATTACH_GUEST', function (event, elementInstanceId, guestInstanceId, params) {
  return attachGuest(event.sender, elementInstanceId, guestInstanceId, params)
})

ipcMain.on('ATOM_SHELL_GUEST_VIEW_MANAGER_DESTROY_GUEST', function (event, id) {
  return destroyGuest(event.sender, id)
})

ipcMain.on('ATOM_SHELL_GUEST_VIEW_MANAGER_SET_SIZE', function (event, id, params) {
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

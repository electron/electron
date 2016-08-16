const extensions = process.atomBinding('extension')
const {app, BrowserWindow, ipcMain, webContents} = require('electron')
const path = require('path')

// List of currently active background pages by extensionId
var backgroundPages = {}

// List of events registered for background pages by extensionId
var backgroundPageEvents = {}

// List of current active tabs
var tabs = {}

// List of currently blacklisted tabs
var blacklistedTabs = {}

var getResourceURL = function (extensionId, path) {
  path = String(path)
  if (!path.length || path[0] != '/')
    path = '/' + path
  return 'chrome-extension://' + extensionId + path
}

process.on('enable-extension', function (extensionId) {
  extensions.enable(extensionId)
})

process.on('disable-extension', function (extensionId) {
  extensions.disable(extensionId)
})

process.on('load-extension', function (base_dir, options, cb) {
  if (options instanceof Function) {
    cb = options
    options = {}
  }

  manifest = options.manifest || {}
  manifest_location = options.manifest_location || 'unpacked'
  flags = options.flags || 0

  var install_info = extensions.load(base_dir, manifest, manifest_location, flags)
  cb && cb(install_info)
})

var urlBlacklist = [];

process.once('load-extension-blacklist', (blacklist) => {
  urlBlacklist = blacklist
})

// TODO(bridiver) move these into modules
// background pages
var addBackgroundPage = function (extensionId, backgroundPage) {
  backgroundPages[extensionId] = backgroundPages[extensionId] || []
  backgroundPages[extensionId].push(backgroundPage)
  process.emit('background-page-loaded', extensionId, backgroundPage)
}

var addBackgroundPageEvent = function (extensionId, event) {
  backgroundPageEvents[event] = backgroundPageEvents[event] || []
  if (backgroundPageEvents[event].indexOf(extensionId) === -1) {
    backgroundPageEvents[event].push(extensionId)
  }
}

var sendToBackgroundPages = function (extensionId, session, event) {
  if (!backgroundPageEvents[event] || !session)
    return

  var pages = []
  if (extensionId === 'all') {
    pages = backgroundPageEvents[event].reduce(
      (curr, id) => curr = curr.concat(backgroundPages[id] || []), [])
  } else {
    pages = backgroundPages[extensionId] || []
  }

  var args = [].slice.call(arguments, 2)
  pages.forEach(function (backgroundPage) {
    try {
      // only send to background pages in the same browser context
      if (backgroundPage.session.equal(session)) {
        backgroundPage.send.apply(backgroundPage, args)
      }
    } catch (e) {
      console.error('Could not send to background page: ' + e)
    }
  })
}

var createBackgroundPage = function (webContents) {
  var extensionId = webContents.getURL().match(/chrome-extension:\/\/([^\/]*)/)[1]
  var id = webContents.id
  addBackgroundPage(extensionId, webContents)
  webContents.on('destroyed', function () {
    process.emit('background-page-destroyed', extensionId, id)
    var index = backgroundPages[extensionId].indexOf(webContents)
    if (index > -1) {
      backgroundPages[extensionId].splice(index, 1)
    }
  })
}

// chrome.tabs
var getSessionForTab = function (tabId) {
  let tab = tabs[tabId]
  if (!tab) {
    return
  }

  return tab.session
}

var getWebContentsForTab = function (tabId) {
  let tab = tabs[tabId]
  if (!tab) {
    return
  }

  return tab.webContents
}

var getTabValue = function (tabId) {
  var webContents = getWebContentsForTab(tabId)
  if (webContents) {
    return extensions.tabValue(webContents)
  }
}

var getWindowIdForTab = function (tabId) {
  // cache the windowId so we can still access
  // it when the webContents is destroyed
  if (tabs[tabId] && tabs[tabId].tabInfo) {
    return tabs[tabId].tabInfo.windowId
  } else {
    return -1
  }
}

var getTabsForWindow = function (windowId) {
  return tabsQuery({windowId})
}

var updateWindow = function (windowId, updateInfo) {
  let win = BrowserWindow.fromId(windowId)

  if (win) {
    if (updateInfo.focused) {
      win.focus()
    }

    if (updateInfo.left || updateInfo.top ||
      updateInfo.width || updateInfo.height) {
      let bounds = win.getBounds()
      bounds.x = updateInfo.left || bounds.x
      bounds.y = updateInfo.top || bounds.y
      bounds.width = updateInfo.width || bounds.width
      bounds.height = updateInfo.height || bounds.height
      win.setBounds(bounds)
    }

    switch (updateInfo.state) {
      case 'minimized':
        win.minimize()
        break
      case 'maximized':
        win.maximize()
        break
      case 'fullscreen':
        win.setFullScreen(true)
        break
    }

    return windowInfo(win, false)
  } else {
    console.warn('chrome.windows.update could not find windowId ' + windowId)
    return {}
  }
}

var createGuest = function (opener, url) {
  var payload = {}
  process.emit('ELECTRON_GUEST_VIEW_MANAGER_NEXT_INSTANCE_ID', payload)
  var guestInstanceId = payload.returnValue

  let embedder = opener.hostWebContents || opener
  var options = {
    guestInstanceId,
    embedder,
    session: opener.session,
    isGuest: true,
    delayedLoadUrl: url
  }
  var webPreferences = Object.assign({}, opener.getWebPreferences(), options)
  var guest = webContents.create(webPreferences)

  process.emit('ELECTRON_GUEST_VIEW_MANAGER_REGISTER_GUEST', { sender: opener }, guest, guestInstanceId)
  return guest
}

var pending_tabs = {}

var createTab = function (createProperties, sender, responseId) {
  var opener = null
  var openerTabId = createProperties.openerTabId
  if (openerTabId) {
    opener = getWebContentsForTab(openerTabId)
  }

  var windowId = createProperties.windowId
  if (opener) {
    var win = null
    win = BrowserWindow.fromWebContents(opener)
    if (win && win.id !== windowId) {
      console.warn('The openerTabId is not in the selected window ', createProperties)
      return
    }
    if (!win) {
      console.warn('The openerTabId is not attached to a window ', createProperties)
      return
    }
  } else {
    var tab = tabsQuery({windowId: windowId || -2, active: true})[0]
    if (tab)
      opener = getWebContentsForTab(tab.id)
  }

  if (!opener) {
    console.warn('Could not find an opener for new tab ', createProperties)
    return
  }

  if (opener.isGuest()) {
    var guest = createGuest(opener, createProperties.url)

    let tabInfo = extensions.tabValue(guest)
    tabInfo.status = 'loading'

    pending_tabs[guest.getId()] = {
      tabInfo,
      sender,
      responseId
    }

    var active = createProperties.active !== false
    if (!active)
      active = createProperties.selected !== false

    var disposition = active ? 'foreground-tab' : 'background-tab'

    process.emit('ELECTRON_GUEST_VIEW_MANAGER_TAB_OPEN',
      { sender: opener }, // event
      'about:blank',
      '',
      disposition,
      { webPreferences: guest.getWebPreferences() })

    return tabInfo
  } else {
    // TODO(bridiver) - open a new window
    // ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_OPEN
    console.warn('chrome.tabs.create from a non-guest opener is not supported yet')
  }
}

var removeTabs = function (tabIds) {
  Array(tabIds).forEach((tabId) => {
    var webContents = getWebContentsForTab(tabId)
    if (webContents) {
      if (webContents.isGuest()) {
        webContents.destroy()
      } else {
        var win = BrowserWindow.fromWebContents(webContents)
        win && win.close()
      }
    }
  })
}

var updateTab = function (tabId, updateProperties) {
  var webContents = getWebContentsForTab(tabId)
  if (!webContents)
    return

  if (updateProperties.url)
    webContents.loadURL(updateProperties.url)
  if (updateProperties.active || updateProperties.selected || updateProperties.highlighted)
    webContents.setActive(true)
}

var chromeTabsUpdated = function (tabId, changeInfo) {
  if (blacklistedTabs[tabId]) {
    console.warn('Update denied for blacklisted tab', blacklistedTabs[tabId])
    return
  }
  for (i = 0; i < urlBlacklist.length; i++) {
    if (changeInfo.url && changeInfo.url.match(urlBlacklist[i])) {
      console.warn('Blacklisting', getTabValue(tabId))
      blacklistedTabs[tabId] = getTabValue(tabId)
      chromeTabsRemoved(tabId)
      return
    }
  }

  var tabValue = Object.assign(getTabValue(tabId), changeInfo)
  if (tabValue) {
    sendToBackgroundPages('all', getSessionForTab(tabId), 'chrome-tabs-updated', tabId, changeInfo, tabValue)
  }
}

var chromeTabsRemoved = function (tabId) {
  let windowId = getWindowIdForTab(tabId)
  let session = getSessionForTab(tabId)
  delete tabs[tabId]
  sendToBackgroundPages('all', session, 'chrome-tabs-removed', tabId, {
    windowId,
    isWindowClosing: windowId === -1 ? true : false
  })
}

Array.prototype.diff = function(a) {
    return this.filter(function(i) {return a.indexOf(i) < 0;});
};

var tabsQuery = function (queryInfo, useCurrentWindowId = false) {
  var tabIds = Object.keys(tabs).diff(Object.keys(blacklistedTabs))

  // convert current window identifier to the actual current window id
  if (queryInfo.windowId === -2 || queryInfo.currentWindow === true) {
    delete queryInfo.currentWindow
    var focusedWindow = BrowserWindow.getFocusedWindow()
    if (focusedWindow) {
      queryInfo.windowId = focusedWindow.id
    }
  }

  var queryKeys = Object.keys(queryInfo)
  // get the values for all tabs
  var tabValues = tabIds.reduce((tabs, tabId) => {
    tabs[tabId] = getTabValue(tabId)
    return tabs
  }, {})
  var result = []
  tabIds.forEach((tabId) => {
    // delete tab from the list if any key doesn't match
    if (!queryKeys.map((queryKey) => (tabValues[tabId][queryKey] === queryInfo[queryKey])).includes(false)) {
      result.push(tabValues[tabId])
    }
  })

  return result
}

var initializeTab = function (tabId, webContents) {
  if (tabs[tabId] || blacklistedTabs[tabId]) {
    return
  }
  tabs[tabId] = {}
  tabs[tabId].webContents = webContents
  tabs[tabId].session = webContents.session
  tabs[tabId].tabInfo = extensions.tabValue(webContents)

  let pending = pending_tabs[tabId]
  if (pending) {
    // sender may be gone when we try to call this
    try {
      pending.sender.send('chrome-tabs-create-response-' + pending.responseId, tabs[tabId].tabInfo)
    } catch (e) {}
    delete pending_tabs[tabId]
  }

  sendToBackgroundPages('all', getSessionForTab(tabId), 'chrome-tabs-created', tabs[tabId].tabInfo)
}

app.on('web-contents-created', function (event, webContents) {
  if (extensions.isBackgroundPage(webContents)) {
    createBackgroundPage(webContents)
    return
  }

  var tabId = webContents.getId()
  if (tabId === -1)
    return

  webContents.once('did-attach', function () {
    initializeTab(tabId, webContents)
  })
  webContents.on('did-navigate', function (evt, url) {
    initializeTab(tabId, webContents)
    chromeTabsUpdated(tabId, {url: url, status: 'complete'})
  })
  webContents.on('load-start', function (evt, url, isMainFrame, isErrorPage) {
    initializeTab(tabId, webContents)
    if (isMainFrame) {
      chromeTabsUpdated(tabId, {url: url, status: 'loading'})
    }
  })
  webContents.on('did-finish-load', function () {
    initializeTab(tabId, webContents)
    chromeTabsUpdated(tabId, {status: 'complete'})
  })
  webContents.on('set-active', function (evt, active) {
    if (!tabs[tabId])
      return

    chromeTabsUpdated(tabId, {active})
    var win = webContents.getOwnerBrowserWindow()
    if (win && active)
      sendToBackgroundPages('all', getSessionForTab(tabId), 'chrome-tabs-activated', tabId, {tabId: tabId, windowId: win.id})
  })
  webContents.on('destroyed', function () {
    chromeTabsRemoved(tabId)
  })
  webContents.on('crashed', function () {
    chromeTabsRemoved(tabId)
  })
  webContents.on('close', function () {
    chromeTabsRemoved(tabId)
  })
})

ipcMain.on('chrome-tabs-create', function (evt, responseId, createProperties) {
  createTab(createProperties, evt.sender, responseId)
})

ipcMain.on('chrome-tabs-remove', function (evt, responseId, tabIds) {
  removeTabs(tabIds)
  evt.sender.send('chrome-tabs-remove-' + responseId)
})

ipcMain.on('chrome-tabs-get', function (evt, responseId, tabId) {
  var response = getTabValue(tabId)
  evt.sender.send('chrome-tabs-get-response-' + responseId, response)
})

ipcMain.on('chrome-tabs-query', function (evt, responseId, queryInfo) {
  var response = tabsQuery(queryInfo)
  evt.sender.send('chrome-tabs-query-response-' + responseId, response)
})

ipcMain.on('chrome-tabs-update', function (evt, responseId, tabId, updateProperties) {
  var response = updateTab(tabId, updateProperties)
  evt.sender.send('chrome-tabs-update-response-' + responseId, response)
})

ipcMain.on('chrome-tabs-execute-script', function (evt, extensionId, tabId, details) {
  var tab = getWebContentsForTab(tabId)
  if (tab) {
    tab.executeScriptInTab(extensionId, details.code || '', details)
  }
})

var tabEvents = ['updated', 'created', 'removed', 'activated']
tabEvents.forEach((event_name) => {
  ipcMain.on('register-chrome-tabs-' + event_name, function (evt, extensionId) {
    addBackgroundPageEvent(extensionId, 'chrome-tabs-' + event_name)
  })
})

// chrome.windows

var windowInfo = function (win, populateTabs) {
  var bounds = win.getBounds()
  return {
    focused: false,
    // create psuedo-windows to handle this
    incognito: false, // TODO(bridiver)
    id: win.id,
    focused: win.isFocused(),
    top: bounds.y,
    left: bounds.x,
    width: bounds.width,
    height: bounds.height,
    alwaysOnTop: win.isAlwaysOnTop(),
    tabs: populateTabs ? getTabsForWindow(win.id) : null
  }
}

ipcMain.on('chrome-windows-get-current', function (evt, responseId, getInfo) {
  var response = {
    focused: false,
    incognito: false // TODO(bridiver)
  }
  if(getInfo && getInfo.windowTypes) {
    console.warn('getWindow with windowTypes not supported yet')
  }
  var focusedWindow = BrowserWindow.getFocusedWindow()
  if (focusedWindow) {
    response = windowInfo(focusedWindow, getInfo.populate)
  }

  evt.sender.send('chrome-windows-get-current-response-' + responseId,
    response)
})

ipcMain.on('chrome-windows-get-all', function (evt, responseId, getInfo) {
  if (getInfo && getInfo.windowTypes) {
    console.warn('getWindow with windowTypes not supported yet')
  }
  var response = BrowserWindow.getAllWindows().map((win) => {
    return windowInfo(win, getInfo.populate)
  })

  evt.sender.send('chrome-windows-get-all-response-' + responseId,
    response)
})

ipcMain.on('chrome-windows-update', function (evt, responseId, windowId, updateInfo) {
  var response = updateWindow(windowId, updateInfo)
  evt.sender.send('chrome-windows-update-response-' + responseId, response)
})

// chrome.browserAction
var browserActionPopups = {}

ipcMain.on('register-chrome-browser-action', function (evt, extensionId, actionTitle) {
  addBackgroundPageEvent(extensionId, 'chrome-browser-action-clicked')
  process.emit('chrome-browser-action-registered', extensionId, actionTitle)
})

ipcMain.on('chrome-browser-action-clicked', function (evt, extensionId, tabId, name, props) {
  let popup = browserActionPopups[extensionId]
  if (popup) {
    process.emit('chrome-browser-action-popup', extensionId, tabId, name, props, getResourceURL(extensionId, popup))
  } else {
    let response = getTabValue(tabId)
    if (response) {
      sendToBackgroundPages(extensionId, getSessionForTab(tabId), 'chrome-browser-action-clicked', response)
    }
  }
})

ipcMain.on('chrome-browser-action-set-popup', function (evt, extensionId, details) {
  if (details.popup) {
    browserActionPopups[extensionId] = details.popup
  }
})

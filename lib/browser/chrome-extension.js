const electron = require('electron')
const ipcMain = electron.ipcMain
const app = electron.app
const fs = require('fs')
const path = require('path')
const url = require('url')
const extensions = process.atomBinding('extension')

// Mapping between hostname and file path.
var hostPathMap = {}
var hostPathMapNextKey = 0

var getHostForPath = function (path) {
  var key
  key = 'extension-' + (++hostPathMapNextKey)
  hostPathMap[key] = path
  return key
}

// Cache extensionInfo.
var extensionInfoMap = {}

var getExtensionInfoFromPath = function (srcDirectory) {
  var manifest, page
  manifest = JSON.parse(fs.readFileSync(path.join(srcDirectory, 'manifest.json')))
  if (extensionInfoMap[manifest.name] == null) {
    // We can not use 'file://' directly because all resources in the extension
    // will be treated as relative to the root in Chrome.
    page = url.format({
      protocol: 'chrome-extension',
      slashes: true,
      hostname: getHostForPath(srcDirectory),
      pathname: manifest.devtools_page
    })
    extensionInfoMap[manifest.name] = {
      startPage: page,
      name: manifest.name,
      srcDirectory: srcDirectory,
      exposeExperimentalAPIs: true
    }
    return extensionInfoMap[manifest.name]
  }
}

// The loaded extensions cache and its persistent path.
var loadedExtensions = null
var loadedExtensionsPath = null

app.on('will-quit', function () {
  try {
    loadedExtensions = Object.keys(extensionInfoMap).map(function (key) {
      return extensionInfoMap[key].srcDirectory
    })
    if (loadedExtensions.length > 0) {
      try {
        fs.mkdirSync(path.dirname(loadedExtensionsPath))
      } catch (error) {
        // Ignore error
      }
      fs.writeFileSync(loadedExtensionsPath, JSON.stringify(loadedExtensions))
    } else {
      fs.unlinkSync(loadedExtensionsPath)
    }
  } catch (error) {
    // Ignore error
  }
})

// if defined(ENABLE_EXTENSIONS)
// TODO(bridiver) - turn these into modules

// List of currently active background pages by extensionId
var backgroundPages = {}

// List of events registered for background pages by extensionId
var backgroundPageEvents = {}

// List of current active tabs
var tabs = {}

process.on('load-extension', function (name, base_dir, manifest_location, flags) {
  flags = flags || 0
  manifest_location = manifest_location || 'unpacked'

  var error_message = extensions.load(path.join(base_dir, name), manifest_location, flags)
  if (error_message && error_message !== '') {
    console.error(error_message)
    process.emit('did-extension-load-error', name, error_message)
  } else {
    process.emit('did-extension-load', name)
  }
})

// background pages
var addBackgroundPage = function (extensionId, backgroundPage) {
  backgroundPages[extensionId] = backgroundPages[extensionId] || []
  backgroundPages[extensionId].push(backgroundPage)
}

var addBackgroundPageEvent = function (extensionId, event) {
  backgroundPageEvents[event] = backgroundPageEvents[event] || []
  backgroundPageEvents[event].push(extensionId)
}

var sendToBackgroundPages = function (extensionId, event) {
  if (!backgroundPageEvents[event])
    return

  var pages = []
  if (extensionId === 'all') {
    pages = backgroundPageEvents[event].reduce(
      (curr, id) => curr = curr.concat(backgroundPages[id] || []), [])
  } else {
    pages = backgroundPages[extensionId] || []
  }
  var args = [].slice.call(arguments, 1)
  pages.forEach(function (backgroundPage) {
    try {
      backgroundPage.send.apply(backgroundPage, args)
    } catch (e) {
      console.error('Could not send to background page: ' + e)
    }
  })
}

process.on('background-page-created', function (webContents) {
  var extensionId = webContents.getURL().match(/chrome-extension:\/\/([^\/]*)/)[1]
  addBackgroundPage(extensionId, webContents)
  webContents.on('destroyed', function () {
    var index = backgroundPages[extensionId].indexOf(webContents)
    if (index > -1) {
      backgroundPages[extensionId].splice(index, 1)
    }
  })
})

// chrome.tabs
var createTabInfo = function (tabId) {
  return {
    id: tabId,
    // TODO(bridiver) add incognito info to session api
    incognito: false,
    status: 'complete'
  }
}

var updateTabInfo = function (tabId, changeInfo) {
  var tab = tabs[tabId]
  if (!tab)
    return

  var webContents = tab.webContents
  var win = webContents.getOwnerBrowserWindow()

  var dynamicInfo = {
    audible: !webContents.isAudioMuted(),
    windowId: win ? win.id : -1,
    status: 'complete',
    // TODO(bridiver) these should require the 'tabs' permission
    url: webContents.getURL(),
    title: webContents.getTitle()
  }
  Object.assign(tab.tabInfo, dynamicInfo)
  return Object.assign(tab.tabInfo, changeInfo)
}

var updateTab = function (tabId, updateProperties) {
  var tab = tabs[tabId]
  if (!tab)
    return

  var webContents = tab.webContents

  if (updateProperties.url)
    webContents.loadURL(updateProperties.url)
  // TODO(bridiver) - implement other properties

  return updateTabInfo(tabId, {})
}

var chromeTabsUpdated = function (tabId, changeInfo) {
  var webContents = tabs[tabId]
  if (webContents) {
    sendToBackgroundPages('all', 'chrome-tabs-updated', tabId, changeInfo, updateTabInfo(tabId, changeInfo))
  }
}

var tabsQuery = function (queryInfo) {
  if (queryInfo.windowId === -2 || queryInfo.currentWindow === true) {
    delete queryInfo.currentWindow
    var focusedWindow = electron.BrowserWindow.getFocusedWindow()
    if (focusedWindow)
      queryInfo.windowId = focusedWindow.id
  }
  var keys = Object.keys(queryInfo)
  var tabIds = Object.keys(tabs)

  var matchingTabIds = tabIds.filter((tabId) =>
    !keys.map((key) => (tabs[tabId].tabInfo[key] === queryInfo[key])).includes(false))

  return matchingTabIds.reduce(function (o, k) { o.push(tabs[k].tabInfo); return o }, [])
}

process.on('tab-created', function (tabId, webContents) {
  tabs[tabId] = {
    webContents,
    tabInfo: createTabInfo(tabId)
  }
  webContents.on('destroyed', function () {
    delete tabs[tabId]
  })
  webContents.on('did-navigate', function (evt, url) {
    chromeTabsUpdated(tabId, {url: url, status: 'complete'})
  })
  webContents.on('did-navigate-in-page', function (evt, url) {
    chromeTabsUpdated(tabId, {url: url, status: 'complete'})
  })
  // TODO(bridiver) - add update for window id changing
  webContents.on('set-active', function (evt, active) {
    var tab = tabs[tabId]
    if (tab) {
      var win = webContents.getOwnerBrowserWindow()

      if (!win) {
        return
      }

      if (active === false) {
        updateTabInfo(tabId, {active: false})
      } else {
        var winTabs = tabsQuery({windowId: win.id})
        winTabs.forEach((tabInfo) => {
          if (tabInfo.id === tabId) {
            updateTabInfo(tabId, {active: true})
          } else {
            // only one tab can be active per window
            updateTabInfo(tabInfo.id, {active: false})
          }
        })
      }
    }
  })
})

ipcMain.on('chrome-tabs-query', function (evt, responseId, queryInfo) {
  var response = tabsQuery(queryInfo)
  evt.sender.send('chrome-tabs-query-response-' + responseId, response)
})

ipcMain.on('chrome-tabs-update', function (evt, responseId, tabId, updateProperties) {
  var response = updateTab(tabId, updateProperties)
  evt.sender.send('chrome-tabs-update-response-' + responseId, response)
})

ipcMain.on('register-chrome-tabs-updated', function (evt, extensionId) {
  addBackgroundPageEvent(extensionId, 'chrome-tabs-updated')
})

// chrome.windows
ipcMain.on('chrome-windows-get-current', function (evt, responseId, getInfo) {
  var response = {
    focused: false,
    incognito: false // TODO(bridiver) add incognito info to session api
  }
  if (getInfo) {
    if (getInfo.populate) {
      console.error('getWindow with tabs not supported yet')
    } else if (getInfo.windowTypes) {
      console.error('getWindow with windowTypes not supported yet')
    }
  }
  var focusedWindow = electron.BrowserWindow.getFocusedWindow()
  if (focusedWindow) {
    response.id = focusedWindow.id
    response.focused = focusedWindow.isFocused()
    var bounds = focusedWindow.getBounds()
    response.top = bounds.y
    response.left = bounds.x
    response.width = bounds.width
    response.height = bounds.height
    response.alwaysOnTop = focusedWindow.isAlwaysOnTop()
  }

  evt.sender.send('chrome-windows-get-current-response-' + responseId,
    response)
})

ipcMain.on('chrome-windows-create', function(evt, responseId, createData) { // eslint-disable-line
  // TODO(bridiver) - implement this
  evt.sender.send('chrome-windows-create-response-' + responseId, {})
})

// chrome.browserAction
ipcMain.on('register-chrome-browser-action', function (evt, extensionId, actionTitle) {
  // There can only be one handler per action. The backgroundPage could change if it
  // is an event page, or if the background process crashes and has to be restarted
  var channel = 'chrome-browser-action-clicked-' + extensionId
  ipcMain.removeAllListeners(channel)
  ipcMain.on(channel, function (evt, actionTitle) {  // eslint-disable-line
    var response = tabsQuery({currentWindow: true, active: true})
    if (response[0])
      sendToBackgroundPages(extensionId, 'chrome-browser-action-clicked', response[0])
  })
  addBackgroundPageEvent(extensionId, 'chrome-browser-action-clicked')
  process.emit('chrome-browser-action-registered', channel, actionTitle)
})

ipcMain.on('chrome-context-menu-create', function (evt, extensionId, properties) { // eslint-disable-line
  // TODO(bridiver) - implement this
})
// endif

// We can not use protocol or BrowserWindow until app is ready.
app.once('ready', function () {
  var BrowserWindow, i, init, len, srcDirectory
  BrowserWindow = electron.BrowserWindow

  // Load persisted extensions.
  loadedExtensionsPath = path.join(app.getPath('userData'), 'DevTools Extensions')
  try {
    loadedExtensions = JSON.parse(fs.readFileSync(loadedExtensionsPath))
    if (!Array.isArray(loadedExtensions)) {
      loadedExtensions = []
    }

    // Preheat the extensionInfo cache.
    for (i = 0, len = loadedExtensions.length; i < len; i++) {
      srcDirectory = loadedExtensions[i]
      getExtensionInfoFromPath(srcDirectory)
    }
  } catch (error) {
    // Ignore error
  }

  BrowserWindow.prototype._loadDevToolsExtensions = function (extensionInfoArray) {
    var ref
    return (ref = this.devToolsWebContents) != null ? ref.executeJavaScript('DevToolsAPI.addExtensions(' + (JSON.stringify(extensionInfoArray)) + ');') : void 0
  }
  BrowserWindow.addDevToolsExtension = function (srcDirectory) {
    var extensionInfo, j, len1, ref, window
    extensionInfo = getExtensionInfoFromPath(srcDirectory)
    if (extensionInfo) {
      ref = BrowserWindow.getAllWindows()
      for (j = 0, len1 = ref.length; j < len1; j++) {
        window = ref[j]
        window._loadDevToolsExtensions([extensionInfo])
      }
      return extensionInfo.name
    }
  }
  BrowserWindow.removeDevToolsExtension = function (name) {
    return delete extensionInfoMap[name]
  }

  // Load persisted extensions when devtools is opened.
  init = BrowserWindow.prototype._init
  return BrowserWindow.prototype._init = function () {
    init.call(this)
    return this.on('devtools-opened', function () {
      return this._loadDevToolsExtensions(Object.keys(extensionInfoMap).map(function (key) {
        return extensionInfoMap[key]
      }))
    })
  }
})

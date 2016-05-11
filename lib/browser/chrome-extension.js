const electron = require('electron')
const app = electron.app
const fs = require('fs')
const path = require('path')
const url = require('url')

// Mapping between hostname and file path.
var hostPathMap = {}
var hostPathMapNextKey = 0

var getHostForPath = function (path) {
  var key
  key = 'extension-' + (++hostPathMapNextKey)
  hostPathMap[key] = path
  return key
}

var getPathForHost = function (host) {
  return hostPathMap[host]
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

// We can not use protocol or BrowserWindow until app is ready.
app.once('ready', function () {
  var BrowserWindow, chromeExtensionHandler, i, init, len, protocol, srcDirectory
  protocol = electron.protocol
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

  // The chrome-extension: can map a extension URL request to real file path.
  chromeExtensionHandler = function (request, callback) {
    var directory, parsed
    parsed = url.parse(request.url)
    if (!(parsed.hostname && (parsed.path != null))) {
      return callback()
    }
    if (!/extension-\d+/.test(parsed.hostname)) {
      return callback()
    }
    directory = getPathForHost(parsed.hostname)
    if (directory == null) {
      return callback()
    }
    return callback(path.join(directory, parsed.path))
  }
  protocol.registerFileProtocol('chrome-extension', chromeExtensionHandler, function (error) {
    if (error) {
      return console.error('Unable to register chrome-extension protocol')
    }
  })
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
  BrowserWindow.prototype._init = function () {
    init.call(this)
    return this.webContents.on('devtools-opened', function () {
      return this._loadDevToolsExtensions(Object.keys(extensionInfoMap).map(function (key) {
        return extensionInfoMap[key]
      }))
    })
  }
})

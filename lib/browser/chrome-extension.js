const {app, protocol, BrowserWindow} = require('electron')
const fs = require('fs')
const path = require('path')
const url = require('url')

// Remove this when we have Object.values().
const objectValues = function (object) {
  return Object.keys(object).map(function (key) { return object[key] })
}

// Mapping between hostname and file path.
let hostPathMap = {}
let hostPathMapNextKey = 0

const generateHostForPath = function (path) {
  let key = `extension-${++hostPathMapNextKey}`
  hostPathMap[key] = path
  return key
}

const getPathForHost = function (host) {
  return hostPathMap[host]
}

// Cache extensionInfo.
let extensionInfoMap = {}

const getExtensionInfoFromPath = function (srcDirectory) {
  let manifest = JSON.parse(fs.readFileSync(path.join(srcDirectory, 'manifest.json')))
  if (extensionInfoMap[manifest.name] == null) {
    // We can not use 'file://' directly because all resources in the extension
    // will be treated as relative to the root in Chrome.
    let page = url.format({
      protocol: 'chrome-extension',
      slashes: true,
      hostname: generateHostForPath(srcDirectory),
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

// Load the extensions for the window.
const loadDevToolsExtensions = function (win, extensionInfoArray) {
  if (!win.devToolsWebContents) return
  win.devToolsWebContents.executeJavaScript(`DevToolsAPI.addExtensions(${JSON.stringify(extensionInfoArray)})`)
}

// The persistent path of "DevTools Extensions" preference file.
let loadedExtensionsPath = null

app.on('will-quit', function () {
  try {
    let loadedExtensions = Object.keys(extensionInfoMap).map(function (key) {
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
  // Load persisted extensions.
  loadedExtensionsPath = path.join(app.getPath('userData'), 'DevTools Extensions')
  try {
    let loadedExtensions = JSON.parse(fs.readFileSync(loadedExtensionsPath))
    if (Array.isArray(loadedExtensions)) {
      // Preheat the extensionInfo cache.
      for (let srcDirectory of loadedExtensions) {
        getExtensionInfoFromPath(srcDirectory)
      }
    }
  } catch (error) {
    // Ignore error
  }

  // The chrome-extension: can map a extension URL request to real file path.
  const chromeExtensionHandler = function (request, callback) {
    let parsed = url.parse(request.url)
    if (!parsed.hostname || !parsed.path) return callback()
    if (!/extension-\d+/.test(parsed.hostname)) return callback()

    let directory = getPathForHost(parsed.hostname)
    if (!directory) return callback()

    callback(path.join(directory, parsed.path))
  }
  protocol.registerFileProtocol('chrome-extension', chromeExtensionHandler, function (error) {
    if (error) {
      console.error(`Unable to register chrome-extension protocol: ${error}`)
    }
  })

  BrowserWindow.addDevToolsExtension = function (srcDirectory) {
    let extensionInfo = getExtensionInfoFromPath(srcDirectory)
    if (extensionInfo) {
      for (let win of BrowserWindow.getAllWindows()) {
        loadDevToolsExtensions(win, [extensionInfo])
      }
      return extensionInfo.name
    }
  }
  BrowserWindow.removeDevToolsExtension = function (name) {
    delete extensionInfoMap[name]
  }

  // Load persisted extensions when devtools is opened.
  let init = BrowserWindow.prototype._init
  BrowserWindow.prototype._init = function () {
    init.call(this)
    this.webContents.on('devtools-opened', () => {
      loadDevToolsExtensions(this, objectValues(extensionInfoMap))
    })
  }
})

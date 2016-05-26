const {app, protocol, BrowserWindow} = require('electron')
const fs = require('fs')
const path = require('path')
const url = require('url')

// TODO(zcbenz): Remove this when we have Object.values().
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

// Cache manifests.
let manifestMap = {}

const getManifestFromPath = function (srcDirectory) {
  let manifest = JSON.parse(fs.readFileSync(path.join(srcDirectory, 'manifest.json')))
  if (!manifestMap[manifest.name]) {
    manifestMap[manifest.name] = manifest
    // We can not use 'file://' directly because all resources in the extension
    // will be treated as relative to the root in Chrome.
    manifest.startPage = url.format({
      protocol: 'chrome-extension',
      slashes: true,
      hostname: generateHostForPath(srcDirectory),
      pathname: manifest.devtools_page
    })
    manifest.srcDirectory = srcDirectory
    return manifest
  }
}

// Load the extensions for the window.
const loadDevToolsExtensions = function (win, extensionInfoArray) {
  if (!win.devToolsWebContents) return
  win.devToolsWebContents.executeJavaScript(`DevToolsAPI.addExtensions(${JSON.stringify(extensionInfoArray)})`)
}

// Transfer the |manifest| to a format that can be recognized by the
// |DevToolsAPI.addExtensions|.
const manifestToExtensionInfo = function (manifest) {
  return {
    startPage: manifest.startPage,
    srcDirectory: manifest.srcDirectory,
    name: manifest.name,
    exposeExperimentalAPIs: true
  }
}

// The persistent path of "DevTools Extensions" preference file.
let loadedExtensionsPath = null

app.on('will-quit', function () {
  try {
    let loadedExtensions = objectValues(manifestMap).map(function (manifest) {
      return manifest.srcDirectory
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
      // Preheat the manifest cache.
      for (let srcDirectory of loadedExtensions) {
        getManifestFromPath(srcDirectory)
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
    const manifest = getManifestFromPath(srcDirectory)
    if (manifest) {
      const extensionInfo = manifestToExtensionInfo(manifest)
      for (let win of BrowserWindow.getAllWindows()) {
        loadDevToolsExtensions(win, [extensionInfo])
      }
      return manifest.name
    }
  }
  BrowserWindow.removeDevToolsExtension = function (name) {
    delete manifestMap[name]
  }

  // Load persisted extensions when devtools is opened.
  let init = BrowserWindow.prototype._init
  BrowserWindow.prototype._init = function () {
    init.call(this)
    this.webContents.on('devtools-opened', () => {
      loadDevToolsExtensions(this, objectValues(manifestMap).map(manifestToExtensionInfo))
    })
  }
})

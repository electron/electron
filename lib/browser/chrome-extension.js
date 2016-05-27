const {app, protocol, webContents, BrowserWindow} = require('electron')
const renderProcessPreferences = process.atomBinding('render_process_preferences').forAllBrowserWindow()

const fs = require('fs')
const path = require('path')
const url = require('url')

// TODO(zcbenz): Remove this when we have Object.values().
const objectValues = function (object) {
  return Object.keys(object).map(function (key) { return object[key] })
}

// Mapping between hostname and file path.
const hostPathMap = {}
let hostPathMapNextKey = 0

const generateHostForPath = function (path) {
  const key = `extension-${++hostPathMapNextKey}`
  hostPathMap[key] = path
  return key
}

const getPathForHost = function (host) {
  return hostPathMap[host]
}

// Cache manifests.
const manifestMap = {}

const getManifestFromPath = function (srcDirectory) {
  const manifest = JSON.parse(fs.readFileSync(path.join(srcDirectory, 'manifest.json')))
  if (!manifestMap[manifest.name]) {
    const hostname = generateHostForPath(srcDirectory)
    manifestMap[manifest.name] = manifest
    Object.assign(manifest, {
      srcDirectory: srcDirectory,
      hostname: hostname,
      // We can not use 'file://' directly because all resources in the extension
      // will be treated as relative to the root in Chrome.
      startPage: url.format({
        protocol: 'chrome-extension',
        slashes: true,
        hostname: hostname,
        pathname: manifest.devtools_page
      })
    })
    return manifest
  }
}

// Manage the background pages.
const backgroundPages = {}

const startBackgroundPages = function (manifest) {
  if (backgroundPages[manifest.hostname] || !manifest.background) return

  const scripts = manifest.background.scripts.map((name) => {
    return `<script src="${name}"></script>`
  }).join('')
  const html = new Buffer(`<html><body>${scripts}</body></html>`)

  const contents = webContents.create({})
  backgroundPages[manifest.hostname] = { html: html, webContents: contents }
  contents.loadURL(url.format({
    protocol: 'chrome-extension',
    slashes: true,
    hostname: manifest.hostname,
    pathname: '_generated_background_page.html'
  }))
}

const removeBackgroundPages = function (manifest) {
  if (!backgroundPages[manifest.hostname]) return

  backgroundPages[manifest.hostname].webContents.destroy()
  delete backgroundPages[manifest.hostname]
}

// Transfer the content scripts to renderer.
const contentScripts = {}

const injectContentScripts = function (manifest) {
  if (contentScripts[manifest.name] || !manifest.content_scripts) return

  const readArrayOfFiles = function (relativePath) {
    return String(fs.readFileSync(path.join(manifest.srcDirectory, relativePath)))
  }

  const contentScriptToEntry = function (script) {
    return {
      matches: script.matches,
      js: script.js.map(readArrayOfFiles),
      runAt: script.run_at || 'document_idle'
    }
  }

  try {
    const entry = {
      contentScripts: manifest.content_scripts.map(contentScriptToEntry)
    }
    contentScripts[manifest.name] = renderProcessPreferences.addEntry(entry)
  } catch (e) {
    console.error('Failed to read content scripts', e)
  }
}

const removeContentScripts = function (manifest) {
  if (!contentScripts[manifest.name]) return

  renderProcessPreferences.removeEntry(contentScripts[manifest.name])
  delete contentScripts[manifest.name]
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

// Load the extensions for the window.
const loadDevToolsExtensions = function (win, manifests) {
  if (!win.devToolsWebContents) return

  for (const manifest of manifests) {
    startBackgroundPages(manifest)
    injectContentScripts(manifest)
  }
  const extensionInfoArray = manifests.map(manifestToExtensionInfo)
  win.devToolsWebContents.executeJavaScript(`DevToolsAPI.addExtensions(${JSON.stringify(extensionInfoArray)})`)
}

// The persistent path of "DevTools Extensions" preference file.
let loadedExtensionsPath = null

app.on('will-quit', function () {
  try {
    const loadedExtensions = objectValues(manifestMap).map(function (manifest) {
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
    const loadedExtensions = JSON.parse(fs.readFileSync(loadedExtensionsPath))
    if (Array.isArray(loadedExtensions)) {
      // Preheat the manifest cache.
      for (const srcDirectory of loadedExtensions) {
        getManifestFromPath(srcDirectory)
      }
    }
  } catch (error) {
    // Ignore error
  }

  // The chrome-extension: can map a extension URL request to real file path.
  const chromeExtensionHandler = function (request, callback) {
    const parsed = url.parse(request.url)
    if (!parsed.hostname || !parsed.path) return callback()
    if (!/extension-\d+/.test(parsed.hostname)) return callback()

    const directory = getPathForHost(parsed.hostname)
    if (!directory) return callback()

    if (parsed.path === '/_generated_background_page.html' &&
        backgroundPages[parsed.hostname]) {
      return callback({
        mimeType: 'text/html',
        data: backgroundPages[parsed.hostname].html
      })
    }

    fs.readFile(path.join(directory, parsed.path), function (err, content) {
      if (err) {
        return callback(-6)  // FILE_NOT_FOUND
      } else {
        return callback({mimeType: 'text/html', data: content})
      }
    })
  }
  protocol.registerBufferProtocol('chrome-extension', chromeExtensionHandler, function (error) {
    if (error) {
      console.error(`Unable to register chrome-extension protocol: ${error}`)
    }
  })

  BrowserWindow.addDevToolsExtension = function (srcDirectory) {
    const manifest = getManifestFromPath(srcDirectory)
    if (manifest) {
      for (const win of BrowserWindow.getAllWindows()) {
        loadDevToolsExtensions(win, [manifest])
      }
      return manifest.name
    }
  }
  BrowserWindow.removeDevToolsExtension = function (name) {
    const manifest = manifestMap[name]
    if (!manifest) return

    removeBackgroundPages(manifest)
    removeContentScripts(manifest)
    delete manifestMap[name]
  }

  // Load persisted extensions when devtools is opened.
  const init = BrowserWindow.prototype._init
  BrowserWindow.prototype._init = function () {
    init.call(this)
    this.webContents.on('devtools-opened', () => {
      loadDevToolsExtensions(this, objectValues(manifestMap))
    })
  }
})

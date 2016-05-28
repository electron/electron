const {app, ipcMain, protocol, webContents, BrowserWindow} = require('electron')
const renderProcessPreferences = process.atomBinding('render_process_preferences').forAllBrowserWindow()

const fs = require('fs')
const path = require('path')
const url = require('url')

// TODO(zcbenz): Remove this when we have Object.values().
const objectValues = function (object) {
  return Object.keys(object).map(function (key) { return object[key] })
}

// Mapping between extensionId(hostname) and manifest.
const manifestMap = {}  // extensionId => manifest
const manifestNameMap = {}  // name => manifest

let nextExtensionId = 0

// Create or get manifest object from |srcDirectory|.
const getManifestFromPath = function (srcDirectory) {
  const manifest = JSON.parse(fs.readFileSync(path.join(srcDirectory, 'manifest.json')))
  if (!manifestNameMap[manifest.name]) {
    const extensionId = `extension-${++nextExtensionId}`
    manifestMap[extensionId] = manifestNameMap[manifest.name] = manifest
    Object.assign(manifest, {
      srcDirectory: srcDirectory,
      extensionId: extensionId,
      // We can not use 'file://' directly because all resources in the extension
      // will be treated as relative to the root in Chrome.
      startPage: url.format({
        protocol: 'chrome-extension',
        slashes: true,
        hostname: extensionId,
        pathname: manifest.devtools_page
      })
    })
    return manifest
  }
}

// Manage the background pages.
const backgroundPages = {}

const startBackgroundPages = function (manifest) {
  if (backgroundPages[manifest.extensionId] || !manifest.background) return

  const scripts = manifest.background.scripts.map((name) => {
    return `<script src="${name}"></script>`
  }).join('')
  const html = new Buffer(`<html><body>${scripts}</body></html>`)

  const contents = webContents.create({})
  backgroundPages[manifest.extensionId] = { html: html, webContents: contents }
  contents.loadURL(url.format({
    protocol: 'chrome-extension',
    slashes: true,
    hostname: manifest.extensionId,
    pathname: '_generated_background_page.html'
  }))
}

const removeBackgroundPages = function (manifest) {
  if (!backgroundPages[manifest.extensionId]) return

  backgroundPages[manifest.extensionId].webContents.destroy()
  delete backgroundPages[manifest.extensionId]
}

// Handle the chrome.* API messages.
let nextId = 0

ipcMain.on('CHROME_RUNTIME_CONNECT', function (event, extensionId, connectInfo) {
  const page = backgroundPages[extensionId]
  if (!page) {
    console.error(`Connect to unkown extension ${extensionId}`)
    return
  }

  const portId = ++nextId
  event.returnValue = {webContentsId: page.webContents.id, portId: portId}

  event.sender.once('render-view-deleted', () => {
    page.webContents.sendToAll(`CHROME_PORT_ONDISCONNECT_${portId}`)
  })
  page.webContents.sendToAll('CHROME_RUNTIME_ONCONNECT', event.sender.id, portId, extensionId, connectInfo)
})

ipcMain.on('CHROME_PORT_DISCONNECT', function (event, webContentsId, portId) {
  const contents = webContents.fromId(webContentsId)
  if (!contents) {
    console.error(`Disconnet to unkown webContentsId ${webContentsId}`)
    return
  }

  contents.sendToAll(`CHROME_PORT_ONDISCONNECT_${portId}`)
})

ipcMain.on('CHROME_PORT_POSTMESSAGE', function (event, webContentsId, portId, message) {
  const contents = webContents.fromId(webContentsId)
  if (!contents) {
    console.error(`Sending message to unkown webContentsId ${webContentsId}`)
    return
  }

  contents.sendToAll(`CHROME_PORT_ONMESSAGE_${portId}`, message)
})

ipcMain.on('CHROME_TABS_EXECUTESCRIPT', function (event, requestId, webContentsId, extensionId, details) {
  const contents = webContents.fromId(webContentsId)
  if (!contents) {
    console.error(`Sending message to unkown webContentsId ${webContentsId}`)
    return
  }

  let code, url
  if (details.file) {
    const manifest = manifestMap[extensionId]
    code = String(fs.readFileSync(path.join(manifest.srcDirectory, details.file)))
    url = `chrome-extension://${extensionId}${details.file}`
  } else {
    code = details.code
    url = `chrome-extension://${extensionId}/${String(Math.random()).substr(2, 8)}.js`
  }

  contents.send('CHROME_TABS_EXECUTESCRIPT', requestId, event.sender.id, extensionId, url, code)
})

ipcMain.on('CHROME_TABS_EXECUTESCRIPT_RESULT', (event, requestId, webContentsId, result) => {
  const contents = webContents.fromId(webContentsId)
  if (!contents) {
    console.error(`Sending message to unkown webContentsId ${webContentsId}`)
    return
  }

  contents.send(`CHROME_TABS_EXECUTESCRIPT_RESULT_${requestId}`, result)
})

// Transfer the content scripts to renderer.
const contentScripts = {}

const injectContentScripts = function (manifest) {
  if (contentScripts[manifest.name] || !manifest.content_scripts) return

  const readArrayOfFiles = function (relativePath) {
    return {
      url: `chrome-extension://${manifest.extensionId}/${relativePath}`,
      code: String(fs.readFileSync(path.join(manifest.srcDirectory, relativePath)))
    }
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
      extensionId: manifest.extensionId,
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
const loadExtension = function (manifest) {
  startBackgroundPages(manifest)
  injectContentScripts(manifest)
}

const loadDevToolsExtensions = function (win, manifests) {
  if (!win.devToolsWebContents) return

  manifests.forEach(loadExtension)

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
  // The chrome-extension: can map a extension URL request to real file path.
  const chromeExtensionHandler = function (request, callback) {
    const parsed = url.parse(request.url)
    if (!parsed.hostname || !parsed.path) return callback()
    if (!/extension-\d+/.test(parsed.hostname)) return callback()

    const manifest = manifestMap[parsed.hostname]
    if (!manifest) return callback()

    if (parsed.path === '/_generated_background_page.html' &&
        backgroundPages[parsed.hostname]) {
      return callback({
        mimeType: 'text/html',
        data: backgroundPages[parsed.hostname].html
      })
    }

    fs.readFile(path.join(manifest.srcDirectory, parsed.path), function (err, content) {
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

  // Load persisted extensions.
  loadedExtensionsPath = path.join(app.getPath('userData'), 'DevTools Extensions')
  try {
    const loadedExtensions = JSON.parse(fs.readFileSync(loadedExtensionsPath))
    if (Array.isArray(loadedExtensions)) {
      for (const srcDirectory of loadedExtensions) {
        // Start background pages and set content scripts.
        const manifest = getManifestFromPath(srcDirectory)
        loadExtension(manifest)
      }
    }
  } catch (error) {
    // Ignore error
  }

  // The public API to add/remove extensions.
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
    const manifest = manifestNameMap[name]
    if (!manifest) return

    removeBackgroundPages(manifest)
    removeContentScripts(manifest)
    delete manifestMap[manifest.extensionId]
    delete manifestNameMap[name]
  }

  // Load extensions automatically when devtools is opened.
  const init = BrowserWindow.prototype._init
  BrowserWindow.prototype._init = function () {
    init.call(this)
    this.webContents.on('devtools-opened', () => {
      loadDevToolsExtensions(this, objectValues(manifestMap))
    })
  }
})

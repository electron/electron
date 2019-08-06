'use strict'

const { app, webContents, BrowserWindow } = require('electron')
const { getAllWebContents } = process.atomBinding('web_contents')
const renderProcessPreferences = process.atomBinding('render_process_preferences').forAllWebContents()
const ipcMain = require('@electron/internal/browser/ipc-main-internal')

const { Buffer } = require('buffer')
const fs = require('fs')
const path = require('path')
const url = require('url')

// Mapping between extensionId(hostname) and manifest.
const manifestMap = {} // extensionId => manifest
const manifestNameMap = {} // name => manifest
const devToolsExtensionNames = new Set()

const generateExtensionIdFromName = function (name) {
  return name.replace(/[\W_]+/g, '-').toLowerCase()
}

const isWindowOrWebView = function (webContents) {
  const type = webContents.getType()
  return type === 'window' || type === 'webview'
}

// Create or get manifest object from |srcDirectory|.
const getManifestFromPath = function (srcDirectory) {
  let manifest
  let manifestContent

  try {
    manifestContent = fs.readFileSync(path.join(srcDirectory, 'manifest.json'))
  } catch (readError) {
    console.warn(`Reading ${path.join(srcDirectory, 'manifest.json')} failed.`)
    console.warn(readError.stack || readError)
    throw readError
  }

  try {
    manifest = JSON.parse(manifestContent)
  } catch (parseError) {
    console.warn(`Parsing ${path.join(srcDirectory, 'manifest.json')} failed.`)
    console.warn(parseError.stack || parseError)
    throw parseError
  }

  if (!manifestNameMap[manifest.name]) {
    const extensionId = generateExtensionIdFromName(manifest.name)
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
  } else if (manifest && manifest.name) {
    console.warn(`Attempted to load extension "${manifest.name}" that has already been loaded.`)
    return manifest
  }
}

// Manage the background pages.
const backgroundPages = {}

const startBackgroundPages = function (manifest) {
  if (backgroundPages[manifest.extensionId] || !manifest.background) return

  let html
  let name
  if (manifest.background.page) {
    name = manifest.background.page
    html = fs.readFileSync(path.join(manifest.srcDirectory, manifest.background.page))
  } else {
    name = '_generated_background_page.html'
    const scripts = manifest.background.scripts.map((name) => {
      return `<script src="${name}"></script>`
    }).join('')
    html = Buffer.from(`<html><body>${scripts}</body></html>`)
  }

  const contents = webContents.create({
    partition: 'persist:__chrome_extension',
    isBackgroundPage: true,
    commandLineSwitches: ['--background-page']
  })
  backgroundPages[manifest.extensionId] = { html: html, webContents: contents, name: name }
  contents.loadURL(url.format({
    protocol: 'chrome-extension',
    slashes: true,
    hostname: manifest.extensionId,
    pathname: name
  }))
}

const removeBackgroundPages = function (manifest) {
  if (!backgroundPages[manifest.extensionId]) return

  backgroundPages[manifest.extensionId].webContents.destroy()
  delete backgroundPages[manifest.extensionId]
}

const sendToBackgroundPages = function (...args) {
  for (const page of Object.values(backgroundPages)) {
    if (!page.webContents.isDestroyed()) {
      page.webContents._sendInternalToAll(...args)
    }
  }
}

// Dispatch web contents events to Chrome APIs
const hookWebContentsEvents = function (webContents) {
  const tabId = webContents.id

  sendToBackgroundPages('CHROME_TABS_ONCREATED')

  webContents.on('will-navigate', (event, url) => {
    sendToBackgroundPages('CHROME_WEBNAVIGATION_ONBEFORENAVIGATE', {
      frameId: 0,
      parentFrameId: -1,
      processId: webContents.getProcessId(),
      tabId: tabId,
      timeStamp: Date.now(),
      url: url
    })
  })

  webContents.on('did-navigate', (event, url) => {
    sendToBackgroundPages('CHROME_WEBNAVIGATION_ONCOMPLETED', {
      frameId: 0,
      parentFrameId: -1,
      processId: webContents.getProcessId(),
      tabId: tabId,
      timeStamp: Date.now(),
      url: url
    })
  })

  webContents.once('destroyed', () => {
    sendToBackgroundPages('CHROME_TABS_ONREMOVED', tabId)
  })
}

// Handle the chrome.* API messages.
let nextId = 0

ipcMain.on('CHROME_RUNTIME_CONNECT', function (event, extensionId, connectInfo) {
  const page = backgroundPages[extensionId]
  if (!page || page.webContents.isDestroyed()) {
    throw new Error(`Connect to unknown extension ${extensionId}`)
  }

  const portId = ++nextId
  event.returnValue = { tabId: page.webContents.id, portId: portId }

  event.sender.once('render-view-deleted', () => {
    if (page.webContents.isDestroyed()) return
    page.webContents._sendInternalToAll(`CHROME_PORT_DISCONNECT_${portId}`)
  })
  page.webContents._sendInternalToAll(`CHROME_RUNTIME_ONCONNECT_${extensionId}`, event.sender.id, portId, connectInfo)
})

ipcMain.on('CHROME_I18N_MANIFEST', function (event, extensionId) {
  event.returnValue = manifestMap[extensionId]
})

let resultID = 1
ipcMain.on('CHROME_RUNTIME_SENDMESSAGE', function (event, extensionId, message, originResultID) {
  const page = backgroundPages[extensionId]
  if (!page || page.webContents.isDestroyed()) {
    throw new Error(`Connect to unknown extension ${extensionId}`)
  }

  page.webContents._sendInternalToAll(`CHROME_RUNTIME_ONMESSAGE_${extensionId}`, event.sender.id, message, resultID)
  ipcMain.once(`CHROME_RUNTIME_ONMESSAGE_RESULT_${resultID}`, (event, result) => {
    event._replyInternal(`CHROME_RUNTIME_SENDMESSAGE_RESULT_${originResultID}`, result)
  })
  resultID++
})

ipcMain.on('CHROME_TABS_SEND_MESSAGE', function (event, tabId, extensionId, isBackgroundPage, message, originResultID) {
  const contents = webContents.fromId(tabId)
  if (!contents) {
    console.error(`Sending message to unknown tab ${tabId}`)
    return
  }

  const senderTabId = isBackgroundPage ? null : event.sender.id

  contents._sendInternalToAll(`CHROME_RUNTIME_ONMESSAGE_${extensionId}`, senderTabId, message, resultID)
  ipcMain.once(`CHROME_RUNTIME_ONMESSAGE_RESULT_${resultID}`, (event, result) => {
    event._replyInternal(`CHROME_TABS_SEND_MESSAGE_RESULT_${originResultID}`, result)
  })
  resultID++
})

const isChromeExtension = function (pageURL) {
  const { protocol } = url.parse(pageURL)
  return protocol === 'chrome-extension:'
}

ipcMain.on('CHROME_TABS_EXECUTESCRIPT', function (event, requestId, tabId, extensionId, details) {
  const pageURL = event.sender._getURL()
  if (!isChromeExtension(pageURL)) {
    console.error(`Blocked ${pageURL} from calling chrome.tabs.executeScript()`)
    return
  }

  const contents = webContents.fromId(tabId)
  if (!contents) {
    console.error(`Sending message to unknown tab ${tabId}`)
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

  contents._sendInternal('CHROME_TABS_EXECUTESCRIPT', event.sender.id, requestId, extensionId, url, code)
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
      js: script.js ? script.js.map(readArrayOfFiles) : [],
      css: script.css ? script.css.map(readArrayOfFiles) : [],
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
  extensionInfoArray.forEach((extension) => {
    win.devToolsWebContents._grantOriginAccess(extension.startPage)
  })
  win.devToolsWebContents.executeJavaScript(`InspectorFrontendAPI.addExtensions(${JSON.stringify(extensionInfoArray)})`)
}

app.on('web-contents-created', function (event, webContents) {
  if (!isWindowOrWebView(webContents)) return

  hookWebContentsEvents(webContents)
  webContents.on('devtools-opened', function () {
    loadDevToolsExtensions(webContents, Object.values(manifestMap))
  })
})

// The chrome-extension: can map a extension URL request to real file path.
const chromeExtensionHandler = function (request, callback) {
  const parsed = url.parse(request.url)
  if (!parsed.hostname || !parsed.path) return callback()

  const manifest = manifestMap[parsed.hostname]
  if (!manifest) return callback()

  const page = backgroundPages[parsed.hostname]
  if (page && parsed.path === `/${page.name}`) {
    // Disabled due to false positive in StandardJS
    // eslint-disable-next-line standard/no-callback-literal
    return callback({
      mimeType: 'text/html',
      data: page.html
    })
  }

  fs.readFile(path.join(manifest.srcDirectory, parsed.path), function (err, content) {
    if (err) {
      // Disabled due to false positive in StandardJS
      // eslint-disable-next-line standard/no-callback-literal
      return callback(-6) // FILE_NOT_FOUND
    } else {
      return callback(content)
    }
  })
}

app.on('session-created', function (ses) {
  ses.protocol.registerBufferProtocol('chrome-extension', chromeExtensionHandler, function (error) {
    if (error) {
      console.error(`Unable to register chrome-extension protocol: ${error}`)
    }
  })
})

// The persistent path of "DevTools Extensions" preference file.
let loadedDevToolsExtensionsPath = null

app.on('will-quit', function () {
  try {
    const loadedDevToolsExtensions = Array.from(devToolsExtensionNames)
      .map(name => manifestNameMap[name].srcDirectory)
    if (loadedDevToolsExtensions.length > 0) {
      try {
        fs.mkdirSync(path.dirname(loadedDevToolsExtensionsPath))
      } catch (error) {
        // Ignore error
      }
      fs.writeFileSync(loadedDevToolsExtensionsPath, JSON.stringify(loadedDevToolsExtensions))
    } else {
      fs.unlinkSync(loadedDevToolsExtensionsPath)
    }
  } catch (error) {
    // Ignore error
  }
})

// We can not use protocol or BrowserWindow until app is ready.
app.once('ready', function () {
  // The public API to add/remove extensions.
  BrowserWindow.addExtension = function (srcDirectory) {
    const manifest = getManifestFromPath(srcDirectory)
    if (manifest) {
      loadExtension(manifest)
      for (const webContents of getAllWebContents()) {
        if (isWindowOrWebView(webContents)) {
          loadDevToolsExtensions(webContents, [manifest])
        }
      }
      return manifest.name
    }
  }

  BrowserWindow.removeExtension = function (name) {
    const manifest = manifestNameMap[name]
    if (!manifest) return

    removeBackgroundPages(manifest)
    removeContentScripts(manifest)
    delete manifestMap[manifest.extensionId]
    delete manifestNameMap[name]
  }

  BrowserWindow.getExtensions = function () {
    const extensions = {}
    Object.keys(manifestNameMap).forEach(function (name) {
      const manifest = manifestNameMap[name]
      extensions[name] = { name: manifest.name, version: manifest.version }
    })
    return extensions
  }

  BrowserWindow.addDevToolsExtension = function (srcDirectory) {
    const manifestName = BrowserWindow.addExtension(srcDirectory)
    if (manifestName) {
      devToolsExtensionNames.add(manifestName)
    }
    return manifestName
  }

  BrowserWindow.removeDevToolsExtension = function (name) {
    BrowserWindow.removeExtension(name)
    devToolsExtensionNames.delete(name)
  }

  BrowserWindow.getDevToolsExtensions = function () {
    const extensions = BrowserWindow.getExtensions()
    const devExtensions = {}
    Array.from(devToolsExtensionNames).forEach(function (name) {
      if (!extensions[name]) return
      devExtensions[name] = extensions[name]
    })
    return devExtensions
  }

  // Load persisted extensions.
  loadedDevToolsExtensionsPath = path.join(app.getPath('userData'), 'DevTools Extensions')
  try {
    const loadedDevToolsExtensions = JSON.parse(fs.readFileSync(loadedDevToolsExtensionsPath))
    if (Array.isArray(loadedDevToolsExtensions)) {
      for (const srcDirectory of loadedDevToolsExtensions) {
        // Start background pages and set content scripts.
        BrowserWindow.addDevToolsExtension(srcDirectory)
      }
    }
  } catch (error) {
    if (process.env.ELECTRON_ENABLE_LOGGING && error.code !== 'ENOENT') {
      console.error('Failed to load browser extensions from directory:', loadedDevToolsExtensionsPath)
      console.error(error)
    }
  }
})

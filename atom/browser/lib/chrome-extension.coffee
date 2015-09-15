app  = require 'app'
fs   = require 'fs'
path = require 'path'
url  = require 'url'

# Mapping between hostname and file path.
hostPathMap = {}
hostPathMapNextKey = 0

getHostForPath = (path) ->
  key = "extension-#{++hostPathMapNextKey}"
  hostPathMap[key] = path
  key

getPathForHost = (host) ->
  hostPathMap[host]

# Cache extensionInfo.
extensionInfoMap = {}

getExtensionInfoFromPath = (srcDirectory) ->
  manifest = JSON.parse fs.readFileSync(path.join(srcDirectory, 'manifest.json'))
  unless extensionInfoMap[manifest.name]?
    # We can not use 'file://' directly because all resources in the extension
    # will be treated as relative to the root in Chrome.
    page = url.format
      protocol: 'chrome-extension'
      slashes: true
      hostname: getHostForPath srcDirectory
      pathname: manifest.devtools_page
    extensionInfoMap[manifest.name] =
      startPage: page
      name: manifest.name
      srcDirectory: srcDirectory
      exposeExperimentalAPIs: true
    extensionInfoMap[manifest.name]

# The loaded extensions cache and its persistent path.
loadedExtensions = null
loadedExtensionsPath = null

# Persistent loaded extensions.
app.on 'will-quit', ->
  try
    loadedExtensions = Object.keys(extensionInfoMap).map (key) -> extensionInfoMap[key].srcDirectory
    try
      fs.mkdirSync path.dirname(loadedExtensionsPath)
    catch e
    fs.writeFileSync loadedExtensionsPath, JSON.stringify(loadedExtensions)
  catch e

# We can not use protocol or BrowserWindow until app is ready.
app.once 'ready', ->
  protocol = require 'protocol'
  BrowserWindow = require 'browser-window'

  # Load persistented extensions.
  loadedExtensionsPath = path.join app.getDataPath(), 'DevTools Extensions'

  try
    loadedExtensions = JSON.parse fs.readFileSync(loadedExtensionsPath)
    loadedExtensions = [] unless Array.isArray loadedExtensions
    # Preheat the extensionInfo cache.
    getExtensionInfoFromPath srcDirectory for srcDirectory in loadedExtensions
  catch e

  # The chrome-extension: can map a extension URL request to real file path.
  chromeExtensionHandler = (request, callback) ->
    parsed = url.parse request.url
    return callback() unless parsed.hostname and parsed.path?
    return callback() unless /extension-\d+/.test parsed.hostname

    directory = getPathForHost parsed.hostname
    return callback() unless directory?
    callback path.join(directory, parsed.path)
  protocol.registerFileProtocol 'chrome-extension', chromeExtensionHandler, (error) ->
    console.error 'Unable to register chrome-extension protocol' if error

  BrowserWindow::_loadDevToolsExtensions = (extensionInfoArray) ->
    @devToolsWebContents?.executeJavaScript "DevToolsAPI.addExtensions(#{JSON.stringify(extensionInfoArray)});"

  BrowserWindow.addDevToolsExtension = (srcDirectory) ->
    extensionInfo = getExtensionInfoFromPath srcDirectory
    if extensionInfo
      window._loadDevToolsExtensions [extensionInfo] for window in BrowserWindow.getAllWindows()
      extensionInfo.name

  BrowserWindow.removeDevToolsExtension = (name) ->
    delete extensionInfoMap[name]

  # Load persistented extensions when devtools is opened.
  init = BrowserWindow::_init
  BrowserWindow::_init = ->
    init.call this
    @on 'devtools-opened', ->
      @_loadDevToolsExtensions Object.keys(extensionInfoMap).map (key) -> extensionInfoMap[key]

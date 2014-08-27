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

app.once 'ready', ->
  protocol = require 'protocol'
  BrowserWindow = require 'browser-window'

  protocol.registerProtocol 'chrome-extension', (request) ->
    parsed = url.parse request.url
    return unless parsed.hostname and parsed.path?

    return unless /extension-\d+/.test parsed.hostname

    directory = getPathForHost parsed.hostname
    return new protocol.RequestFileJob(path.join(directory, parsed.path))

  BrowserWindow::loadDevToolsExtension = (srcDirectory) ->
    manifest = JSON.parse fs.readFileSync(path.join(srcDirectory, 'manifest.json'))

    # We can not use 'file://' directly because all resources in the extension
    # will be treated as relative to the root in Chrome.
    page = url.format
      protocol: 'chrome-extension'
      slashes: true
      hostname: getHostForPath srcDirectory
      pathname: manifest.devtools_page

    extensionInfo = startPage: page, name: manifest.name
    @devToolsWebContents?.executeJavaScript "WebInspector.addExtensions([#{JSON.stringify(extensionInfo)}]);"

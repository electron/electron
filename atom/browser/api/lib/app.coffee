EventEmitter = require('events').EventEmitter

bindings = process.atomBinding 'app'
sessionBindings = process.atomBinding 'session'
downloadItemBindings = process.atomBinding 'download_item'

app = bindings.app
app.__proto__ = EventEmitter.prototype

wrapSession = (session) ->
  # session is an Event Emitter.
  session.__proto__ = EventEmitter.prototype

wrapDownloadItem = (downloadItem) ->
  # downloadItem is an Event Emitter.
  downloadItem.__proto__ = EventEmitter.prototype
  # Be compatible with old APIs.
  downloadItem.url = downloadItem.getUrl()
  downloadItem.filename = downloadItem.getFilename()
  downloadItem.mimeType = downloadItem.getMimeType()
  downloadItem.hasUserGesture = downloadItem.hasUserGesture()

app.setApplicationMenu = (menu) ->
  require('menu').setApplicationMenu menu

app.getApplicationMenu = ->
  require('menu').getApplicationMenu()

app.commandLine =
  appendSwitch: bindings.appendSwitch,
  appendArgument: bindings.appendArgument

if process.platform is 'darwin'
  app.dock =
    bounce: (type='informational') -> bindings.dockBounce type
    cancelBounce: bindings.dockCancelBounce
    setBadge: bindings.dockSetBadgeText
    getBadge: bindings.dockGetBadgeText
    hide: bindings.dockHide
    show: bindings.dockShow
    setMenu: bindings.dockSetMenu

appPath = null
app.setAppPath = (path) ->
  appPath = path

app.getAppPath = ->
  appPath

# Be compatible with old API.
app.once 'ready', -> @emit 'finish-launching'
app.terminate = app.quit
app.getHomeDir = -> @getPath 'home'
app.getDataPath = -> @getPath 'userData'
app.setDataPath = (path) -> @setPath 'userData', path
app.resolveProxy = -> @defaultSession.resolveProxy.apply @defaultSession, arguments
app.on 'activate', (event, hasVisibleWindows) -> @emit 'activate-with-no-open-windows' if not hasVisibleWindows

# Wrappers for native classes.
sessionBindings._setWrapSession wrapSession
downloadItemBindings._setWrapDownloadItem wrapDownloadItem

# Only one App object pemitted.
module.exports = app

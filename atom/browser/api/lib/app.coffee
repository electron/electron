EventEmitter = require('events').EventEmitter

bindings = process.atomBinding 'app'
sessionBindings = process.atomBinding 'session'
downloadItemBindings = process.atomBinding 'download_item'

app = bindings.app
app.__proto__ = EventEmitter.prototype

wrapSession = (session) ->
  # session is an Event Emitter.
  session.__proto__ = EventEmitter.prototype

wrapDownloadItem = (download_item) ->
  # download_item is an Event Emitter.
  download_item.__proto__ = EventEmitter.prototype
  # Be compatible with old APIs.
  download_item.url = download_item.getUrl()
  download_item.filename = download_item.getFilename()
  download_item.mimeType = download_item.getMimeType()
  download_item.hasUserGesture = download_item.hasUserGesture()

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
app.exit = process.exit
app.getHomeDir = -> @getPath 'home'
app.getDataPath = -> @getPath 'userData'
app.setDataPath = (path) -> @setPath 'userData', path
app.resolveProxy = -> @defaultSession.resolveProxy.apply @defaultSession, arguments
app.on 'activate', (event, hasVisibleWindows) -> @emit 'activate-with-no-open-windows' if not hasVisibleWindows

# Session wrapper.
sessionBindings._setWrapSession wrapSession
process.once 'exit', sessionBindings._clearWrapSession

downloadItemBindings._setWrapDownloadItem wrapDownloadItem
process.once 'exit', downloadItemBindings._clearWrapDownloadItem

# Only one App object pemitted.
module.exports = app

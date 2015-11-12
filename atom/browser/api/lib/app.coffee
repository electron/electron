deprecate = require 'deprecate'
EventEmitter = require('events').EventEmitter

bindings = process.atomBinding 'app'
sessionBindings = process.atomBinding 'session'
downloadItemBindings = process.atomBinding 'download_item'

app = bindings.app
app.__proto__ = EventEmitter.prototype

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

# Helpers.
app.resolveProxy = (url, callback) -> @defaultSession.resolveProxy url, callback

# Deprecated.
app.getHomeDir = deprecate 'app.getHomeDir', 'app.getPath', ->
  @getPath 'home'
app.getDataPath = deprecate 'app.getDataPath', 'app.getPath', ->
  @getPath 'userData'
app.setDataPath = deprecate 'app.setDataPath', 'app.setPath', (path) ->
  @setPath 'userData', path
deprecate.rename app, 'terminate', 'quit'
deprecate.event app, 'finish-launching', 'ready', ->
  setImmediate => # give default app a chance to setup default menu.
    @emit 'finish-launching'
deprecate.event app, 'activate-with-no-open-windows', 'activate', (event, hasVisibleWindows) ->
  @emit 'activate-with-no-open-windows' if not hasVisibleWindows

# Wrappers for native classes.
wrapSession = (session) ->
  # session is an EventEmitter.
  session.__proto__ = EventEmitter.prototype
sessionBindings._setWrapSession wrapSession

wrapDownloadItem = (downloadItem) ->
  # downloadItem is an EventEmitter.
  downloadItem.__proto__ = EventEmitter.prototype
  # Deprecated.
  deprecate.property downloadItem, 'url', 'getUrl'
  deprecate.property downloadItem, 'filename', 'getFilename'
  deprecate.property downloadItem, 'mimeType', 'getMimeType'
  deprecate.property downloadItem, 'hasUserGesture', 'hasUserGesture'
downloadItemBindings._setWrapDownloadItem wrapDownloadItem

# Only one App object pemitted.
module.exports = app

EventEmitter = require('events').EventEmitter

bindings = process.atomBinding 'app'

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

# Be compatible with old API.
app.once 'ready', -> @emit 'finish-launching'
app.terminate = app.quit
app.exit = process.exit
app.getHomeDir = -> @getPath 'home'
app.getDataPath = -> @getPath 'userData'
app.setDataPath = (path) -> @setPath 'userData', path
app.resolveProxy = -> @defaultSession.resolveProxy.apply @defaultSession, arguments

# Only one App object pemitted.
module.exports = app

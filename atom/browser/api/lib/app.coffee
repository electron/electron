EventEmitter = require('events').EventEmitter

bindings = process.atomBinding 'app'

Application = bindings.Application
Application::__proto__ = EventEmitter.prototype

app = new Application

app.getHomeDir = ->
  process.env[if process.platform is 'win32' then 'USERPROFILE' else 'HOME']

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

# Be compatible with old API.
app.once 'ready', -> app.emit 'finish-launching'
app.terminate = app.exit = app.quit

# Only one App object pemitted.
module.exports = app

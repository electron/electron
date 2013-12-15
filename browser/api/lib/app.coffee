bindings = process.atomBinding 'app'
EventEmitter = require('events').EventEmitter

Application = bindings.Application
Application::__proto__ = EventEmitter.prototype

app = new Application

app.getHomeDir = ->
  process.env[if process.platform is 'win32' then 'USERPROFILE' else 'HOME']

app.getBrowserWindows = ->
  require('../../lib/objects-registry.js').getAllWindows()

app.setApplicationMenu = (menu) ->
  require('menu').setApplicationMenu menu

app.getApplicationMenu = ->
  require('menu').getApplicationMenu()

app.commandLine =
  appendSwitch: bindings.appendSwitch,
  appendArgument: bindings.appendArgument

if process.platform is 'darwin'
  app.dock =
    bounce: (type = 'informational') -> bindings.dockBounce type
    cancelBounce: bindings.dockCancelBounce
    setBadge: bindings.dockSetBadgeText
    getBadge: bindings.dockGetBadgeText

# Only one App object pemitted.
module.exports = app

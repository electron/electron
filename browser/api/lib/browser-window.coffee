EventEmitter = require('events').EventEmitter
v8Util = process.atomBinding 'v8_util'
objectsRegistry = require '../../atom/objects-registry.js'

Window = process.atomBinding('window').Window
Window::__proto__ = EventEmitter.prototype

module.exports = class BrowserWindow extends Window
  constructor: ->
    super

  toggleDevTools: ->
    opened = v8Util.getHiddenValue this, 'devtoolsOpened'
    if opened
      @closeDevTools()
      v8Util.setHiddenValue this, 'devtoolsOpened', false
    else
      @openDevTools()
      v8Util.setHiddenValue this, 'devtoolsOpened', true

  restart: ->
    @loadUrl @getUrl()

  @getFocusedWindow: ->
    windows = objectsRegistry.getAllWindows()
    return window for window in windows when window.isFocused()

  @fromProcessIdAndRoutingId = (processId, routingId) ->
    windows = objectsRegistry.getAllWindows()
    return window for window in windows when window.getProcessId() == processId and
                                             window.getRoutingId() == routingId

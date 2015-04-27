EventEmitter = require('events').EventEmitter
NavigationController = require './navigation-controller'
binding = process.atomBinding 'web_contents'
ipc = require 'ipc'

module.exports.wrap = (webContents) ->
  return null unless webContents.isAlive()

  # webContents is an EventEmitter.
  webContents.__proto__ = EventEmitter.prototype

  # WebContents::send(channel, args..)
  webContents.send = (channel, args...) ->
    @_send channel, [args...]

  # Make sure webContents.executeJavaScript would run the code only when the
  # web contents has been loaded.
  webContents.loaded = false
  webContents.once 'did-finish-load', -> @loaded = true
  webContents.executeJavaScript = (code) ->
    if @loaded
      @_executeJavaScript code
    else
      webContents.once 'did-finish-load', @_executeJavaScript.bind(this, code)

  # The processId and routingId and identify a webContents.
  webContents.getId = -> "#{@getProcessId()}-#{@getRoutingId()}"
  webContents.equal = (other) -> @getId() is other.getId()

  # The navigation controller.
  controller = new NavigationController(webContents)
  webContents.controller = controller
  for name, method of NavigationController.prototype when method instanceof Function
    do (name, method) ->
      webContents[name] = -> method.apply controller, arguments

  # Translate |disposition| to string for 'new-window' event.
  webContents.on '-new-window', (args..., disposition) ->
    disposition =
      switch disposition
        when 2 then 'default'
        when 4 then 'foreground-tab'
        when 5 then 'background-tab'
        when 6, 7 then 'new-window'
        else 'other'
    @emit 'new-window', args..., disposition

  # Tell the rpc server that a render view has been deleted and we need to
  # release all objects owned by it.
  webContents.on 'render-view-deleted', (event, processId, routingId) ->
    process.emit 'ATOM_BROWSER_RELEASE_RENDER_VIEW', "#{processId}-#{routingId}"

  # Dispatch IPC messages to the ipc module.
  webContents.on 'ipc-message', (event, packed) ->
    [channel, args...] = packed
    Object.defineProperty event, 'sender', value: webContents
    ipc.emit channel, event, args...
  webContents.on 'ipc-message-sync', (event, packed) ->
    [channel, args...] = packed
    Object.defineProperty event, 'returnValue', set: (value) -> event.sendReply JSON.stringify(value)
    Object.defineProperty event, 'sender', value: webContents
    ipc.emit channel, event, args...

  webContents

module.exports.create = (options={}) ->
  @wrap binding.create(options)

ipc = require('ipc')
app = require('app')
v8Util = process.atomBinding 'v8_util'

destroyEvents = ['destroyed', 'crashed', 'did-navigate-to-different-page']

nextId = 0
getNextId = -> ++nextId

webviewRefs = {}
webviewRefsById = {}

class WebViewRefInternal
  constructor: (embedder, viewInstanceId, internalInstanceId) ->
    @id = getNextId()
    @embedder = embedder
    @viewInstanceId = viewInstanceId
    @internalInstanceId = internalInstanceId

    webviewRefs["#{@embedder.getId()}-#{@viewInstanceId}"] = this
    webviewRefsById[@id] = this

    for event in destroyEvents
      @embedder.once event, @destroy.bind(this)

  destroy: ->
    @embedder.removeListener event, @destroy for event in destroyEvents
    delete webviewRefs["#{@embedder.getId()}-#{@viewInstanceId}"]
    delete webviewRefsById[@id]
    @isDetached = true

  isAlive: ->
    return !@isDetached && @embedder && @embedder.isAlive()

  transferable: ->
    @isAlive() && @internalInstanceId

  gotInstanceId: (internalInstanceId) ->
    @internalInstanceId = internalInstanceId

  getId: ->
    return @id


class WebViewRef
  constructor: (embedder, viewInstanceId, internalInstanceId) ->
    v8Util.setHiddenValue this, 'internal', new WebViewRefInternal(embedder, viewInstanceId, internalInstanceId)

  isAlive: ->
    internal = v8Util.getHiddenValue this, 'internal'
    internal.isAlive()

  transferTo: (webViewRef) ->
    source = v8Util.getHiddenValue this, 'internal'
    target = v8Util.getHiddenValue webViewRef, 'internal'

    #let guest-view-manager do the job
    app.emit 'ATOM_SHELL_GUEST_VIEW_MANAGER_TRANSFER', source, target

  getRefId: ->
    internal = v8Util.getHiddenValue this, 'internal'
    internal.getId()

ipc.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_WEB_VIEW_INTERNALINSTANCEID', (event, viewInstanceId, internalInstanceId) ->
  webviewRef = webviewRefs["#{event.sender.getId()}-#{viewInstanceId}"]
  if webviewRef
    webviewRef.gotInstanceId internalInstanceId

ipc.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_WEB_VIEW_DETACHED', (event, viewInstanceId) ->
  webviewRef = webviewRefs["#{event.sender.getId()}-#{viewInstanceId}"]
  if webviewRef
    webviewRef.destroy()

ipc.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_TRANSFER_REMOTE_WEBVIEWREF', (event, sourceViewInstanceId, sourceInternalInstanceId, webViewRefId) ->
  target = webviewRefsById[webViewRefId]
  source = webviewRefs["#{event.sender.getId()}-#{sourceViewInstanceId}"]
  if !source
    source = new WebViewRefInternal(event.sender, sourceViewInstanceId, sourceInternalInstanceId)
  return unless target && source

  app.emit 'ATOM_SHELL_GUEST_VIEW_MANAGER_TRANSFER', source, target

ipc.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_TRANSFER_REMOTE_WEBVIEW', (event, sourceViewInstanceId, sourceInternalInstanceId, targetViewInstanceId) ->
  target = webviewRefs["#{event.sender.getId()}-#{targetViewInstanceId}"]
  if !target
    target = new WebViewRefInternal(event.sender, targetViewInstanceId, 0)
  source = webviewRefs["#{event.sender.getId()}-#{sourceViewInstanceId}"]
  if !source
    source = new WebViewRefInternal(event.sender, sourceViewInstanceId, sourceInternalInstanceId)
  return unless target && source

  app.emit 'ATOM_SHELL_GUEST_VIEW_MANAGER_TRANSFER', source, target

module.exports = WebViewRef
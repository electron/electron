ipc = require('ipc')
v8Util = process.atomBinding 'v8_util'

destroyEvents = ['destroyed', 'crashed', 'did-navigate-to-different-page']

webviewRefs = {}

class WebViewRefInternal
  constructor: (embedder, viewInstanceId, internalInstanceId) ->
    @embedder = embedder
    @viewInstanceId = viewInstanceId
    @internalInstanceId = internalInstanceId
    webviewRefs["#{@embedder.getId()}-#{@viewInstanceId}"] = this

    for event in destroyEvents
      @embedder.once event, @destroy.bind(this)

  destroy: ->
    @embedder.removeListener event, @destroy for event in destroyEvents
    delete webviewRefs["#{@embedder.getId()}-#{@viewInstanceId}"]
    @isDetached = true

  isAlive: ->
    return !@isDetached && @embedder && @embedder.isAlive()

  gotInstanceId: (internalInstanceId) ->
    @internalInstanceId = internalInstanceId


class WebViewRef
  constructor: (embedder, viewInstanceId) ->
    v8Util.setHiddenValue this, 'internal', new WebViewRefInternal(embedder, viewInstanceId)

  isAlive: ->
    internal = v8Util.getHiddenValue this, 'internal'
    internal.isAlive()

  transferTo: (webViewRef, callback) ->
    callback()

ipc.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_WEB_VIEW_INTERNALINSTANCEID', (event, viewInstanceId, internalInstanceId) ->
  webviewRef = webviewRefs["#{event.sender.getId()}-#{viewInstanceId}"]
  if webviewRef
    webviewRef.gotInstanceId internalInstanceId

ipc.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_WEB_VIEW_DETACHED', (event, viewInstanceId) ->
  webviewRef = webviewRefs["#{event.sender.getId()}-#{viewInstanceId}"]
  if webviewRef
    webviewRef.destroy()

module.exports = WebViewRef
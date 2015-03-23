v8Util = process.atomBinding 'v8_util'
guestViewInternal = require './guest-view-internal'
webViewConstants = require './web-view-constants'
webFrame = require 'web-frame'
remote = require 'remote'

# ID generator.
nextId = 0
getNextId = -> ++nextId

# Represents the internal state of the WebView node.
class WebViewImpl
  constructor: (@webviewNode) ->
    v8Util.setHiddenValue @webviewNode, 'internal', this
    @attached = false
    @pendingGuestCreation = false
    @elementAttached = false

    @beforeFirstNavigation = true

    # on* Event handlers.
    @on = {}

    @browserPluginNode = @createBrowserPluginNode()
    shadowRoot = @webviewNode.createShadowRoot()
    @setupWebViewAttributes()
    @setupFocusPropagation()

    @viewInstanceId = getNextId()

    shadowRoot.appendChild @browserPluginNode

  createBrowserPluginNode: ->
    # We create BrowserPlugin as a custom element in order to observe changes
    # to attributes synchronously.
    browserPluginNode = new WebViewImpl.BrowserPlugin()
    v8Util.setHiddenValue browserPluginNode, 'internal', this
    browserPluginNode

  # Resets some state upon reattaching <webview> element to the DOM.
  reset: ->
    # If guestInstanceId is defined then the <webview> has navigated and has
    # already picked up a partition ID. Thus, we need to reset the initialization
    # state. However, it may be the case that beforeFirstNavigation is false BUT
    # guestInstanceId has yet to be initialized. This means that we have not
    # heard back from createGuest yet. We will not reset the flag in this case so
    # that we don't end up allocating a second guest.
    if @guestInstanceId
      guestViewInternal.destroyGuest @guestInstanceId
      @guestInstanceId = undefined
      @beforeFirstNavigation = true
      @attributes[webViewConstants.ATTRIBUTE_PARTITION].validPartitionId = true
    @internalInstanceId = 0

  # Sets the <webview>.request property.
  setRequestPropertyOnWebViewNode: (request) ->
    Object.defineProperty @webviewNode, 'request', value: request, enumerable: true

  setupFocusPropagation: ->
    unless @webviewNode.hasAttribute 'tabIndex'
      # <webview> needs a tabIndex in order to be focusable.
      # TODO(fsamuel): It would be nice to avoid exposing a tabIndex attribute
      # to allow <webview> to be focusable.
      # See http://crbug.com/231664.
      @webviewNode.setAttribute 'tabIndex', -1
    @webviewNode.addEventListener 'focus', (e) =>
      # Focus the BrowserPlugin when the <webview> takes focus.
      @browserPluginNode.focus()
    @webviewNode.addEventListener 'blur', (e) =>
      # Blur the BrowserPlugin when the <webview> loses focus.
      @browserPluginNode.blur()

  # This observer monitors mutations to attributes of the <webview> and
  # updates the BrowserPlugin properties accordingly. In turn, updating
  # a BrowserPlugin property will update the corresponding BrowserPlugin
  # attribute, if necessary. See BrowserPlugin::UpdateDOMAttribute for more
  # details.
  handleWebviewAttributeMutation: (attributeName, oldValue, newValue) ->
    if not @attributes[attributeName] or @attributes[attributeName].ignoreMutation
      return

    # Let the changed attribute handle its own mutation;
    @attributes[attributeName].handleMutation oldValue, newValue

  handleBrowserPluginAttributeMutation: (attributeName, oldValue, newValue) ->
    if attributeName is webViewConstants.ATTRIBUTE_INTERNALINSTANCEID and !oldValue and !!newValue
      @browserPluginNode.removeAttribute webViewConstants.ATTRIBUTE_INTERNALINSTANCEID
      @internalInstanceId = parseInt newValue

      return unless @guestInstanceId

      guestViewInternal.attachGuest @internalInstanceId, @guestInstanceId, @buildAttachParams()

  onSizeChanged: (webViewEvent) ->
    newWidth = webViewEvent.newWidth
    newHeight = webViewEvent.newHeight

    node = @webviewNode

    width = node.offsetWidth
    height = node.offsetHeight

    # Check the current bounds to make sure we do not resize <webview>
    # outside of current constraints.
    maxWidth = @attributes[webViewConstants.ATTRIBUTE_MAXWIDTH].getValue() | width
    maxHeight = @attributes[webViewConstants.ATTRIBUTE_MAXHEIGHT].getValue() | width
    minWidth = @attributes[webViewConstants.ATTRIBUTE_MINWIDTH].getValue() | width
    minHeight = @attributes[webViewConstants.ATTRIBUTE_MINHEIGHT].getValue() | width

    if not @attributes[webViewConstants.ATTRIBUTE_AUTOSIZE].getValue() or
       (newWidth >= minWidth and
        newWidth <= maxWidth and
        newHeight >= minHeight and
        newHeight <= maxHeight)
      node.style.width = newWidth + 'px'
      node.style.height = newHeight + 'px'
      # Only fire the DOM event if the size of the <webview> has actually
      # changed.
      @dispatchEvent webViewEvent

  createGuest: ->
    return if @pendingGuestCreation
    params =
      storagePartitionId: @attributes[webViewConstants.ATTRIBUTE_PARTITION].getValue()
    guestViewInternal.createGuest 'webview', params, (guestInstanceId) =>
      @pendingGuestCreation = false
      unless @elementAttached
        guestViewInternal.destroyGuest guestInstanceId
        return
      @attachWindow guestInstanceId
    @pendingGuestCreation = true

  dispatchEvent: (webViewEvent) ->
    @webviewNode.dispatchEvent webViewEvent

  # Adds an 'on<event>' property on the webview, which can be used to set/unset
  # an event handler.
  setupEventProperty: (eventName) ->
    propertyName = 'on' + eventName.toLowerCase()
    Object.defineProperty @webviewNode, propertyName,
      get: => @on[propertyName]
      set: (value) =>
        if @on[propertyName]
          @webviewNode.removeEventListener eventName, @on[propertyName]
        @on[propertyName] = value
        if value
          @webviewNode.addEventListener eventName, value
      enumerable: true

  # Updates state upon loadcommit.
  onLoadCommit: (@baseUrlForDataUrl, @currentEntryIndex, @entryCount, @processId, url, isTopLevel) ->
    oldValue = @webviewNode.getAttribute webViewConstants.ATTRIBUTE_SRC
    newValue = url
    if isTopLevel and (oldValue != newValue)
      # Touching the src attribute triggers a navigation. To avoid
      # triggering a page reload on every guest-initiated navigation,
      # we do not handle this mutation
      @attributes[webViewConstants.ATTRIBUTE_SRC].setValueIgnoreMutation newValue

  onAttach: (storagePartitionId) ->
    @attributes[webViewConstants.ATTRIBUTE_PARTITION].setValue storagePartitionId

  buildAttachParams: ->
    params =
      instanceId: @viewInstanceId
      userAgentOverride: @userAgentOverride
    for attributeName, attribute of @attributes
      params[attributeName] = attribute.getValue()
    params

  attachWindow: (guestInstanceId) ->
    @guestInstanceId = guestInstanceId
    params = @buildAttachParams()

    return true unless @internalInstanceId

    guestViewInternal.attachGuest @internalInstanceId, @guestInstanceId, params

# Registers browser plugin <object> custom element.
registerBrowserPluginElement = ->
  proto = Object.create HTMLObjectElement.prototype

  proto.createdCallback = ->
    @setAttribute 'type', 'application/browser-plugin'
    @setAttribute 'id', 'browser-plugin-' + getNextId()
    # The <object> node fills in the <webview> container.
    @style.display = 'block'
    @style.width = '100%'
    @style.height = '100%'

  proto.attributeChangedCallback = (name, oldValue, newValue) ->
    internal = v8Util.getHiddenValue this, 'internal'
    return unless internal
    internal.handleBrowserPluginAttributeMutation name, oldValue, newValue

  proto.attachedCallback = ->
    # Load the plugin immediately.
    unused = this.nonExistentAttribute

  WebViewImpl.BrowserPlugin = webFrame.registerEmbedderCustomElement 'browserplugin',
    extends: 'object', prototype: proto

  delete proto.createdCallback
  delete proto.attachedCallback
  delete proto.detachedCallback
  delete proto.attributeChangedCallback

# Registers <webview> custom element.
registerWebViewElement = ->
  proto = Object.create HTMLObjectElement.prototype

  proto.createdCallback = ->
    new WebViewImpl(this)

  proto.attributeChangedCallback = (name, oldValue, newValue) ->
    internal = v8Util.getHiddenValue this, 'internal'
    return unless internal
    internal.handleWebviewAttributeMutation name, oldValue, newValue

  proto.detachedCallback = ->
    internal = v8Util.getHiddenValue this, 'internal'
    return unless internal
    guestViewInternal.deregisterEvents internal.viewInstanceId
    internal.elementAttached = false
    internal.reset()

  proto.attachedCallback = ->
    internal = v8Util.getHiddenValue this, 'internal'
    return unless internal
    unless internal.elementAttached
      guestViewInternal.registerEvents internal, internal.viewInstanceId
      internal.elementAttached = true
      internal.attributes[webViewConstants.ATTRIBUTE_SRC].parse()

  # Public-facing API methods.
  methods = [
    "getUrl"
    "getTitle"
    "isLoading"
    "isWaitingForResponse"
    "stop"
    "reload"
    "reloadIgnoringCache"
    "canGoBack"
    "canGoForward"
    "canGoToOffset"
    "goBack"
    "goForward"
    "goToIndex"
    "goToOffset"
    "isCrashed"
    "setUserAgent"
    "executeJavaScript"
    "insertCSS"
    "openDevTools"
    "closeDevTools"
    "isDevToolsOpened"
    "undo"
    "redo"
    "cut"
    "copy"
    "paste"
    "delete"
    "selectAll"
    "unselect"
    "replace"
    "replaceMisspelling"
    "send"
    "getId"
  ]

  # Forward proto.foo* method calls to WebViewImpl.foo*.
  createHandler = (m) ->
    (args...) ->
      internal = v8Util.getHiddenValue this, 'internal'
      remote.getGuestWebContents(internal.guestInstanceId)[m]  args...
  proto[m] = createHandler m for m in methods

  window.WebView = webFrame.registerEmbedderCustomElement 'webview',
    prototype: proto

  # Delete the callbacks so developers cannot call them and produce unexpected
  # behavior.
  delete proto.createdCallback
  delete proto.attachedCallback
  delete proto.detachedCallback
  delete proto.attributeChangedCallback

useCapture = true
listener = (event) ->
  return if document.readyState == 'loading'
  registerBrowserPluginElement()
  registerWebViewElement()
  window.removeEventListener event.type, listener, useCapture
window.addEventListener 'readystatechange', listener, true

module.exports = WebViewImpl

v8Util = process.atomBinding 'v8_util'
guestViewInternal = require './guest-view-internal'
webFrame = require 'web-frame'
remote = require 'remote'

# ID generator.
nextId = 0
getNextId = -> ++nextId

# FIXME
# Discarded after Chrome 39
PLUGIN_METHOD_ATTACH = '-internal-attach'

# Attributes.
WEB_VIEW_ATTRIBUTE_ALLOWTRANSPARENCY = 'allowtransparency'
WEB_VIEW_ATTRIBUTE_AUTOSIZE = 'autosize'
WEB_VIEW_ATTRIBUTE_MAXHEIGHT = 'maxheight'
WEB_VIEW_ATTRIBUTE_MAXWIDTH = 'maxwidth'
WEB_VIEW_ATTRIBUTE_MINHEIGHT = 'minheight'
WEB_VIEW_ATTRIBUTE_MINWIDTH = 'minwidth'
WEB_VIEW_ATTRIBUTE_PARTITION = 'partition'
AUTO_SIZE_ATTRIBUTES = [
  WEB_VIEW_ATTRIBUTE_AUTOSIZE,
  WEB_VIEW_ATTRIBUTE_MAXHEIGHT,
  WEB_VIEW_ATTRIBUTE_MAXWIDTH,
  WEB_VIEW_ATTRIBUTE_MINHEIGHT,
  WEB_VIEW_ATTRIBUTE_MINWIDTH,
]

# Error messages.
ERROR_MSG_ALREADY_NAVIGATED =
    'The object has already navigated, so its partition cannot be changed.'
ERROR_MSG_CANNOT_INJECT_SCRIPT = '<webview>: ' +
    'Script cannot be injected into content until the page has loaded.'
ERROR_MSG_CONTENTWINDOW_NOT_AVAILABLE = '<webview>: ' +
    'contentWindow is not available at this time. It will become available ' +
    'when the page has finished loading.'
ERROR_MSG_INVALID_PARTITION_ATTRIBUTE = 'Invalid partition attribute.'

# Represents the state of the storage partition.
class Partition
  constructor: ->
    @validPartitionId = true
    @persistStorage = false
    @storagePartitionId = ''

  toAttribute: ->
    return '' unless @validPartitionId
    (if @persistStorage then 'persist:' else '') + @storagePartitionId

  fromAttribute: (value, hasNavigated) ->
    result = {}
    if hasNavigated
      result.error = ERROR_MSG_ALREADY_NAVIGATED
      return result
    value ?= ''

    LEN = 'persist:'.length
    if value.substr(0, LEN) == 'persist:'
      value = value.substr LEN
      unless value
        @validPartitionId = false
        result.error = ERROR_MSG_INVALID_PARTITION_ATTRIBUTE
        return result
      @persistStorage = true
    else
      @persistStorage = false

    @storagePartitionId = value
    result

# Represents the internal state of the WebView node.
class WebView
  constructor: (@webviewNode) ->
    v8Util.setHiddenValue @webviewNode, 'internal', this
    @attached = false
    @pendingGuestCreation = false
    @elementAttached = false

    @beforeFirstNavigation = true
    @contentWindow = null
    @validPartitionId = true
    # Used to save some state upon deferred attachment.
    # If <object> bindings is not available, we defer attachment.
    # This state contains whether or not the attachment request was for
    # newwindow.
    @deferredAttachState = null

    # on* Event handlers.
    @on = {}

    @browserPluginNode = @createBrowserPluginNode()
    shadowRoot = @webviewNode.createShadowRoot()
    @partition = new Partition()

    @setupWebViewSrcAttributeMutationObserver()
    @setupFocusPropagation()
    @setupWebviewNodeProperties()

    @viewInstanceId = getNextId()
    guestViewInternal.registerEvents this, @viewInstanceId

    shadowRoot.appendChild @browserPluginNode

  createBrowserPluginNode: ->
    # We create BrowserPlugin as a custom element in order to observe changes
    # to attributes synchronously.
    browserPluginNode = new WebView.BrowserPlugin()
    v8Util.setHiddenValue browserPluginNode, 'internal', this
    browserPluginNode

  getGuestInstanceId: ->
    @guestInstanceId

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
      @validPartitionId = true
      @partition.validPartitionId = true
      @contentWindow = null
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

  # Validation helper function for executeScript() and insertCSS().
  validateExecuteCodeCall: ->
    throw new Error(ERROR_MSG_CANNOT_INJECT_SCRIPT) unless @guestInstanceId

  setupAutoSizeProperties: ->
    for attributeName in AUTO_SIZE_ATTRIBUTES
      this[attributeName] = @webviewNode.getAttribute attributeName
      Object.defineProperty @webviewNode, attributeName,
        get: => this[attributeName]
        set: (value) => @webviewNode.setAttribute attributeName, value
        enumerable: true

  setupWebviewNodeProperties: ->
    @setupAutoSizeProperties()

    Object.defineProperty @webviewNode, WEB_VIEW_ATTRIBUTE_ALLOWTRANSPARENCY,
      get: => @allowtransparency
      set: (value) =>
        @webviewNode.setAttribute WEB_VIEW_ATTRIBUTE_ALLOWTRANSPARENCY, value
      enumerable: true

    # We cannot use {writable: true} property descriptor because we want a
    # dynamic getter value.
    Object.defineProperty @webviewNode, 'contentWindow',
      get: =>
        return @contentWindow if @contentWindow?
        window.console.error ERROR_MSG_CONTENTWINDOW_NOT_AVAILABLE
      # No setter.
      enumerable: true

    Object.defineProperty @webviewNode, 'partition',
      get: => @partition.toAttribute()
      set: (value) =>
        result = @partition.fromAttribute value, @hasNavigated()
        throw result.error if result.error?
        @webviewNode.setAttribute 'partition', value
      enumerable: true

    @src = @webviewNode.getAttribute 'src'
    Object.defineProperty @webviewNode, 'src',
      get: => @src
      set: (value) => @webviewNode.setAttribute 'src', value
      # No setter.
      enumerable: true

  # The purpose of this mutation observer is to catch assignment to the src
  # attribute without any changes to its value. This is useful in the case
  # where the webview guest has crashed and navigating to the same address
  # spawns off a new process.
  setupWebViewSrcAttributeMutationObserver: ->
    @srcAndPartitionObserver = new MutationObserver (mutations) =>
      for mutation in mutations
        oldValue = mutation.oldValue
        newValue = @webviewNode.getAttribute mutation.attributeName
        return if oldValue isnt newValue
        @handleWebviewAttributeMutation mutation.attributeName, oldValue, newValue
    params =
      attributes: true,
      attributeOldValue: true,
      attributeFilter: ['src', 'partition']
    @srcAndPartitionObserver.observe @webviewNode, params

  # This observer monitors mutations to attributes of the <webview> and
  # updates the BrowserPlugin properties accordingly. In turn, updating
  # a BrowserPlugin property will update the corresponding BrowserPlugin
  # attribute, if necessary. See BrowserPlugin::UpdateDOMAttribute for more
  # details.
  handleWebviewAttributeMutation: (name, oldValue, newValue) ->
    if name in AUTO_SIZE_ATTRIBUTES
      this[name] = newValue
      return unless @guestInstanceId
      # Convert autosize attribute to boolean.
      autosize = @webviewNode.hasAttribute WEB_VIEW_ATTRIBUTE_AUTOSIZE
      guestViewInternal.setAutoSize @guestInstanceId,
        enableAutoSize: autosize,
        min:
          width: parseInt @minwidth || 0
          height: parseInt @minheight || 0
        max:
          width: parseInt @maxwidth || 0
          height: parseInt @maxheight || 0
    else if name is WEB_VIEW_ATTRIBUTE_ALLOWTRANSPARENCY
      # We treat null attribute (attribute removed) and the empty string as
      # one case.
      oldValue ?= ''
      newValue ?= ''

      return if oldValue is newValue
      @allowtransparency = newValue != ''

      return unless @guestInstanceId

      guestViewInternal.setAllowTransparency @guestInstanceId, @allowtransparency
    else if name is 'src'
      # We treat null attribute (attribute removed) and the empty string as
      # one case.
      oldValue ?= ''
      newValue ?= ''
      # Once we have navigated, we don't allow clearing the src attribute.
      # Once <webview> enters a navigated state, it cannot be return back to a
      # placeholder state.
      if newValue == '' and oldValue != ''
        # src attribute changes normally initiate a navigation. We suppress
        # the next src attribute handler call to avoid reloading the page
        # on every guest-initiated navigation.
        @ignoreNextSrcAttributeChange = true
        @webviewNode.setAttribute 'src', oldValue
      @src = newValue
      if @ignoreNextSrcAttributeChange
        # Don't allow the src mutation observer to see this change.
        @srcAndPartitionObserver.takeRecords()
        @ignoreNextSrcAttributeChange = false
        return
      result = {}
      @parseSrcAttribute result

      throw result.error if result.error?
    else if name is 'partition'
      # Note that throwing error here won't synchronously propagate.
      @partition.fromAttribute newValue, @hasNavigated()

  handleBrowserPluginAttributeMutation: (name, oldValue, newValue) ->
    # FIXME
    # internalbindings => internalInstanceid after Chrome 39
    if name is 'internalbindings' and !oldValue and !!newValue
      @browserPluginNode.removeAttribute 'internalbindings'
      # FIXME
      # @internalInstanceId = parseInt newValue

      if !!@guestInstanceId and @guestInstanceId != 0
        isNewWindow = if @deferredAttachState then @deferredAttachState.isNewWindow else false
        params = @buildAttachParams isNewWindow
        # FIXME
        # guestViewInternalNatives.AttachGuest
        #   @internalInstanceId,
        #   @guestInstanceId,
        #   params,
        #   (w) => @contentWindow = w
        @browserPluginNode[PLUGIN_METHOD_ATTACH] @guestInstanceId, params

  onSizeChanged: (webViewEvent) ->
    newWidth = webViewEvent.newWidth
    newHeight = webViewEvent.newHeight

    node = @webviewNode

    width = node.offsetWidth
    height = node.offsetHeight

    # Check the current bounds to make sure we do not resize <webview>
    # outside of current constraints.
    if node.hasAttribute(WEB_VIEW_ATTRIBUTE_MAXWIDTH) and
       node[WEB_VIEW_ATTRIBUTE_MAXWIDTH]
      maxWidth = node[WEB_VIEW_ATTRIBUTE_MAXWIDTH]
    else
      maxWidth = width

    if node.hasAttribute(WEB_VIEW_ATTRIBUTE_MINWIDTH) and
       node[WEB_VIEW_ATTRIBUTE_MINWIDTH]
      minWidth = node[WEB_VIEW_ATTRIBUTE_MINWIDTH]
    else
      minWidth = width
    minWidth = maxWidth if minWidth > maxWidth

    if node.hasAttribute(WEB_VIEW_ATTRIBUTE_MAXHEIGHT) and
       node[WEB_VIEW_ATTRIBUTE_MAXHEIGHT]
      maxHeight = node[WEB_VIEW_ATTRIBUTE_MAXHEIGHT]
    else
      maxHeight = height

    if node.hasAttribute(WEB_VIEW_ATTRIBUTE_MINHEIGHT) and
       node[WEB_VIEW_ATTRIBUTE_MINHEIGHT]
      minHeight = node[WEB_VIEW_ATTRIBUTE_MINHEIGHT]
    else
      minHeight = height
    minHeight = maxHeight if minHeight > maxHeight

    if not @webviewNode.hasAttribute WEB_VIEW_ATTRIBUTE_AUTOSIZE or
       (newWidth >= minWidth and
        newWidth <= maxWidth and
        newHeight >= minHeight and
        newHeight <= maxHeight)
      node.style.width = newWidth + 'px'
      node.style.height = newHeight + 'px'
      # Only fire the DOM event if the size of the <webview> has actually
      # changed.
      @dispatchEvent webViewEvent

  # Returns if <object> is in the render tree.
  isPluginInRenderTree: ->
    # FIXME
    # !!@internalInstanceId && @internalInstanceId != 0
    'function' == typeof this.browserPluginNode[PLUGIN_METHOD_ATTACH]

  hasNavigated: ->
    not @beforeFirstNavigation

  parseSrcAttribute: (result) ->
    unless @partition.validPartitionId
      result.error = ERROR_MSG_INVALID_PARTITION_ATTRIBUTE
      return
    @src = @webviewNode.getAttribute 'src'

    return unless @src

    unless @guestInstanceId?
      if @beforeFirstNavigation
        @beforeFirstNavigation = false
        @createGuest()
      return

    # Navigate to |this.src|.
    remote.getGuestWebContents(@guestInstanceId).loadUrl @src

  parseAttributes: ->
    return unless @elementAttached
    hasNavigated = @hasNavigated()
    attributeValue = @webviewNode.getAttribute 'partition'
    result = @partition.fromAttribute attributeValue, hasNavigated
    @parseSrcAttribute result

  createGuest: ->
    return if @pendingGuestCreation
    storagePartitionId =
      @webviewNode.getAttribute(WEB_VIEW_ATTRIBUTE_PARTITION) or
      @webviewNode[WEB_VIEW_ATTRIBUTE_PARTITION]
    params = storagePartitionId: storagePartitionId
    guestViewInternal.createGuest 'webview', params, (guestInstanceId) =>
      @pendingGuestCreation = false
      unless @elementAttached
        guestViewInternal.destroyGuest guestInstanceId
        return
      @attachWindow guestInstanceId, false
    @pendingGuestCreation = true

  @dispatchEvent = (webViewEvent) ->
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
    oldValue = @webviewNode.getAttribute 'src'
    newValue = url
    if isTopLevel and (oldValue != newValue)
      # Touching the src attribute triggers a navigation. To avoid
      # triggering a page reload on every guest-initiated navigation,
      # we use the flag ignoreNextSrcAttributeChange here.
      this.ignoreNextSrcAttributeChange = true
      this.webviewNode.setAttribute 'src', newValue

  onAttach: (storagePartitionId) ->
    @webviewNode.setAttribute 'partition', storagePartitionId
    @partition.fromAttribute storagePartitionId, this.hasNavigated()

  buildAttachParams: (isNewWindow) ->
    allowtransparency: @allowtransparency || false
    autosize: @webviewNode.hasAttribute WEB_VIEW_ATTRIBUTE_AUTOSIZE
    instanceId: @viewInstanceId
    maxheight: parseInt @maxheight || 0
    maxwidth: parseInt @maxwidth || 0
    minheight: parseInt @minheight || 0
    minwidth: parseInt @minwidth || 0
    # We don't need to navigate new window from here.
    src: if isNewWindow then undefined else @src
    # If we have a partition from the opener, that will also be already
    # set via this.onAttach().
    storagePartitionId: @partition.toAttribute()
    userAgentOverride: @userAgentOverride

  attachWindow: (guestInstanceId, isNewWindow) ->
    @guestInstanceId = guestInstanceId
    params = @buildAttachParams isNewWindow

    unless @isPluginInRenderTree()
      @deferredAttachState = isNewWindow: isNewWindow
      return true

    @deferredAttachState = null
    # FIXME
    # guestViewInternalNatives.AttachGuest @internalInstanceId, @guestInstanceId, params, (w) => @contentWindow = w
    @browserPluginNode[PLUGIN_METHOD_ATTACH] @guestInstanceId, params

# Registers browser plugin <object> custom element.
registerBrowserPluginElement = ->
  proto = Object.create HTMLObjectElement.prototype

  proto.createdCallback = ->
    @setAttribute 'type', 'application/browser-plugin'
    @setAttribute 'id', 'browser-plugin-' + getNextId()
    # The <object> node fills in the <webview> container.
    @style.width = '100%'
    @style.height = '100%'

  proto.attributeChangedCallback = (name, oldValue, newValue) ->
    internal = v8Util.getHiddenValue this, 'internal'
    return unless internal
    internal.handleBrowserPluginAttributeMutation name, oldValue, newValue

  proto.attachedCallback = ->
    # Load the plugin immediately.
    unused = this.nonExistentAttribute

  WebView.BrowserPlugin = webFrame.registerEmbedderCustomElement 'browserplugin',
    extends: 'object', prototype: proto

  delete proto.createdCallback
  delete proto.attachedCallback
  delete proto.detachedCallback
  delete proto.attributeChangedCallback

# Registers <webview> custom element.
registerWebViewElement = ->
  proto = Object.create HTMLObjectElement.prototype

  proto.createdCallback = ->
    new WebView(this)

  proto.attributeChangedCallback = (name, oldValue, newValue) ->
    internal = v8Util.getHiddenValue this, 'internal'
    return unless internal
    internal.handleWebviewAttributeMutation name, oldValue, newValue

  proto.detachedCallback = ->
    internal = v8Util.getHiddenValue this, 'internal'
    return unless internal
    internal.elementAttached = false
    internal.reset()

  proto.attachedCallback = ->
    internal = v8Util.getHiddenValue this, 'internal'
    return unless internal
    unless internal.elementAttached
      internal.elementAttached = true
      internal.parseAttributes()

  # Public-facing API methods.
  methods = [
    "getUrl"
    "getTitle"
    "isLoading"
    "isWaitingForResponse"
    "stop"
    "reload"
    "reloadIngoringCache"
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
    "send"
  ]

  # Forward proto.foo* method calls to WebView.foo*.
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

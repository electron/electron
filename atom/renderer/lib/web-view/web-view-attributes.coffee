WebViewImpl = require './web-view'
guestViewInternal = require './guest-view-internal'
webViewConstants = require './web-view-constants'
remote = require 'remote'

# Helper function to resolve url set in attribute.
a = document.createElement 'a'
resolveUrl = (url) ->
  a.href = url
  a.href

# Attribute objects.
# Default implementation of a WebView attribute.
class WebViewAttribute
  constructor: (name, webViewImpl) ->
    @name = name
    @webViewImpl = webViewImpl
    @ignoreMutation = false

    @defineProperty()

  # Retrieves and returns the attribute's value.
  getValue: -> @webViewImpl.webviewNode.getAttribute(@name) || ''

  # Sets the attribute's value.
  setValue: (value) -> @webViewImpl.webviewNode.setAttribute(@name, value || '')

  # Changes the attribute's value without triggering its mutation handler.
  setValueIgnoreMutation: (value) ->
    @ignoreMutation = true
    @setValue value
    @ignoreMutation = false

  # Defines this attribute as a property on the webview node.
  defineProperty: ->
    Object.defineProperty @webViewImpl.webviewNode, @name,
      get: => @getValue()
      set: (value) => @setValue value
      enumerable: true

  # Called when the attribute's value changes.
  handleMutation: ->

# An attribute that is treated as a Boolean.
class BooleanAttribute extends WebViewAttribute
  constructor: (name, webViewImpl) ->
    super name, webViewImpl

  getValue: -> @webViewImpl.webviewNode.hasAttribute @name

  setValue: (value) ->
    unless value
      @webViewImpl.webviewNode.removeAttribute @name
    else
      @webViewImpl.webviewNode.setAttribute @name, ''

# Attribute that specifies whether transparency is allowed in the webview.
class AllowTransparencyAttribute extends BooleanAttribute
  constructor: (webViewImpl) ->
    super webViewConstants.ATTRIBUTE_ALLOWTRANSPARENCY, webViewImpl

  handleMutation: (oldValue, newValue) ->
    return unless @webViewImpl.guestInstanceId
    guestViewInternal.setAllowTransparency @webViewImpl.guestInstanceId, @getValue()

# Attribute used to define the demension limits of autosizing.
class AutosizeDimensionAttribute extends WebViewAttribute
  constructor: (name, webViewImpl) ->
    super name, webViewImpl

  getValue: -> parseInt(@webViewImpl.webviewNode.getAttribute(@name)) || 0

  handleMutation: (oldValue, newValue) ->
    return unless @webViewImpl.guestInstanceId
    guestViewInternal.setSize @webViewImpl.guestInstanceId,
      enableAutoSize: @webViewImpl.attributes[webViewConstants.ATTRIBUTE_AUTOSIZE].getValue()
      min:
        width: parseInt @webViewImpl.attributes[webViewConstants.ATTRIBUTE_MINWIDTH].getValue() || 0
        height: parseInt @webViewImpl.attributes[webViewConstants.ATTRIBUTE_MINHEIGHT].getValue() || 0
      max:
        width: parseInt @webViewImpl.attributes[webViewConstants.ATTRIBUTE_MAXWIDTH].getValue() || 0
        height: parseInt @webViewImpl.attributes[webViewConstants.ATTRIBUTE_MAXHEIGHT].getValue() || 0

# Attribute that specifies whether the webview should be autosized.
class AutosizeAttribute extends BooleanAttribute
  constructor: (webViewImpl) ->
    super webViewConstants.ATTRIBUTE_AUTOSIZE, webViewImpl

  handleMutation: AutosizeDimensionAttribute::handleMutation

# Attribute representing the state of the storage partition.
class PartitionAttribute extends WebViewAttribute
  constructor: (webViewImpl) ->
    super webViewConstants.ATTRIBUTE_PARTITION, webViewImpl
    @validPartitionId = true

  handleMutation: (oldValue, newValue) ->
    newValue = newValue || ''

    # The partition cannot change if the webview has already navigated.
    unless @webViewImpl.beforeFirstNavigation
      window.console.error webViewConstants.ERROR_MSG_ALREADY_NAVIGATED
      @setValueIgnoreMutation oldValue
      return

    if newValue is 'persist:'
      @validPartitionId = false
      window.console.error webViewConstants.ERROR_MSG_INVALID_PARTITION_ATTRIBUTE

# Attribute that handles the location and navigation of the webview.
class SrcAttribute extends WebViewAttribute
  constructor: (webViewImpl) ->
    super webViewConstants.ATTRIBUTE_SRC, webViewImpl
    @setupMutationObserver()

  getValue: ->
    if @webViewImpl.webviewNode.hasAttribute @name
      resolveUrl @webViewImpl.webviewNode.getAttribute(@name)
    else
      ''

  setValueIgnoreMutation: (value) ->
    WebViewAttribute::setValueIgnoreMutation.call(this, value)
    # takeRecords() is needed to clear queued up src mutations. Without it, it
    # is possible for this change to get picked up asyncronously by src's
    # mutation observer |observer|, and then get handled even though we do not
    # want to handle this mutation.
    @observer.takeRecords()

  handleMutation: (oldValue, newValue) ->
    # Once we have navigated, we don't allow clearing the src attribute.
    # Once <webview> enters a navigated state, it cannot return to a
    # placeholder state.
    if not newValue and oldValue
      # src attribute changes normally initiate a navigation. We suppress
      # the next src attribute handler call to avoid reloading the page
      # on every guest-initiated navigation.
      @setValueIgnoreMutation oldValue
      return
    @parse()

  # The purpose of this mutation observer is to catch assignment to the src
  # attribute without any changes to its value. This is useful in the case
  # where the webview guest has crashed and navigating to the same address
  # spawns off a new process.
  setupMutationObserver: ->
    @observer = new MutationObserver (mutations) =>
      for mutation in mutations
        oldValue = mutation.oldValue
        newValue = @getValue()
        return if oldValue isnt newValue
        @handleMutation oldValue, newValue
    params =
      attributes: true,
      attributeOldValue: true,
      attributeFilter: [@name]
    @observer.observe @webViewImpl.webviewNode, params

  parse: ->
    if not @webViewImpl.elementAttached or
       not @webViewImpl.attributes[webViewConstants.ATTRIBUTE_PARTITION].validPartitionId or
       not @.getValue()
      return

    unless @webViewImpl.guestInstanceId?
      if @webViewImpl.beforeFirstNavigation
        @webViewImpl.beforeFirstNavigation = false
        @webViewImpl.createGuest()
      return

    # Navigate to |this.src|.
    opts = {}
    httpreferrer = @webViewImpl.attributes[webViewConstants.ATTRIBUTE_HTTPREFERRER].getValue()
    if httpreferrer then opts.httpReferrer = httpreferrer

    useragent = @webViewImpl.attributes[webViewConstants.ATTRIBUTE_USERAGENT].getValue()
    if useragent then opts.userAgent = useragent

    guestContents = remote.getGuestWebContents(@webViewImpl.guestInstanceId)
    guestContents.loadUrl @getValue(), opts

# Attribute specifies HTTP referrer.
class HttpReferrerAttribute extends WebViewAttribute
  constructor: (webViewImpl) ->
    super webViewConstants.ATTRIBUTE_HTTPREFERRER, webViewImpl

# Attribute specifies user agent
class UserAgentAttribute extends WebViewAttribute
  constructor: (webViewImpl) ->
    super webViewConstants.ATTRIBUTE_USERAGENT, webViewImpl

# Attribute that set preload script.
class PreloadAttribute extends WebViewAttribute
  constructor: (webViewImpl) ->
    super webViewConstants.ATTRIBUTE_PRELOAD, webViewImpl

  getValue: ->
    return '' unless @webViewImpl.webviewNode.hasAttribute @name
    preload = resolveUrl @webViewImpl.webviewNode.getAttribute(@name)
    protocol = preload.substr 0, 5
    unless protocol is 'file:'
      console.error webViewConstants.ERROR_MSG_INVALID_PRELOAD_ATTRIBUTE
      preload = ''
    preload

# Sets up all of the webview attributes.
WebViewImpl::setupWebViewAttributes = ->
  @attributes = {}

  @attributes[webViewConstants.ATTRIBUTE_ALLOWTRANSPARENCY] = new AllowTransparencyAttribute(this)
  @attributes[webViewConstants.ATTRIBUTE_AUTOSIZE] = new AutosizeAttribute(this)
  @attributes[webViewConstants.ATTRIBUTE_PARTITION] = new PartitionAttribute(this)
  @attributes[webViewConstants.ATTRIBUTE_SRC] = new SrcAttribute(this)
  @attributes[webViewConstants.ATTRIBUTE_HTTPREFERRER] = new HttpReferrerAttribute(this)
  @attributes[webViewConstants.ATTRIBUTE_USERAGENT] = new UserAgentAttribute(this)
  @attributes[webViewConstants.ATTRIBUTE_NODEINTEGRATION] = new BooleanAttribute(webViewConstants.ATTRIBUTE_NODEINTEGRATION, this)
  @attributes[webViewConstants.ATTRIBUTE_PLUGINS] = new BooleanAttribute(webViewConstants.ATTRIBUTE_PLUGINS, this)
  @attributes[webViewConstants.ATTRIBUTE_DISABLEWEBSECURITY] = new BooleanAttribute(webViewConstants.ATTRIBUTE_DISABLEWEBSECURITY, this)
  @attributes[webViewConstants.ATTRIBUTE_ALLOWPOPUPS] = new BooleanAttribute(webViewConstants.ATTRIBUTE_ALLOWPOPUPS, this)
  @attributes[webViewConstants.ATTRIBUTE_PRELOAD] = new PreloadAttribute(this)

  autosizeAttributes = [
    webViewConstants.ATTRIBUTE_MAXHEIGHT
    webViewConstants.ATTRIBUTE_MAXWIDTH
    webViewConstants.ATTRIBUTE_MINHEIGHT
    webViewConstants.ATTRIBUTE_MINWIDTH
  ]
  for attribute in autosizeAttributes
    @attributes[attribute] = new AutosizeDimensionAttribute(attribute, this)

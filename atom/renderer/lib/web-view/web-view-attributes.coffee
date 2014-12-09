WebView = require './web-view'
webViewConstants = require './web-view-constants'

# Attribute objects.
# Default implementation of a WebView attribute.
class WebViewAttribute
  constructor: (name, webViewImpl) ->
    @name = name
    @webViewImpl = webViewImpl
    @ignoreNextMutation = false

  # Retrieves and returns the attribute's value.
  getValue: -> @webViewImpl.webviewNode.getAttribute(@name) || ''

  # Sets the attribute's value.
  setValue: (value) -> @webViewImpl.webviewNode.setAttribute(@name, value || '')

  # Called when the attribute's value changes.
  handleMutation: ->

# Attribute specifying whether transparency is allowed in the webview.
class BooleanAttribute extends WebViewAttribute
  constructor: (name, webViewImpl) ->
    super name, webViewImpl

  getValue: ->
    # This attribute is treated as a boolean, and is retrieved as such.
    @webViewImpl.webviewNode.hasAttribute @name

  setValue: (value) ->
    unless value
      @webViewImpl.webviewNode.removeAttribute @name
    else
      @webViewImpl.webviewNode.setAttribute @name, ''

# Attribute representing the state of the storage partition.
class Partition extends WebViewAttribute
  constructor: (webViewImpl) ->
    super webViewConstants.ATTRIBUTE_PARTITION, webViewImpl
    @validPartitionId = true

  handleMutation: (oldValue, newValue) ->
    newValue = newValue || ''

    # The partition cannot change if the webview has already navigated.
    unless @webViewImpl.beforeFirstNavigation
      window.console.error webViewConstants.ERROR_MSG_ALREADY_NAVIGATED
      @ignoreNextMutation = true
      @webViewImpl.webviewNode.setAttribute @name, oldValue
      return

    if newValue is 'persist:'
      @validPartitionId = false
      window.console.error webViewConstants.ERROR_MSG_INVALID_PARTITION_ATTRIBUTE

# Sets up all of the webview attributes.
WebView::setupWebViewAttributes = ->
  @attributes = {}

  # Initialize the attributes with special behavior (and custom attribute
  # objects).
  @attributes[webViewConstants.ATTRIBUTE_ALLOWTRANSPARENCY] =
    new BooleanAttribute(webViewConstants.ATTRIBUTE_ALLOWTRANSPARENCY, this)
  @attributes[webViewConstants.ATTRIBUTE_AUTOSIZE] =
    new BooleanAttribute(webViewConstants.ATTRIBUTE_AUTOSIZE, this)
  @attributes[webViewConstants.ATTRIBUTE_PARTITION] = new Partition(this)

  # Initialize the remaining attributes, which have default behavior.
  defaultAttributes = [
    webViewConstants.ATTRIBUTE_MAXHEIGHT
    webViewConstants.ATTRIBUTE_MAXWIDTH
    webViewConstants.ATTRIBUTE_MINHEIGHT
    webViewConstants.ATTRIBUTE_MINWIDTH
    webViewConstants.ATTRIBUTE_SRC
    webViewConstants.ATTRIBUTE_HTTPREFERRER
  ]

  for attribute in defaultAttributes
    @attributes[attribute] = new WebViewAttribute(attribute, this)

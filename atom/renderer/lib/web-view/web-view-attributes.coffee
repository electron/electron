WebView = require './web-view'
webViewConstants = require './web-view-constants'

# Attribute objects.
# Default implementation of a WebView attribute.
class WebViewAttribute
  constructor: (name, webViewImpl) ->
    @name = name
    @value = ''
    @webViewImpl = webViewImpl

  getValue: -> @value || ''

  setValue: (value) -> @value = value

# Attribute representing the state of the storage partition.
class Partition extends WebViewAttribute
  constructor: (webViewImpl) ->
    super webViewConstants.ATTRIBUTE_PARTITION, webViewImpl

    @validPartitionId = true
    @persistStorage = false
    @storagePartitionId = ''
    @webViewImpl = webViewImpl

  getValue: ->
    return '' unless @validPartitionId
    (if @persistStorage then 'persist:' else '') + @storagePartitionId

  setValue: (value) ->
    result = {}
    hasNavigated = !@webViewImpl.beforeFirstNavigation
    if hasNavigated
      result.error = webViewConstants.ERROR_MSG_ALREADY_NAVIGATED
      return result
    value = '' unless value
    LEN = 'persist:'.length
    if value.substr(0, LEN) == 'persist:'
      value = value.substr LEN
      unless value
        @validPartitionId = false
        result.error = webViewConstants.ERROR_MSG_INVALID_PARTITION_ATTRIBUTE
        return result
      @persistStorage = true
    else
      @persistStorage = false
    @storagePartitionId = value
    result

# Sets up all of the webview attributes.
WebView::setupWebViewAttributes = ->
  @attributes = {}

  # Initialize the attributes with special behavior (and custom attribute
  # objects).
  @attributes[webViewConstants.ATTRIBUTE_PARTITION] = new Partition(this)

  # Initialize the remaining attributes, which have default behavior.
  defaultAttributes = [
    webViewConstants.ATTRIBUTE_ALLOWTRANSPARENCY
    webViewConstants.ATTRIBUTE_AUTOSIZE
    webViewConstants.ATTRIBUTE_MAXHEIGHT
    webViewConstants.ATTRIBUTE_MAXWIDTH
    webViewConstants.ATTRIBUTE_MINHEIGHT
    webViewConstants.ATTRIBUTE_MINWIDTH
    webViewConstants.ATTRIBUTE_SRC
    webViewConstants.ATTRIBUTE_HTTPREFERRER
  ]

  for attribute in defaultAttributes
    @attributes[attribute] = new WebViewAttribute(attribute, this)

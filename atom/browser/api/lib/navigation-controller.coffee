# JavaScript implementation of Chromium's NavigationController.
# Instead of relying on Chromium for history control, we compeletely do history
# control on user land, and only rely on WebContents.loadUrl for navigation.
# This helps us avoid Chromium's various optimizations so we can ensure renderer
# process is restarted everytime.
class NavigationController
  constructor: (@webContents) ->
    @history = []
    @currentIndex = -1
    @pendingIndex = -1

    @webContents.on 'navigation-entry-commited', (event, url) =>
      if @pendingIndex is -1  # Normal navigation.
        @history = @history.slice 0, @currentIndex + 1  # Clear history.
        if @history[@currentIndex] isnt url
          @currentIndex++
          @history.push url
      else  # Go to index.
        @currentIndex = @pendingIndex
        @pendingIndex = -1
        @history[@currentIndex] = url

  loadUrl: (url, options={}) ->
    @pendingIndex = -1
    @webContents._loadUrl url, options

  getUrl: ->
    if @currentIndex is -1
      ''
    else
      @history[@currentIndex]

  stop: ->
    @pendingIndex = -1
    @webContents._stop()

  reload: ->
    @pendingIndex = @currentIndex
    @webContents._loadUrl @getUrl(), {}

  reloadIgnoringCache: ->
    @webContents._reloadIgnoringCache()  # Rely on WebContents to clear cache.
    @reload()

  canGoBack: ->
    @getActiveIndex() > 0

  canGoForward: ->
    @getActiveIndex() < @history.length - 1

  canGoToIndex: (index) ->
    index >=0 and index < @history.length

  canGoToOffset: (offset) ->
    @canGoToIndex @currentIndex + offset

  goBack: ->
    return unless @canGoBack()
    @pendingIndex = @getActiveIndex() - 1
    @webContents._loadUrl @history[@pendingIndex], {}

  goForward: ->
    return unless @canGoForward()
    @pendingIndex = @getActiveIndex() + 1
    @webContents._loadUrl @history[@pendingIndex], {}

  goToIndex: (index) ->
    return unless @canGoToIndex index
    @pendingIndex = index
    @webContents._loadUrl @history[@pendingIndex], {}

  goToOffset: (offset) ->
    return unless @canGoToOffset offset
    @goToIndex @currentIndex + offset

  getActiveIndex: ->
    if @pendingIndex is -1 then @currentIndex else @pendingIndex

module.exports = NavigationController

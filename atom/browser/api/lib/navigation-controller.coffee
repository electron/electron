ipc = require 'ipc'

# The history operation in renderer is redirected to browser.
ipc.on 'ATOM_SHELL_NAVIGATION_CONTROLLER', (event, method, args...) ->
  event.sender[method] args...

ipc.on 'ATOM_SHELL_SYNC_NAVIGATION_CONTROLLER', (event, method, args...) ->
  event.returnValue = event.sender[method] args...

# JavaScript implementation of Chromium's NavigationController.
# Instead of relying on Chromium for history control, we compeletely do history
# control on user land, and only rely on WebContents.loadUrl for navigation.
# This helps us avoid Chromium's various optimizations so we can ensure renderer
# process is restarted everytime.
class NavigationController
  constructor: (@webContents) ->
    @clearHistory()

    # webContents may have already navigated to a page.
    if @webContents._getUrl()
      @currentIndex++
      @history.push @webContents._getUrl()

    @webContents.on 'navigation-entry-commited', (event, url, inPage, replaceEntry) =>
      if @inPageIndex > -1 and not inPage
        # Navigated to a new page, clear in-page mark.
        @inPageIndex = -1
      else if @inPageIndex is -1 and inPage
        # Started in-page navigations.
        @inPageIndex = @currentIndex

      if @pendingIndex >= 0 # Go to index.
        @currentIndex = @pendingIndex
        @pendingIndex = -1
        @history[@currentIndex] = url
      else if replaceEntry # Non-user initialized navigation.
        @history[@currentIndex] = url
      else  # Normal navigation.
        @history = @history.slice 0, @currentIndex + 1  # Clear history.
        currentEntry = @history[@currentIndex]
        if currentEntry?.url isnt url
          @currentIndex++
          @history.push url

  loadUrl: (url, options={}) ->
    @pendingIndex = -1
    @webContents._loadUrl url, options
    @webContents.emit 'load-url', url, options

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

  clearHistory: ->
    @history = []
    @currentIndex = -1
    @pendingIndex = -1
    @inPageIndex = -1

  goBack: ->
    return unless @canGoBack()
    @pendingIndex = @getActiveIndex() - 1
    if @inPageIndex > -1 and @pendingIndex >= @inPageIndex
      @webContents._goBack()
    else
      @webContents._loadUrl @history[@pendingIndex], {}

  goForward: ->
    return unless @canGoForward()
    @pendingIndex = @getActiveIndex() + 1
    if @inPageIndex > -1 and @pendingIndex >= @inPageIndex
      @webContents._goForward()
    else
      @webContents._loadUrl @history[@pendingIndex], {}

  goToIndex: (index) ->
    return unless @canGoToIndex index
    @pendingIndex = index
    @webContents._loadUrl @history[@pendingIndex], {}

  goToOffset: (offset) ->
    return unless @canGoToOffset offset
    pendingIndex = @currentIndex + offset
    if @inPageIndex > -1 and pendingIndex >= @inPageIndex
      @pendingIndex = pendingIndex
      @webContents._goToOffset offset
    else
      @goToIndex pendingIndex

  getActiveIndex: ->
    if @pendingIndex is -1 then @currentIndex else @pendingIndex

  length: ->
    @history.length

module.exports = NavigationController

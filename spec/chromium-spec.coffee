assert = require 'assert'
path = require 'path'

describe 'chromium feature', ->
  fixtures = path.resolve __dirname, 'fixtures'

  describe 'heap snapshot', ->
    it 'does not crash', ->
      process.atomBinding('v8_util').takeHeapSnapshot()

  describe 'sending request of http protocol urls', ->
    it 'does not crash', ->
      $.get 'https://api.github.com/zen'

  describe 'navigator.webkitGetUserMedia', ->
    it 'calls its callbacks', (done) ->
      @timeout 5000
      navigator.webkitGetUserMedia audio: true, video: false,
        -> done()
        -> done()

  describe 'window.open', ->
    it 'returns a BrowserWindow object', ->
      b = window.open 'about:blank', 'test', 'show=no'
      assert.equal b.constructor.name, 'BrowserWindow'
      b.destroy()

  describe 'iframe', ->
    page = path.join fixtures, 'pages', 'change-parent.html'

    beforeEach ->
      global.changedByIframe = false

    it 'can not modify parent by default', (done) ->
      iframe = $('<iframe>')
      iframe.hide()
      iframe.attr 'src', "file://#{page}"
      iframe.appendTo 'body'
      isChanged = ->
        iframe.remove()
        assert.equal global.changedByIframe, false
        done()
      setTimeout isChanged, 30

    it 'can modify parent when sanbox is set to none', (done) ->
      iframe = $('<iframe sandbox="none">')
      iframe.hide()
      iframe.attr 'src', "file://#{page}"
      iframe.appendTo 'body'
      isChanged = ->
        iframe.remove()
        assert.equal global.changedByIframe, true
        done()
      setTimeout isChanged, 30

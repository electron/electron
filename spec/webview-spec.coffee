assert = require 'assert'
path   = require 'path'

describe '<webview> tag', ->
  fixtures = path.join __dirname, 'fixtures'

  webview = null

  beforeEach ->
    webview = new WebView

  afterEach ->
    document.body.removeChild webview

  describe 'src attribute', ->
    it 'specifies the page to load', (done) ->
      webview.addEventListener 'console-message', (e) ->
        assert.equal e.message, 'a'
        done()
      webview.src = "file://#{fixtures}/pages/a.html"
      document.body.appendChild webview

    it 'navigates to new page when changed', (done) ->
      listener = (e) ->
        webview.src = "file://#{fixtures}/pages/b.html"
        webview.addEventListener 'console-message', (e) ->
          assert.equal e.message, 'b'
          done()
        webview.removeEventListener 'did-finish-load', listener
      webview.addEventListener 'did-finish-load', listener
      webview.src = "file://#{fixtures}/pages/a.html"
      document.body.appendChild webview

  describe 'nodeintegration attribute', ->
    it 'inserts no node symbols when not set', (done) ->
      webview.addEventListener 'console-message', (e) ->
        assert.equal e.message, 'undefined undefined undefined'
        done()
      webview.src = "file://#{fixtures}/pages/c.html"
      document.body.appendChild webview

    it 'inserts node symbols when set', (done) ->
      webview.addEventListener 'console-message', (e) ->
        assert.equal e.message, 'function object object'
        done()
      webview.setAttribute 'nodeintegration', 'on'
      webview.src = "file://#{fixtures}/pages/d.html"
      document.body.appendChild webview

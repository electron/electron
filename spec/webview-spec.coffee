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

    it 'loads native modules when navigation happens', (done) ->
      listener = (e) ->
        webview.removeEventListener 'did-finish-load', listener
        listener2 = (e) ->
          assert.equal e.message, 'function'
          done()
        webview.addEventListener 'console-message', listener2
        webview.reload()
      webview.addEventListener 'did-finish-load', listener
      webview.setAttribute 'nodeintegration', 'on'
      webview.src = "file://#{fixtures}/pages/native-module.html"
      document.body.appendChild webview

  describe 'preload attribute', ->
    it 'loads the script before other scripts in window', (done) ->
      listener = (e) ->
        assert.equal e.message, 'function object object'
        webview.removeEventListener 'console-message', listener
        done()
      webview.addEventListener 'console-message', listener
      webview.setAttribute 'preload', "#{fixtures}/module/preload.js"
      webview.src = "file://#{fixtures}/pages/e.html"
      document.body.appendChild webview

    it 'receives ipc message in preload script', (done) ->
      message = 'boom!'
      listener = (e) ->
        assert.equal e.channel, 'pong'
        assert.deepEqual e.args, [message]
        webview.removeEventListener 'ipc-message', listener
        done()
      listener2 = (e) ->
        webview.send 'ping', message
        webview.removeEventListener 'did-finish-load', listener2
      webview.addEventListener 'ipc-message', listener
      webview.addEventListener 'did-finish-load', listener2
      webview.setAttribute 'preload', "#{fixtures}/module/preload-ipc.js"
      webview.src = "file://#{fixtures}/pages/e.html"
      document.body.appendChild webview

  describe 'httpreferrer attribute', ->
    it 'sets the referrer url', (done) ->
      referrer = 'http://github.com/'
      listener = (e) ->
        assert.equal e.message, referrer
        webview.removeEventListener 'console-message', listener
        done()
      webview.addEventListener 'console-message', listener
      webview.setAttribute 'httpreferrer', referrer
      webview.src = "file://#{fixtures}/pages/referrer.html"
      document.body.appendChild webview

  describe 'useragent attribute', ->
    it 'sets the user agent', (done) ->
      referrer = 'Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; AS; rv:11.0) like Gecko'
      listener = (e) ->
        assert.equal e.message, referrer
        webview.removeEventListener 'console-message', listener
        done()
      webview.addEventListener 'console-message', listener
      webview.setAttribute 'useragent', referrer
      webview.src = "file://#{fixtures}/pages/useragent.html"
      document.body.appendChild webview

  describe 'disablewebsecurity attribute', ->
    it 'does not disable web security when not set', (done) ->
      src = "
        <script src='file://#{__dirname}/static/jquery-2.0.3.min.js'></script>
        <script>console.log('ok');</script>
      "
      encoded = btoa(unescape(encodeURIComponent(src)))
      listener = (e) ->
        assert /Not allowed to load local resource/.test(e.message)
        webview.removeEventListener 'console-message', listener
        done()
      webview.addEventListener 'console-message', listener
      webview.src = "data:text/html;base64,#{encoded}"
      document.body.appendChild webview

    it 'disables web security when set', (done) ->
      src = "
        <script src='file://#{__dirname}/static/jquery-2.0.3.min.js'></script>
        <script>console.log('ok');</script>
      "
      encoded = btoa(unescape(encodeURIComponent(src)))
      listener = (e) ->
        assert.equal e.message, 'ok'
        webview.removeEventListener 'console-message', listener
        done()
      webview.addEventListener 'console-message', listener
      webview.setAttribute 'disablewebsecurity', ''
      webview.src = "data:text/html;base64,#{encoded}"
      document.body.appendChild webview

  describe 'new-window event', ->
    it 'emits when window.open is called', (done) ->
      webview.addEventListener 'new-window', (e) ->
        assert.equal e.url, 'http://host'
        assert.equal e.frameName, 'host'
        done()
      webview.src = "file://#{fixtures}/pages/window-open.html"
      document.body.appendChild webview

    it 'emits when link with target is called', (done) ->
      webview.addEventListener 'new-window', (e) ->
        assert.equal e.url, 'http://host/'
        assert.equal e.frameName, 'target'
        done()
      webview.src = "file://#{fixtures}/pages/target-name.html"
      document.body.appendChild webview

  describe 'ipc-message event', ->
    it 'emits when guest sends a ipc message to browser', (done) ->
      webview.addEventListener 'ipc-message', (e) ->
        assert.equal e.channel, 'channel'
        assert.deepEqual e.args, ['arg1', 'arg2']
        done()
      webview.src = "file://#{fixtures}/pages/ipc-message.html"
      webview.setAttribute 'nodeintegration', 'on'
      document.body.appendChild webview

  describe 'page-title-set event', ->
    it 'emits when title is set', (done) ->
      webview.addEventListener 'page-title-set', (e) ->
        assert.equal e.title, 'test'
        assert e.explicitSet
        done()
      webview.src = "file://#{fixtures}/pages/a.html"
      document.body.appendChild webview

  describe 'page-favicon-updated event', ->
    it 'emits when favicon urls are received', (done) ->
      webview.addEventListener 'page-favicon-updated', (e) ->
        assert.equal e.favicons.length, 2
        url =
          if process.platform is 'win32'
            'file:///C:/favicon.png'
          else
            'file:///favicon.png'
        assert.equal e.favicons[0], url
        done()
      webview.src = "file://#{fixtures}/pages/a.html"
      document.body.appendChild webview

  describe '<webview>.reload()', ->
    it 'should emit beforeunload handler', (done) ->
      listener = (e) ->
        assert.equal e.channel, 'onbeforeunload'
        webview.removeEventListener 'ipc-message', listener
        done()
      listener2 = (e) ->
        webview.reload()
        webview.removeEventListener 'did-finish-load', listener2
      webview.addEventListener 'ipc-message', listener
      webview.addEventListener 'did-finish-load', listener2
      webview.setAttribute 'nodeintegration', 'on'
      webview.src = "file://#{fixtures}/pages/beforeunload-false.html"
      document.body.appendChild webview

  describe '<webview>.clearHistory()', ->
    it 'should clear the navigation history', (done) ->
      listener = (e) ->
        assert.equal e.channel, 'history'
        assert.equal e.args[0], 2
        assert webview.canGoBack()
        webview.clearHistory()
        assert not webview.canGoBack()
        webview.removeEventListener 'ipc-message', listener
        done()
      webview.addEventListener 'ipc-message', listener
      webview.setAttribute 'nodeintegration', 'on'      
      webview.src = "file://#{fixtures}/pages/history.html"
      document.body.appendChild webview

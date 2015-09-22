assert = require 'assert'
path   = require 'path'
http   = require 'http'

describe '<webview> tag', ->
  @timeout 10000

  fixtures = path.join __dirname, 'fixtures'

  webview = null
  beforeEach ->
    webview = new WebView
  afterEach ->
    document.body.removeChild(webview) if document.body.contains(webview)

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
        assert.equal e.message, 'undefined undefined undefined undefined'
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

    it 'loads node symbols after POST navigation when set', (done) ->
      webview.addEventListener 'console-message', (e) ->
        assert.equal e.message, 'function object object'
        done()
      webview.setAttribute 'nodeintegration', 'on'
      webview.src = "file://#{fixtures}/pages/post.html"
      document.body.appendChild webview

    # If the test is executed with the debug build on Windows, we will skip it
    # because native modules don't work with the debug build (see issue #2558).
    if process.platform isnt 'win32' or
        process.execPath.toLowerCase().indexOf('\\out\\d\\') is -1
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

    it 'preload script can still use "process" in required modules when nodeintegration is off', (done) ->
      webview.addEventListener 'console-message', (e) ->
        assert.equal e.message, 'object undefined object'
        done()
      webview.setAttribute 'preload', "#{fixtures}/module/preload-node-off.js"
      webview.src = "file://#{fixtures}/api/blank.html"
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

  describe 'partition attribute', ->
    it 'inserts no node symbols when not set', (done) ->
      webview.addEventListener 'console-message', (e) ->
        assert.equal e.message, 'undefined undefined undefined undefined'
        done()
      webview.src = "file://#{fixtures}/pages/c.html"
      webview.partition = 'test1'
      document.body.appendChild webview

    it 'inserts node symbols when set', (done) ->
      webview.addEventListener 'console-message', (e) ->
        assert.equal e.message, 'function object object'
        done()
      webview.setAttribute 'nodeintegration', 'on'
      webview.src = "file://#{fixtures}/pages/d.html"
      webview.partition = 'test2'
      document.body.appendChild webview

    it 'isolates storage for different id', (done) ->
      listener = (e) ->
        assert.equal e.message, " 0"
        webview.removeEventListener 'console-message', listener
        done()
      window.localStorage.setItem 'test', 'one'
      webview.addEventListener 'console-message', listener
      webview.src = "file://#{fixtures}/pages/partition/one.html"
      webview.partition = 'test3'
      document.body.appendChild webview

    it 'uses current session storage when no id is provided', (done) ->
      listener = (e) ->
        assert.equal e.message, "one 1"
        webview.removeEventListener 'console-message', listener
        done()
      window.localStorage.setItem 'test', 'one'
      webview.addEventListener 'console-message', listener
      webview.src = "file://#{fixtures}/pages/partition/one.html"
      document.body.appendChild webview

  describe 'allowpopups attribute', ->
    it 'can not open new window when not set', (done) ->
      listener = (e) ->
        assert.equal e.message, 'null'
        webview.removeEventListener 'console-message', listener
        done()
      webview.addEventListener 'console-message', listener
      webview.src = "file://#{fixtures}/pages/window-open-hide.html"
      document.body.appendChild webview

    it 'can open new window when set', (done) ->
      listener = (e) ->
        assert.equal e.message, 'window'
        webview.removeEventListener 'console-message', listener
        done()
      webview.addEventListener 'console-message', listener
      webview.setAttribute 'allowpopups', 'on'
      webview.src = "file://#{fixtures}/pages/window-open-hide.html"
      document.body.appendChild webview

  describe 'new-window event', ->
    it 'emits when window.open is called', (done) ->
      webview.addEventListener 'new-window', (e) ->
        assert.equal e.url, 'http://host/'
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

  describe 'close event', ->
    it 'should fire when interior page calls window.close', (done) ->
      webview.addEventListener 'close', ->
        done()

      webview.src = "file://#{fixtures}/pages/close.html"
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

  describe 'basic auth', ->
    auth = require 'basic-auth'

    it 'should authenticate with correct credentials', (done) ->
      message = 'Authenticated'
      server = http.createServer (req, res) ->
        credentials = auth(req)
        if credentials.name == 'test' and credentials.pass == 'test'
          res.end(message)
        else
          res.end('failed')
        server.close()
      server.listen 0, '127.0.0.1', ->
        {port} = server.address()
        webview.addEventListener 'ipc-message', (e) ->
          assert.equal e.channel, message
          done()
        webview.src = "file://#{fixtures}/pages/basic-auth.html?port=#{port}"
        webview.setAttribute 'nodeintegration', 'on'
        document.body.appendChild webview

  describe 'dom-ready event', ->
    it 'emits when document is loaded', (done) ->
      server = http.createServer (req) ->
        # Never respond, so the page never finished loading.
      server.listen 0, '127.0.0.1', ->
        {port} = server.address()
        webview.addEventListener 'dom-ready', ->
          done()
        webview.src = "file://#{fixtures}/pages/dom-ready.html?port=#{port}"
        document.body.appendChild webview

  describe 'executeJavaScript', ->
    return unless process.env.TRAVIS is 'true'

    it 'should support user gesture', (done) ->
      listener = (e) ->
        webview.removeEventListener 'enter-html-full-screen', listener
        done()
      listener2 = (e) ->
        jsScript = 'document.getElementsByTagName("video")[0].webkitRequestFullScreen()'
        webview.executeJavaScript jsScript, true
        webview.removeEventListener 'did-finish-load', listener2
      webview.addEventListener 'enter-html-full-screen', listener
      webview.addEventListener 'did-finish-load', listener2
      webview.src = "file://#{fixtures}/pages/fullscreen.html"
      document.body.appendChild webview

  describe 'sendInputEvent', ->
    it 'can send keyboard event', (done) ->
      webview.addEventListener 'ipc-message', (e) ->
        assert.equal e.channel, 'keyup'
        assert.deepEqual e.args, [67, true, false]
        done()
      webview.addEventListener 'dom-ready', ->
        webview.sendInputEvent type: 'keyup', keyCode: 'c', modifiers: ['shift']
      webview.src = "file://#{fixtures}/pages/onkeyup.html"
      webview.setAttribute 'nodeintegration', 'on'
      document.body.appendChild webview

    it 'can send mouse event', (done) ->
      webview.addEventListener 'ipc-message', (e) ->
        assert.equal e.channel, 'mouseup'
        assert.deepEqual e.args, [10, 20, false, true]
        done()
      webview.addEventListener 'dom-ready', ->
        webview.sendInputEvent type: 'mouseup', modifiers: ['ctrl'], x: 10, y: 20
      webview.src = "file://#{fixtures}/pages/onmouseup.html"
      webview.setAttribute 'nodeintegration', 'on'
      document.body.appendChild webview

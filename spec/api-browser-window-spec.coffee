assert = require 'assert'
fs     = require 'fs'
path   = require 'path'
remote = require 'remote'

BrowserWindow = remote.require 'browser-window'

describe 'browser-window module', ->
  fixtures = path.resolve __dirname, 'fixtures'

  describe 'BrowserWindow.close()', ->
    it 'should emit unload handler', (done) ->
      w = new BrowserWindow(show: false)
      w.on 'loading-state-changed', (event, isLoading) ->
        if (!isLoading)
          w.close()
      w.on 'destroyed', ->
        test = path.join(fixtures, 'api', 'unload')
        content = fs.readFileSync(test)
        fs.unlinkSync(test)
        assert.equal String(content), 'unload'
        done()
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'unload.html')

    it 'should emit beforeunload handler', (done) ->
      w = new BrowserWindow(show: false)
      w.on 'onbeforeunload', ->
        w.destroy()
        done()
      w.on 'loading-state-changed', (event, isLoading) ->
        if (!isLoading)
          w.close()
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'beforeunload-false.html')

  describe 'window.close()', ->
    xit 'should emit unload handler', (done) ->
      w = new BrowserWindow(show: false)
      w.on 'closed', ->
        test = path.join(fixtures, 'api', 'close')
        content = fs.readFileSync(test)
        fs.unlinkSync(test)
        assert.equal String(content), 'close'
        done()
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'close.html')

    it 'should emit beforeunload handler', (done) ->
      w = new BrowserWindow(show: false)
      w.on 'onbeforeunload', ->
        w.destroy()
        done()
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'close-beforeunload-false.html')

  describe 'BrowserWindow.loadUrl(url)', ->
    it 'should emit loading-state-changed event', (done) ->
      w = new BrowserWindow(show: false)
      count = 0
      w.on 'loading-state-changed', (event, isLoading) ->
        if count == 0
          assert.equal isLoading, true
        else if count == 1
          assert.equal isLoading, false
          w.close()
          done()
        else
          w.close()
          assert false

        ++count
      w.loadUrl 'about:blank'

  describe 'BrowserWindow.focus()', ->
    it 'does not make the window become visible', ->
      w = new BrowserWindow(show: false)
      assert.equal w.isVisible(), false
      w.focus()
      assert.equal w.isVisible(), false
      w.close()

  describe 'BrowserWindow.capturePage(rect, callback)', ->
    it 'calls the callback with a Buffer', (done) ->
      w = new BrowserWindow(show: false)
      w.capturePage {x: 0, y: 0, width: 100, height: 100}, (image) ->
        assert.equal image.constructor.name, 'Buffer'
        done()
        w.close()

  describe 'beforeunload handler', ->
    it 'returning true would not prevent close', (done) ->
      w = new BrowserWindow(show: false)
      w.on 'closed', ->
        done()
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'close-beforeunload-true.html')

    it 'returning non-empty string would not prevent close', (done) ->
      w = new BrowserWindow(show: false)
      w.on 'closed', ->
        done()
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'close-beforeunload-string.html')

    it 'returning false would prevent close', (done) ->
      w = new BrowserWindow(show: false)
      w.on 'onbeforeunload', ->
        w.destroy()
        done()
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'close-beforeunload-false.html')

    it 'returning empty string would prevent close', (done) ->
      w = new BrowserWindow(show: false)
      w.on 'onbeforeunload', ->
        w.destroy()
        done()
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'close-beforeunload-empty-string.html')

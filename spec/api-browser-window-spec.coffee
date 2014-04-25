assert = require 'assert'
fs     = require 'fs'
path   = require 'path'
remote = require 'remote'

BrowserWindow = remote.require 'browser-window'

describe 'browser-window module', ->
  fixtures = path.resolve __dirname, 'fixtures'

  w = null
  beforeEach ->
    w.destroy() if w?
    w = new BrowserWindow(show: false)
  afterEach ->
    w.destroy() if w?
    w = null

  describe 'BrowserWindow.close()', ->
    it 'should emit unload handler', (done) ->
      w.webContents.on 'did-finish-load', ->
        w.close()
      w.on 'closed', ->
        test = path.join(fixtures, 'api', 'unload')
        content = fs.readFileSync(test)
        fs.unlinkSync(test)
        assert.equal String(content), 'unload'
        done()
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'unload.html')

    it 'should emit beforeunload handler', (done) ->
      w.on 'onbeforeunload', ->
        done()
      w.webContents.on 'did-finish-load', ->
        w.close()
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'beforeunload-false.html')

  describe 'window.close()', ->
    it 'should emit unload handler', (done) ->
      w.on 'closed', ->
        test = path.join(fixtures, 'api', 'close')
        content = fs.readFileSync(test)
        fs.unlinkSync(test)
        assert.equal String(content), 'close'
        done()
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'close.html')

    it 'should emit beforeunload handler', (done) ->
      w.on 'onbeforeunload', ->
        done()
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'close-beforeunload-false.html')

  describe 'BrowserWindow.loadUrl(url)', ->
    it 'should emit did-start-loading event', (done) ->
      w.webContents.on 'did-start-loading', ->
        done()
      w.loadUrl 'about:blank'

  describe 'BrowserWindow.focus()', ->
    it 'does not make the window become visible', ->
      assert.equal w.isVisible(), false
      w.focus()
      assert.equal w.isVisible(), false

  describe 'BrowserWindow.capturePage(rect, callback)', ->
    it 'calls the callback with a Buffer', (done) ->
      w.capturePage {x: 0, y: 0, width: 100, height: 100}, (image) ->
        assert.equal image.constructor.name, 'Buffer'
        done()

  describe 'beforeunload handler', ->
    it 'returning true would not prevent close', (done) ->
      w.on 'closed', ->
        done()
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'close-beforeunload-true.html')

    it 'returning non-empty string would not prevent close', (done) ->
      w.on 'closed', ->
        done()
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'close-beforeunload-string.html')

    it 'returning false would prevent close', (done) ->
      w.on 'onbeforeunload', ->
        done()
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'close-beforeunload-false.html')

    it 'returning empty string would prevent close', (done) ->
      w.on 'onbeforeunload', ->
        done()
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'close-beforeunload-empty-string.html')

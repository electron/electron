assert = require 'assert'
fs     = require 'fs'
path   = require 'path'
remote = require 'remote'

BrowserWindow = remote.require 'browser-window'

isCI = remote.process.argv[1] == '--ci'

describe 'browser-window module', ->
  fixtures = path.resolve __dirname, 'fixtures'

  w = null
  beforeEach ->
    w.destroy() if w?
    w = new BrowserWindow(show: false, width: 400, height: 400)
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

    it 'should emit did-fail-load event', (done) ->
      w.webContents.on 'did-fail-load', ->
        done()
      w.loadUrl 'file://a.txt'

  describe 'BrowserWindow.show()', ->
    it 'should focus on window', ->
      return if isCI
      w.show()
      assert w.isFocused()

  describe 'BrowserWindow.showInactive()', ->
    it 'should not focus on window', ->
      w.showInactive()
      assert !w.isFocused()

  describe 'BrowserWindow.focus()', ->
    it 'does not make the window become visible', ->
      assert.equal w.isVisible(), false
      w.focus()
      assert.equal w.isVisible(), false

  describe 'BrowserWindow.capturePage(rect, callback)', ->
    it 'calls the callback with a Buffer', (done) ->
      w.capturePage {x: 0, y: 0, width: 100, height: 100}, (image) ->
        assert.equal image.isEmpty(), true
        done()

  describe 'BrowserWindow.setSize(width, height)', ->
    it 'sets the window size', ->
      size = [400, 400]
      w.setSize size[0], size[1]
      after = w.getSize()
      assert.equal after[0], size[0]
      assert.equal after[1], size[1]

  describe 'BrowserWindow.setContentSize(width, height)', ->
    it 'sets the content size', ->
      size = [400, 400]
      w.setContentSize size[0], size[1]
      after = w.getContentSize()
      assert.equal after[0], size[0]
      assert.equal after[1], size[1]

  describe 'BrowserWindow.fromId(id)', ->
    it 'returns the window with id', ->
      assert.equal w.id, BrowserWindow.fromId(w.id).id

  describe '"use-content-size" option', ->
    it 'make window created with content size when used', ->
      w.destroy()
      w = new BrowserWindow(show: false, width: 400, height: 400, 'use-content-size': true)
      contentSize = w.getContentSize()
      assert.equal contentSize[0], 400
      assert.equal contentSize[1], 400

    it 'make window created with window size when not used', ->
      size = w.getSize()
      assert.equal size[0], 400
      assert.equal size[1], 400

  describe '"enable-larger-than-screen" option', ->
    return if process.platform is 'linux'

    beforeEach ->
      w.destroy()
      w = new BrowserWindow(show: true, width: 400, height: 400, 'enable-larger-than-screen': true)

    it 'can move the window out of screen', ->
      w.setPosition -10, -10
      after = w.getPosition()
      assert.equal after[0], -10
      assert.equal after[1], -10

    it 'can set the window larger than screen', ->
      size = require('screen').getPrimaryDisplay().size
      size.width += 100
      size.height += 100
      w.setSize size.width, size.height
      after = w.getSize()
      assert.equal after[0], size.width
      assert.equal after[1], size.height

  describe '"preload" options', ->
    it 'loads the script before other scripts in window', (done) ->
      preload = path.join fixtures, 'module', 'set-global.js'
      remote.require('ipc').once 'preload', (event, test) ->
        assert.equal(test, 'preload')
        done()
      w.destroy()
      w = new BrowserWindow(show: false, width: 400, height: 400, preload: preload)
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'preload.html')

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

  describe 'new-window event', ->
    it 'emits when window.open is called', (done) ->
      w.webContents.once 'new-window', (e, url, frameName) ->
        e.preventDefault()
        assert.equal url, 'http://host'
        assert.equal frameName, 'host'
        done()
      w.loadUrl "file://#{fixtures}/pages/window-open.html"

    it 'emits when link with target is called', (done) ->
      w.webContents.once 'new-window', (e, url, frameName) ->
        e.preventDefault()
        assert.equal url, 'http://host/'
        assert.equal frameName, 'target'
        done()
      w.loadUrl "file://#{fixtures}/pages/target-name.html"

  describe 'maximize event', ->
    return if isCI and process.platform is 'linux'
    it 'emits when window is maximized', (done) ->
      @timeout 10000
      w.once 'maximize', -> done()
      w.show()
      w.maximize()

  describe 'unmaximize event', ->
    return if isCI and process.platform is 'linux'
    it 'emits when window is unmaximized', (done) ->
      @timeout 10000
      w.once 'unmaximize', -> done()
      w.show()
      w.maximize()
      w.unmaximize()

  describe 'minimize event', ->
    return if isCI and process.platform is 'linux'
    it 'emits when window is minimized', (done) ->
      @timeout 10000
      w.once 'minimize', -> done()
      w.show()
      w.minimize()

  describe 'will-navigate event', ->
    it 'emits when user starts a navigation', (done) ->
      w.webContents.on 'will-navigate', (event, url) ->
        event.preventDefault()
        assert.equal url, 'https://www.github.com/'
        done()
      w.loadUrl "file://#{fixtures}/pages/will-navigate.html"

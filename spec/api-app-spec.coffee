assert = require 'assert'
remote = require 'remote'
app = remote.require 'app'
BrowserWindow = remote.require 'browser-window'

describe 'app module', ->
  describe 'app.getVersion()', ->
    it 'returns the version field of package.json', ->
      assert.equal app.getVersion(), '0.1.0'

  describe 'app.setVersion(version)', ->
    it 'overrides the version', ->
      assert.equal app.getVersion(), '0.1.0'
      app.setVersion 'test-version'
      assert.equal app.getVersion(), 'test-version'
      app.setVersion '0.1.0'

  describe 'app.getName()', ->
    it 'returns the name field of package.json', ->
      assert.equal app.getName(), 'Electron Test'

  describe 'app.setName(name)', ->
    it 'overrides the name', ->
      assert.equal app.getName(), 'Electron Test'
      app.setName 'test-name'
      assert.equal app.getName(), 'test-name'
      app.setName 'Electron Test'

  describe 'app.getLocale()', ->
    it 'should not be empty', ->
      assert.notEqual app.getLocale(), ''

  describe 'BrowserWindow events', ->
    w = null
    afterEach ->
      w.destroy() if w?
      w = null

    it 'should emit browser-window-focus event when window is focused', (done) ->
      app.once 'browser-window-focus', (e, window) ->
        assert.equal w.id, window.id
        done()
      w = new BrowserWindow(show: false)
      w.emit 'focus'

    it 'should emit browser-window-blur event when window is blured', (done) ->
      app.once 'browser-window-blur', (e, window) ->
        assert.equal w.id, window.id
        done()
      w = new BrowserWindow(show: false)
      w.emit 'blur'

    it 'should emit browser-window-created event when window is created', (done) ->
      app.once 'browser-window-created', (e, window) ->
        setImmediate ->
          assert.equal w.id, window.id
          done()
      w = new BrowserWindow(show: false)
      w.emit 'blur'

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

  describe 'focus/blur event', ->
    w = null
    beforeEach ->
      w.destroy() if w?
      w = new BrowserWindow(show: false, width: 400, height: 400)
    afterEach ->
      w.destroy() if w?
      w = null
    it 'should emit focus event', (done) ->
      app.once 'browser-window-blur', (e, window) ->
        assert.equal w.id, window.id
        done()
      app.once 'browser-window-focus', (e, window) ->
        assert.equal w.id, window.id
        w.emit 'blur'
      w.emit 'focus'

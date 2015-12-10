assert = require 'assert'
ChildProcess = require 'child_process'
path = require 'path'
{remote} = require 'electron'
{app, BrowserWindow} = remote.require 'electron'

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

  describe 'app.exit(exitCode)', ->
    appProcess = null
    afterEach ->
      appProcess?.kill()

    it 'emits a process exit event with the code', (done) ->
      appPath = path.join(__dirname, 'fixtures', 'api', 'quit-app')
      electronPath = remote.getGlobal('process').execPath
      appProcess = ChildProcess.spawn(electronPath, [appPath])

      output = ''
      appProcess.stdout.on 'data', (data) -> output += data
      appProcess.on 'close', (code) ->
        assert.notEqual output.indexOf('Exit event with code: 123'), -1
        assert.equal code, 123
        done()

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

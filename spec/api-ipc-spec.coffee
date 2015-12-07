assert = require 'assert'
path   = require 'path'

{ipcRenderer, remote} = require 'electron'
{ipcMain, BrowserWindow} = remote.require 'electron'

comparePaths = (path1, path2) ->
  if process.platform is 'win32'
    # Paths in Windows are case insensitive.
    path1 = path1.toLowerCase()
    path2 = path2.toLowerCase()
  assert.equal path1, path2

describe 'ipc module', ->
  fixtures = path.join __dirname, 'fixtures'

  describe 'remote.require', ->
    it 'should returns same object for the same module', ->
      dialog1 = remote.require 'electron'
      dialog2 = remote.require 'electron'
      assert.equal dialog1, dialog2

    it 'should work when object contains id property', ->
      a = remote.require path.join(fixtures, 'module', 'id.js')
      assert.equal a.id, 1127

    it 'should search module from the user app', ->
      comparePaths path.normalize(remote.process.mainModule.filename), path.resolve(__dirname, 'static', 'main.js')
      comparePaths path.normalize(remote.process.mainModule.paths[0]), path.resolve(__dirname, 'static', 'node_modules')

  describe 'remote.createFunctionWithReturnValue', ->
    it 'should be called in browser synchronously', ->
      buf = new Buffer('test')
      call = remote.require path.join(fixtures, 'module', 'call.js')
      result = call.call remote.createFunctionWithReturnValue(buf)
      assert.equal result.constructor.name, 'Buffer'

  describe 'remote object in renderer', ->
    it 'can change its properties', ->
      property = remote.require path.join(fixtures, 'module', 'property.js')
      assert.equal property.property, 1127
      property.property = 1007
      assert.equal property.property, 1007
      property2 = remote.require path.join(fixtures, 'module', 'property.js')
      assert.equal property2.property, 1007

      # Restore.
      property.property = 1127

    it 'can construct an object from its member', ->
      call = remote.require path.join(fixtures, 'module', 'call.js')
      obj = new call.constructor
      assert.equal obj.test, 'test'

  describe 'remote value in browser', ->
    it 'keeps its constructor name for objects', ->
      buf = new Buffer('test')
      print_name = remote.require path.join(fixtures, 'module', 'print_name.js')
      assert.equal print_name.print(buf), 'Buffer'

  describe 'remote promise', ->
    it 'can be used as promise in each side', (done) ->
      promise = remote.require path.join(fixtures, 'module', 'promise.js')
      promise.twicePromise(Promise.resolve(1234))
        .then (value) =>
          assert.equal value, 2468
          done()

  describe 'ipc.sender.send', ->
    it 'should work when sending an object containing id property', (done) ->
      obj = id: 1, name: 'ly'
      ipcRenderer.once 'message', (event, message) ->
        assert.deepEqual message, obj
        done()
      ipcRenderer.send 'message', obj

  describe 'ipc.sendSync', ->
    it 'can be replied by setting event.returnValue', ->
      msg = ipcRenderer.sendSync 'echo', 'test'
      assert.equal msg, 'test'

    it 'does not crash when reply is not sent and browser is destroyed', (done) ->
      @timeout 10000
      w = new BrowserWindow(show: false)
      ipcMain.once 'send-sync-message', (event) ->
        event.returnValue = null
        w.destroy()
        done()
      w.loadURL 'file://' + path.join(fixtures, 'api', 'send-sync-message.html')

  describe 'remote listeners', ->
    w = null
    afterEach ->
      w.destroy()

    it 'can be added and removed correctly', ->
      w = new BrowserWindow(show: false)
      listener = ->
      w.on 'test', listener
      assert.equal w.listenerCount('test'), 1
      w.removeListener 'test', listener
      assert.equal w.listenerCount('test'), 0

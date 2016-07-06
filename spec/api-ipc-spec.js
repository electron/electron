'use strict'

const assert = require('assert')
const path = require('path')

const {ipcRenderer, remote} = require('electron')
const {ipcMain, webContents, BrowserWindow} = remote

const comparePaths = function (path1, path2) {
  if (process.platform === 'win32') {
    path1 = path1.toLowerCase()
    path2 = path2.toLowerCase()
  }
  assert.equal(path1, path2)
}

describe('ipc module', function () {
  var fixtures = path.join(__dirname, 'fixtures')

  describe('remote.require', function () {
    it('should returns same object for the same module', function () {
      var dialog1 = remote.require('electron')
      var dialog2 = remote.require('electron')
      assert.equal(dialog1, dialog2)
    })

    it('should work when object contains id property', function () {
      var a = remote.require(path.join(fixtures, 'module', 'id.js'))
      assert.equal(a.id, 1127)
    })

    it('should work when object has no prototype', function () {
      var a = remote.require(path.join(fixtures, 'module', 'no-prototype.js'))
      assert.equal(a.foo.bar, 'baz')
      assert.equal(a.foo.baz, false)
      assert.equal(a.bar, 1234)
      assert.equal(a.getConstructorName(Object.create(null)), 'Object')
      assert.equal(a.getConstructorName(new (class {})), '')
    })

    it('should search module from the user app', function () {
      comparePaths(path.normalize(remote.process.mainModule.filename), path.resolve(__dirname, 'static', 'main.js'))
      comparePaths(path.normalize(remote.process.mainModule.paths[0]), path.resolve(__dirname, 'static', 'node_modules'))
    })
  })

  describe('remote.createFunctionWithReturnValue', function () {
    it('should be called in browser synchronously', function () {
      var buf = new Buffer('test')
      var call = remote.require(path.join(fixtures, 'module', 'call.js'))
      var result = call.call(remote.createFunctionWithReturnValue(buf))
      assert.equal(result.constructor.name, 'Buffer')
    })
  })

  describe('remote object in renderer', function () {
    it('can change its properties', function () {
      var property = remote.require(path.join(fixtures, 'module', 'property.js'))
      assert.equal(property.property, 1127)
      property.property = 1007
      assert.equal(property.property, 1007)
      var property2 = remote.require(path.join(fixtures, 'module', 'property.js'))
      assert.equal(property2.property, 1007)
      property.property = 1127
    })

    it('can construct an object from its member', function () {
      var call = remote.require(path.join(fixtures, 'module', 'call.js'))
      var obj = new call.constructor()
      assert.equal(obj.test, 'test')
    })

    it('can reassign and delete its member functions', function () {
      var remoteFunctions = remote.require(path.join(fixtures, 'module', 'function.js'))
      assert.equal(remoteFunctions.aFunction(), 1127)

      remoteFunctions.aFunction = function () { return 1234 }
      assert.equal(remoteFunctions.aFunction(), 1234)

      assert.equal(delete remoteFunctions.aFunction, true)
    })

    it('is referenced by its members', function () {
      let stringify = remote.getGlobal('JSON').stringify
      global.gc()
      stringify({})
    })
  })

  describe('remote value in browser', function () {
    var print = path.join(fixtures, 'module', 'print_name.js')

    it('keeps its constructor name for objects', function () {
      var buf = new Buffer('test')
      var printName = remote.require(print)
      assert.equal(printName.print(buf), 'Buffer')
    })

    it('supports instanceof Date', function () {
      var now = new Date()
      var printName = remote.require(print)
      assert.equal(printName.print(now), 'Date')
      assert.deepEqual(printName.echo(now), now)
    })
  })

  describe('remote promise', function () {
    it('can be used as promise in each side', function (done) {
      var promise = remote.require(path.join(fixtures, 'module', 'promise.js'))
      promise.twicePromise(Promise.resolve(1234)).then(function (value) {
        assert.equal(value, 2468)
        done()
      })
    })

    it('handles rejections via catch(onRejected)', function (done) {
      var promise = remote.require(path.join(fixtures, 'module', 'rejected-promise.js'))
      promise.reject(Promise.resolve(1234)).catch(function (error) {
        assert.equal(error.message, 'rejected')
        done()
      })
    })

    it('handles rejections via then(onFulfilled, onRejected)', function (done) {
      var promise = remote.require(path.join(fixtures, 'module', 'rejected-promise.js'))
      promise.reject(Promise.resolve(1234)).then(function () {}, function (error) {
        assert.equal(error.message, 'rejected')
        done()
      })
    })

    it('does not emit unhandled rejection events in the main process', function (done) {
      remote.process.once('unhandledRejection', function (reason) {
        done(reason)
      })

      var promise = remote.require(path.join(fixtures, 'module', 'unhandled-rejection.js'))
      promise.reject().then(function () {
        done(new Error('Promise was not rejected'))
      }).catch(function (error) {
        assert.equal(error.message, 'rejected')
        done()
      })
    })

    it('emits unhandled rejection events in the renderer process', function (done) {
      window.addEventListener('unhandledrejection', function (event) {
        event.preventDefault()
        assert.equal(event.reason.message, 'rejected')
        done()
      })

      var promise = remote.require(path.join(fixtures, 'module', 'unhandled-rejection.js'))
      promise.reject().then(function () {
        done(new Error('Promise was not rejected'))
      })
    })
  })

  describe('remote webContents', function () {
    it('can return same object with different getters', function () {
      var contents1 = remote.getCurrentWindow().webContents
      var contents2 = remote.getCurrentWebContents()
      assert(contents1 === contents2)
    })
  })

  describe('remote class', function () {
    let cl = remote.require(path.join(fixtures, 'module', 'class.js'))
    let base = cl.base
    let derived = cl.derived

    it('can get methods', function () {
      assert.equal(base.method(), 'method')
    })

    it('can get properties', function () {
      assert.equal(base.readonly, 'readonly')
    })

    it('can change properties', function () {
      assert.equal(base.value, 'old')
      base.value = 'new'
      assert.equal(base.value, 'new')
      base.value = 'old'
    })

    it('has unenumerable methods', function () {
      assert(!base.hasOwnProperty('method'))
      assert(Object.getPrototypeOf(base).hasOwnProperty('method'))
    })

    it('keeps prototype chain in derived class', function () {
      assert.equal(derived.method(), 'method')
      assert.equal(derived.readonly, 'readonly')
      assert(!derived.hasOwnProperty('method'))
      let proto = Object.getPrototypeOf(derived)
      assert(!proto.hasOwnProperty('method'))
      assert(Object.getPrototypeOf(proto).hasOwnProperty('method'))
    })

    it('is referenced by methods in prototype chain', function () {
      let method = derived.method
      derived = null
      global.gc()
      assert.equal(method(), 'method')
    })
  })

  describe('ipc.sender.send', function () {
    it('should work when sending an object containing id property', function (done) {
      var obj = {
        id: 1,
        name: 'ly'
      }
      ipcRenderer.once('message', function (event, message) {
        assert.deepEqual(message, obj)
        done()
      })
      ipcRenderer.send('message', obj)
    })

    it('can send instance of Date', function (done) {
      const currentDate = new Date()
      ipcRenderer.once('message', function (event, value) {
        assert.equal(value, currentDate.toISOString())
        done()
      })
      ipcRenderer.send('message', currentDate)
    })
  })

  describe('ipc.sendSync', function () {
    afterEach(function () {
      ipcMain.removeAllListeners('send-sync-message')
    })

    it('can be replied by setting event.returnValue', function () {
      var msg = ipcRenderer.sendSync('echo', 'test')
      assert.equal(msg, 'test')
    })

    it('does not crash when reply is not sent and browser is destroyed', function (done) {
      this.timeout(10000)

      var w = new BrowserWindow({
        show: false
      })
      ipcMain.once('send-sync-message', function (event) {
        event.returnValue = null
        w.destroy()
        done()
      })
      w.loadURL('file://' + path.join(fixtures, 'api', 'send-sync-message.html'))
    })

    it('does not crash when reply is sent by multiple listeners', function (done) {
      var w = new BrowserWindow({
        show: false
      })
      ipcMain.on('send-sync-message', function (event) {
        event.returnValue = null
      })
      ipcMain.on('send-sync-message', function (event) {
        event.returnValue = null
        w.destroy()
        done()
      })
      w.loadURL('file://' + path.join(fixtures, 'api', 'send-sync-message.html'))
    })
  })

  describe('ipcRenderer.sendTo', function () {
    let contents = null
    beforeEach(function () {
      contents = webContents.create({})
    })
    afterEach(function () {
      ipcRenderer.removeAllListeners('pong')
      contents.destroy()
      contents = null
    })

    it('sends message to WebContents', function (done) {
      const webContentsId = remote.getCurrentWebContents().id
      ipcRenderer.once('pong', function (event, id) {
        assert.equal(webContentsId, id)
        done()
      })
      contents.once('did-finish-load', function () {
        ipcRenderer.sendTo(contents.id, 'ping', webContentsId)
      })
      contents.loadURL('file://' + path.join(fixtures, 'pages', 'ping-pong.html'))
    })
  })

  describe('remote listeners', function () {
    var w = null

    afterEach(function () {
      w.destroy()
    })

    it('can be added and removed correctly', function () {
      w = new BrowserWindow({
        show: false
      })
      var listener = function () {}
      w.on('test', listener)
      assert.equal(w.listenerCount('test'), 1)
      w.removeListener('test', listener)
      assert.equal(w.listenerCount('test'), 0)
    })
  })
})

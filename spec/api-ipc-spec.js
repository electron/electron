'use strict'

const assert = require('assert')
const http = require('http')
const path = require('path')
const {closeWindow} = require('./window-helpers')

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

  var w = null

  afterEach(function () {
    return closeWindow(w).then(function () { w = null })
  })

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
      assert.equal(a.foo.constructor.name, '')
      assert.equal(a.foo.bar, 'baz')
      assert.equal(a.foo.baz, false)
      assert.equal(a.bar, 1234)
      assert.equal(a.anonymous.constructor.name, '')
      assert.equal(a.getConstructorName(Object.create(null)), '')
      assert.equal(a.getConstructorName(new (class {})), '')
    })

    it('should search module from the user app', function () {
      comparePaths(path.normalize(remote.process.mainModule.filename), path.resolve(__dirname, 'static', 'main.js'))
      comparePaths(path.normalize(remote.process.mainModule.paths[0]), path.resolve(__dirname, 'static', 'node_modules'))
    })

    it('should work with function properties', function () {
      var a = remote.require(path.join(fixtures, 'module', 'export-function-with-properties.js'))
      assert.equal(typeof a, 'function')
      assert.equal(a.bar, 'baz')

      a = remote.require(path.join(fixtures, 'module', 'function-with-properties.js'))
      assert.equal(typeof a, 'object')
      assert.equal(a.foo(), 'hello')
      assert.equal(a.foo.bar, 'baz')
      assert.equal(a.foo.nested.prop, 'yes')
      assert.equal(a.foo.method1(), 'world')
      assert.equal(a.foo.method1.prop1(), 123)

      assert.ok(Object.keys(a.foo).includes('bar'))
      assert.ok(Object.keys(a.foo).includes('nested'))
      assert.ok(Object.keys(a.foo).includes('method1'))
    })

    it('should work with static class members', function () {
      var a = remote.require(path.join(fixtures, 'module', 'remote-static.js'))
      assert.equal(typeof a.Foo, 'function')
      assert.equal(a.Foo.foo(), 3)
      assert.equal(a.Foo.bar, 'baz')

      var foo = new a.Foo()
      assert.equal(foo.baz(), 123)
    })

    it('handles circular references in arrays and objects', function () {
      var a = remote.require(path.join(fixtures, 'module', 'circular.js'))

      var arrayA = ['foo']
      var arrayB = [arrayA, 'bar']
      arrayA.push(arrayB)
      assert.deepEqual(a.returnArgs(arrayA, arrayB), [
        ['foo', [null, 'bar']],
        [['foo', null], 'bar']
      ])

      var objectA = {foo: 'bar'}
      var objectB = {baz: objectA}
      objectA.objectB = objectB
      assert.deepEqual(a.returnArgs(objectA, objectB), [
        {foo: 'bar', objectB: {baz: null}},
        {baz: {foo: 'bar', objectB: null}}
      ])

      arrayA = [1, 2, 3]
      assert.deepEqual(a.returnArgs({foo: arrayA}, {bar: arrayA}), [
        {foo: [1, 2, 3]},
        {bar: [1, 2, 3]}
      ])

      objectA = {foo: 'bar'}
      assert.deepEqual(a.returnArgs({foo: objectA}, {bar: objectA}), [
        {foo: {foo: 'bar'}},
        {bar: {foo: 'bar'}}
      ])

      arrayA = []
      arrayA.push(arrayA)
      assert.deepEqual(a.returnArgs(arrayA), [
        [null]
      ])

      objectA = {}
      objectA.foo = objectA
      objectA.bar = 'baz'
      assert.deepEqual(a.returnArgs(objectA), [
        {foo: null, bar: 'baz'}
      ])

      objectA = {}
      objectA.foo = {bar: objectA}
      objectA.bar = 'baz'
      assert.deepEqual(a.returnArgs(objectA), [
        {foo: {bar: null}, bar: 'baz'}
      ])
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
      assert.equal(property.getFunctionProperty(), 'foo-browser')
      property.func.property = 'bar'
      assert.equal(property.getFunctionProperty(), 'bar-browser')

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
    const print = path.join(fixtures, 'module', 'print_name.js')
    const printName = remote.require(print)

    it('keeps its constructor name for objects', function () {
      const buf = new Buffer('test')
      assert.equal(printName.print(buf), 'Buffer')
    })

    it('supports instanceof Date', function () {
      const now = new Date()
      assert.equal(printName.print(now), 'Date')
      assert.deepEqual(printName.echo(now), now)
    })

    it('supports instanceof Buffer', function () {
      const buffer = Buffer.from('test')
      assert.ok(buffer.equals(printName.echo(buffer)))

      const objectWithBuffer = {a: 'foo', b: Buffer.from('bar')}
      assert.ok(objectWithBuffer.b.equals(printName.echo(objectWithBuffer).b))

      const arrayWithBuffer = [1, 2, Buffer.from('baz')]
      assert.ok(arrayWithBuffer[2].equals(printName.echo(arrayWithBuffer)[2]))
    })

    it('supports TypedArray', function () {
      const values = [1, 2, 3, 4]
      assert.deepEqual(printName.typedArray(values), values)

      const int16values = new Int16Array([1, 2, 3, 4])
      assert.deepEqual(printName.typedArray(int16values), int16values)
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

    it('can send instances of Date', function (done) {
      const currentDate = new Date()
      ipcRenderer.once('message', function (event, value) {
        assert.equal(value, currentDate.toISOString())
        done()
      })
      ipcRenderer.send('message', currentDate)
    })

    it('can send instances of Buffer', function (done) {
      const buffer = Buffer.from('hello')
      ipcRenderer.once('message', function (event, message) {
        assert.ok(buffer.equals(message))
        done()
      })
      ipcRenderer.send('message', buffer)
    })

    it('can send objects with DOM class prototypes', function (done) {
      ipcRenderer.once('message', function (event, value) {
        assert.equal(value.protocol, 'file:')
        assert.equal(value.hostname, '')
        done()
      })
      ipcRenderer.send('message', document.location)
    })

    it('can send Electron API objects', function (done) {
      const webContents = remote.getCurrentWebContents()
      ipcRenderer.once('message', function (event, value) {
        assert.deepEqual(value.browserWindowOptions, webContents.browserWindowOptions)
        done()
      })
      ipcRenderer.send('message', webContents)
    })

    it('does not crash on external objects (regression)', function (done) {
      const request = http.request({port: 5000, hostname: '127.0.0.1', method: 'GET', path: '/'})
      const stream = request.agent.sockets['127.0.0.1:5000:'][0]._handle._externalStream
      request.on('error', function () {})
      ipcRenderer.once('message', function (event, requestValue, externalStreamValue) {
        assert.equal(requestValue.method, 'GET')
        assert.equal(requestValue.path, '/')
        assert.equal(externalStreamValue, null)
        done()
      })

      ipcRenderer.send('message', request, stream)
    })

    it('can send objects that both reference the same object', function (done) {
      const child = {hello: 'world'}
      const foo = {name: 'foo', child: child}
      const bar = {name: 'bar', child: child}
      const array = [foo, bar]

      ipcRenderer.once('message', function (event, arrayValue, fooValue, barValue, childValue) {
        assert.deepEqual(arrayValue, array)
        assert.deepEqual(fooValue, foo)
        assert.deepEqual(barValue, bar)
        assert.deepEqual(childValue, child)
        done()
      })
      ipcRenderer.send('message', array, foo, bar, child)
    })

    it('inserts null for cyclic references', function (done) {
      const array = [5]
      array.push(array)

      const child = {hello: 'world'}
      child.child = child

      ipcRenderer.once('message', function (event, arrayValue, childValue) {
        assert.equal(arrayValue[0], 5)
        assert.equal(arrayValue[1], null)

        assert.equal(childValue.hello, 'world')
        assert.equal(childValue.child, null)

        done()
      })
      ipcRenderer.send('message', array, child)
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

      w = new BrowserWindow({
        show: false
      })
      ipcMain.once('send-sync-message', function (event) {
        event.returnValue = null
        done()
      })
      w.loadURL('file://' + path.join(fixtures, 'api', 'send-sync-message.html'))
    })

    it('does not crash when reply is sent by multiple listeners', function (done) {
      w = new BrowserWindow({
        show: false
      })
      ipcMain.on('send-sync-message', function (event) {
        event.returnValue = null
      })
      ipcMain.on('send-sync-message', function (event) {
        event.returnValue = null
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

  describe('remote objects registry', function () {
    it('does not dereference until the render view is deleted (regression)', function (done) {
      w = new BrowserWindow({
        show: true
      })

      ipcMain.once('error-message', (event, message) => {
        assert(message.startsWith('Cannot read property \'object\' of undefined'), message)
        done()
      })

      w.loadURL('file://' + path.join(fixtures, 'api', 'render-view-deleted.html'))
    })
  })
})

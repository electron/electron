'use strict'

const assert = require('assert')
const path = require('path')
const {closeWindow} = require('./window-helpers')

const {remote} = require('electron')

const comparePaths = (path1, path2) => {
  if (process.platform === 'win32') {
    path1 = path1.toLowerCase()
    path2 = path2.toLowerCase()
  }
  assert.equal(path1, path2)
}

describe('remote module', () => {
  const fixtures = path.join(__dirname, 'fixtures')

  let w = null

  afterEach(() => closeWindow(w).then(() => { w = null }))

  describe('remote.require', () => {
    it('should returns same object for the same module', () => {
      const dialog1 = remote.require('electron')
      const dialog2 = remote.require('electron')
      assert.equal(dialog1, dialog2)
    })

    it('should work when object contains id property', () => {
      const a = remote.require(path.join(fixtures, 'module', 'id.js'))
      assert.equal(a.id, 1127)
    })

    it('should work when object has no prototype', () => {
      const a = remote.require(path.join(fixtures, 'module', 'no-prototype.js'))
      assert.equal(a.foo.constructor.name, '')
      assert.equal(a.foo.bar, 'baz')
      assert.equal(a.foo.baz, false)
      assert.equal(a.bar, 1234)
      assert.equal(a.anonymous.constructor.name, '')
      assert.equal(a.getConstructorName(Object.create(null)), '')
      assert.equal(a.getConstructorName(new (class {})()), '')
    })

    it('should search module from the user app', () => {
      comparePaths(path.normalize(remote.process.mainModule.filename), path.resolve(__dirname, 'static', 'main.js'))
      comparePaths(path.normalize(remote.process.mainModule.paths[0]), path.resolve(__dirname, 'static', 'node_modules'))
    })

    it('should work with function properties', () => {
      let a = remote.require(path.join(fixtures, 'module', 'export-function-with-properties.js'))
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

      a = remote.require(path.join(fixtures, 'module', 'function-with-missing-properties.js')).setup()
      assert.equal(a.bar(), true)
      assert.equal(a.bar.baz, undefined)
    })

    it('should work with static class members', () => {
      const a = remote.require(path.join(fixtures, 'module', 'remote-static.js'))
      assert.equal(typeof a.Foo, 'function')
      assert.equal(a.Foo.foo(), 3)
      assert.equal(a.Foo.bar, 'baz')

      const foo = new a.Foo()
      assert.equal(foo.baz(), 123)
    })

    it('includes the length of functions specified as arguments', () => {
      const a = remote.require(path.join(fixtures, 'module', 'function-with-args.js'))
      assert.equal(a((a, b, c, d, f) => {}), 5)
      assert.equal(a((a) => {}), 1)
      assert.equal(a((...args) => {}), 0)
    })

    it('handles circular references in arrays and objects', () => {
      const a = remote.require(path.join(fixtures, 'module', 'circular.js'))

      let arrayA = ['foo']
      const arrayB = [arrayA, 'bar']
      arrayA.push(arrayB)
      assert.deepEqual(a.returnArgs(arrayA, arrayB), [
        ['foo', [null, 'bar']],
        [['foo', null], 'bar']
      ])

      let objectA = {foo: 'bar'}
      const objectB = {baz: objectA}
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

  describe('remote.createFunctionWithReturnValue', () => {
    it('should be called in browser synchronously', () => {
      const buf = Buffer.from('test')
      const call = remote.require(path.join(fixtures, 'module', 'call.js'))
      const result = call.call(remote.createFunctionWithReturnValue(buf))
      assert.equal(result.constructor.name, 'Buffer')
    })
  })

  describe('remote modules', () => {
    it('includes browser process modules as properties', () => {
      assert.equal(typeof remote.app.getPath, 'function')
      assert.equal(typeof remote.webContents.getFocusedWebContents, 'function')
      assert.equal(typeof remote.clipboard.readText, 'function')
      assert.equal(typeof remote.shell.openExternal, 'function')
    })

    it('returns toString() of original function via toString()', () => {
      const {readText} = remote.clipboard
      assert(readText.toString().startsWith('function'))

      const {functionWithToStringProperty} = remote.require(path.join(fixtures, 'module', 'to-string-non-function.js'))
      assert.equal(functionWithToStringProperty.toString, 'hello')
    })
  })

  describe('remote object in renderer', () => {
    it('can change its properties', () => {
      const property = remote.require(path.join(fixtures, 'module', 'property.js'))
      assert.equal(property.property, 1127)

      property.property = null
      assert.equal(property.property, null)
      property.property = undefined
      assert.equal(property.property, undefined)
      property.property = 1007
      assert.equal(property.property, 1007)

      assert.equal(property.getFunctionProperty(), 'foo-browser')
      property.func.property = 'bar'
      assert.equal(property.getFunctionProperty(), 'bar-browser')
      property.func.property = 'foo'  // revert back

      const property2 = remote.require(path.join(fixtures, 'module', 'property.js'))
      assert.equal(property2.property, 1007)
      property.property = 1127
    })

    it('rethrows errors getting/setting properties', () => {
      const foo = remote.require(path.join(fixtures, 'module', 'error-properties.js'))

      assert.throws(() => {
        // eslint-disable-next-line
        foo.bar
      }, /getting error/)

      assert.throws(() => {
        foo.bar = 'test'
      }, /setting error/)
    })

    it('can set a remote property with a remote object', () => {
      const foo = remote.require(path.join(fixtures, 'module', 'remote-object-set.js'))

      assert.doesNotThrow(() => {
        foo.bar = remote.getCurrentWindow()
      })
    })

    it('can construct an object from its member', () => {
      const call = remote.require(path.join(fixtures, 'module', 'call.js'))
      const obj = new call.constructor()
      assert.equal(obj.test, 'test')
    })

    it('can reassign and delete its member functions', () => {
      const remoteFunctions = remote.require(path.join(fixtures, 'module', 'function.js'))
      assert.equal(remoteFunctions.aFunction(), 1127)

      remoteFunctions.aFunction = () => { return 1234 }
      assert.equal(remoteFunctions.aFunction(), 1234)

      assert.equal(delete remoteFunctions.aFunction, true)
    })

    it('is referenced by its members', () => {
      let stringify = remote.getGlobal('JSON').stringify
      global.gc()
      stringify({})
    })
  })

  describe('remote value in browser', () => {
    const print = path.join(fixtures, 'module', 'print_name.js')
    const printName = remote.require(print)

    it('keeps its constructor name for objects', () => {
      const buf = Buffer.from('test')
      assert.equal(printName.print(buf), 'Buffer')
    })

    it('supports instanceof Date', () => {
      const now = new Date()
      assert.equal(printName.print(now), 'Date')
      assert.deepEqual(printName.echo(now), now)
    })

    it('supports instanceof Buffer', () => {
      const buffer = Buffer.from('test')
      assert.ok(buffer.equals(printName.echo(buffer)))

      const objectWithBuffer = {a: 'foo', b: Buffer.from('bar')}
      assert.ok(objectWithBuffer.b.equals(printName.echo(objectWithBuffer).b))

      const arrayWithBuffer = [1, 2, Buffer.from('baz')]
      assert.ok(arrayWithBuffer[2].equals(printName.echo(arrayWithBuffer)[2]))
    })

    it('supports TypedArray', () => {
      const values = [1, 2, 3, 4]
      assert.deepEqual(printName.typedArray(values), values)

      const int16values = new Int16Array([1, 2, 3, 4])
      assert.deepEqual(printName.typedArray(int16values), int16values)
    })
  })

  describe('remote promise', () => {
    it('can be used as promise in each side', (done) => {
      const promise = remote.require(path.join(fixtures, 'module', 'promise.js'))
      promise.twicePromise(Promise.resolve(1234)).then((value) => {
        assert.equal(value, 2468)
        done()
      })
    })

    it('handles rejections via catch(onRejected)', (done) => {
      const promise = remote.require(path.join(fixtures, 'module', 'rejected-promise.js'))
      promise.reject(Promise.resolve(1234)).catch((error) => {
        assert.equal(error.message, 'rejected')
        done()
      })
    })

    it('handles rejections via then(onFulfilled, onRejected)', (done) => {
      const promise = remote.require(path.join(fixtures, 'module', 'rejected-promise.js'))
      promise.reject(Promise.resolve(1234)).then(() => {}, (error) => {
        assert.equal(error.message, 'rejected')
        done()
      })
    })

    it('does not emit unhandled rejection events in the main process', (done) => {
      remote.process.once('unhandledRejection', function (reason) {
        done(reason)
      })

      const promise = remote.require(path.join(fixtures, 'module', 'unhandled-rejection.js'))
      promise.reject().then(() => {
        done(new Error('Promise was not rejected'))
      }).catch((error) => {
        assert.equal(error.message, 'rejected')
        done()
      })
    })

    it('emits unhandled rejection events in the renderer process', (done) => {
      window.addEventListener('unhandledrejection', function (event) {
        event.preventDefault()
        assert.equal(event.reason.message, 'rejected')
        done()
      })

      const promise = remote.require(path.join(fixtures, 'module', 'unhandled-rejection.js'))
      promise.reject().then(() => {
        done(new Error('Promise was not rejected'))
      })
    })
  })

  describe('remote webContents', () => {
    it('can return same object with different getters', () => {
      const contents1 = remote.getCurrentWindow().webContents
      const contents2 = remote.getCurrentWebContents()
      assert(contents1 === contents2)
    })
  })

  describe('remote class', () => {
    const cl = remote.require(path.join(fixtures, 'module', 'class.js'))
    const base = cl.base
    let derived = cl.derived

    it('can get methods', () => {
      assert.equal(base.method(), 'method')
    })

    it('can get properties', () => {
      assert.equal(base.readonly, 'readonly')
    })

    it('can change properties', () => {
      assert.equal(base.value, 'old')
      base.value = 'new'
      assert.equal(base.value, 'new')
      base.value = 'old'
    })

    it('has unenumerable methods', () => {
      assert(!base.hasOwnProperty('method'))
      assert(Object.getPrototypeOf(base).hasOwnProperty('method'))
    })

    it('keeps prototype chain in derived class', () => {
      assert.equal(derived.method(), 'method')
      assert.equal(derived.readonly, 'readonly')
      assert(!derived.hasOwnProperty('method'))
      let proto = Object.getPrototypeOf(derived)
      assert(!proto.hasOwnProperty('method'))
      assert(Object.getPrototypeOf(proto).hasOwnProperty('method'))
    })

    it('is referenced by methods in prototype chain', () => {
      let method = derived.method
      derived = null
      global.gc()
      assert.equal(method(), 'method')
    })
  })
})

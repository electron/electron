'use strict'

const assert = require('assert')
const chai = require('chai')
const dirtyChai = require('dirty-chai')
const path = require('path')
const { closeWindow } = require('./window-helpers')
const { resolveGetters } = require('./assert-helpers')

const { remote, ipcRenderer } = require('electron')
const { ipcMain, BrowserWindow } = remote
const { expect } = chai

chai.use(dirtyChai)

const comparePaths = (path1, path2) => {
  if (process.platform === 'win32') {
    path1 = path1.toLowerCase()
    path2 = path2.toLowerCase()
  }
  assert.strictEqual(path1, path2)
}

describe('remote module', () => {
  const fixtures = path.join(__dirname, 'fixtures')

  let w = null

  afterEach(() => closeWindow(w).then(() => { w = null }))

  describe('remote.getGlobal', () => {
    it('can return custom values', () => {
      ipcRenderer.send('handle-next-remote-get-global', { test: 'Hello World!' })
      expect(remote.getGlobal('test')).to.be.equal('Hello World!')
    })

    it('throws when no returnValue set', () => {
      ipcRenderer.send('handle-next-remote-get-global')
      expect(() => remote.getGlobal('process')).to.throw('Invalid global: process')
    })
  })

  describe('remote.require', () => {
    it('can return custom values', () => {
      ipcRenderer.send('handle-next-remote-require', { test: 'Hello World!' })
      expect(remote.require('test')).to.be.equal('Hello World!')
    })

    it('throws when no returnValue set', () => {
      ipcRenderer.send('handle-next-remote-require')
      expect(() => remote.require('electron')).to.throw('Invalid module: electron')
    })

    it('should returns same object for the same module', () => {
      const dialog1 = remote.require('electron')
      const dialog2 = remote.require('electron')
      assert.strictEqual(dialog1, dialog2)
    })

    it('should work when object contains id property', () => {
      const a = remote.require(path.join(fixtures, 'module', 'id.js'))
      assert.strictEqual(a.id, 1127)
    })

    it('should work when object has no prototype', () => {
      const a = remote.require(path.join(fixtures, 'module', 'no-prototype.js'))
      assert.strictEqual(a.foo.constructor.name, '')
      assert.strictEqual(a.foo.bar, 'baz')
      assert.strictEqual(a.foo.baz, false)
      assert.strictEqual(a.bar, 1234)
      assert.strictEqual(a.anonymous.constructor.name, '')
      assert.strictEqual(a.getConstructorName(Object.create(null)), '')
      assert.strictEqual(a.getConstructorName(new (class {})()), '')
    })

    it('should search module from the user app', () => {
      comparePaths(path.normalize(remote.process.mainModule.filename), path.resolve(__dirname, 'static', 'main.js'))
      comparePaths(path.normalize(remote.process.mainModule.paths[0]), path.resolve(__dirname, 'static', 'node_modules'))
    })

    it('should work with function properties', () => {
      let a = remote.require(path.join(fixtures, 'module', 'export-function-with-properties.js'))
      assert.strictEqual(typeof a, 'function')
      assert.strictEqual(a.bar, 'baz')

      a = remote.require(path.join(fixtures, 'module', 'function-with-properties.js'))
      assert.strictEqual(typeof a, 'object')
      assert.strictEqual(a.foo(), 'hello')
      assert.strictEqual(a.foo.bar, 'baz')
      assert.strictEqual(a.foo.nested.prop, 'yes')
      assert.strictEqual(a.foo.method1(), 'world')
      assert.strictEqual(a.foo.method1.prop1(), 123)

      assert.ok(Object.keys(a.foo).includes('bar'))
      assert.ok(Object.keys(a.foo).includes('nested'))
      assert.ok(Object.keys(a.foo).includes('method1'))

      a = remote.require(path.join(fixtures, 'module', 'function-with-missing-properties.js')).setup()
      assert.strictEqual(a.bar(), true)
      assert.strictEqual(a.bar.baz, undefined)
    })

    it('should work with static class members', () => {
      const a = remote.require(path.join(fixtures, 'module', 'remote-static.js'))
      assert.strictEqual(typeof a.Foo, 'function')
      assert.strictEqual(a.Foo.foo(), 3)
      assert.strictEqual(a.Foo.bar, 'baz')

      const foo = new a.Foo()
      assert.strictEqual(foo.baz(), 123)
    })

    it('includes the length of functions specified as arguments', () => {
      const a = remote.require(path.join(fixtures, 'module', 'function-with-args.js'))
      assert.strictEqual(a((a, b, c, d, f) => {}), 5)
      assert.strictEqual(a((a) => {}), 1)
      assert.strictEqual(a((...args) => {}), 0)
    })

    it('handles circular references in arrays and objects', () => {
      const a = remote.require(path.join(fixtures, 'module', 'circular.js'))

      let arrayA = ['foo']
      const arrayB = [arrayA, 'bar']
      arrayA.push(arrayB)
      assert.deepStrictEqual(a.returnArgs(arrayA, arrayB), [
        ['foo', [null, 'bar']],
        [['foo', null], 'bar']
      ])

      let objectA = { foo: 'bar' }
      const objectB = { baz: objectA }
      objectA.objectB = objectB
      assert.deepStrictEqual(a.returnArgs(objectA, objectB), [
        { foo: 'bar', objectB: { baz: null } },
        { baz: { foo: 'bar', objectB: null } }
      ])

      arrayA = [1, 2, 3]
      assert.deepStrictEqual(a.returnArgs({ foo: arrayA }, { bar: arrayA }), [
        { foo: [1, 2, 3] },
        { bar: [1, 2, 3] }
      ])

      objectA = { foo: 'bar' }
      assert.deepStrictEqual(a.returnArgs({ foo: objectA }, { bar: objectA }), [
        { foo: { foo: 'bar' } },
        { bar: { foo: 'bar' } }
      ])

      arrayA = []
      arrayA.push(arrayA)
      assert.deepStrictEqual(a.returnArgs(arrayA), [
        [null]
      ])

      objectA = {}
      objectA.foo = objectA
      objectA.bar = 'baz'
      assert.deepStrictEqual(a.returnArgs(objectA), [
        { foo: null, bar: 'baz' }
      ])

      objectA = {}
      objectA.foo = { bar: objectA }
      objectA.bar = 'baz'
      assert.deepStrictEqual(a.returnArgs(objectA), [
        { foo: { bar: null }, bar: 'baz' }
      ])
    })
  })

  describe('remote.createFunctionWithReturnValue', () => {
    it('should be called in browser synchronously', () => {
      const buf = Buffer.from('test')
      const call = remote.require(path.join(fixtures, 'module', 'call.js'))
      const result = call.call(remote.createFunctionWithReturnValue(buf))
      assert.strictEqual(result.constructor.name, 'Buffer')
    })
  })

  describe('remote modules', () => {
    it('includes browser process modules as properties', () => {
      assert.strictEqual(typeof remote.app.getPath, 'function')
      assert.strictEqual(typeof remote.webContents.getFocusedWebContents, 'function')
      assert.strictEqual(typeof remote.clipboard.readText, 'function')
      assert.strictEqual(typeof remote.shell.openExternal, 'function')
    })

    it('returns toString() of original function via toString()', () => {
      const { readText } = remote.clipboard
      assert(readText.toString().startsWith('function'))

      const { functionWithToStringProperty } = remote.require(path.join(fixtures, 'module', 'to-string-non-function.js'))
      assert.strictEqual(functionWithToStringProperty.toString, 'hello')
    })
  })

  describe('remote object in renderer', () => {
    it('can change its properties', () => {
      const property = remote.require(path.join(fixtures, 'module', 'property.js'))
      assert.strictEqual(property.property, 1127)

      property.property = null
      assert.strictEqual(property.property, null)
      property.property = undefined
      assert.strictEqual(property.property, undefined)
      property.property = 1007
      assert.strictEqual(property.property, 1007)

      assert.strictEqual(property.getFunctionProperty(), 'foo-browser')
      property.func.property = 'bar'
      assert.strictEqual(property.getFunctionProperty(), 'bar-browser')
      property.func.property = 'foo' // revert back

      const property2 = remote.require(path.join(fixtures, 'module', 'property.js'))
      assert.strictEqual(property2.property, 1007)
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
      assert.strictEqual(obj.test, 'test')
    })

    it('can reassign and delete its member functions', () => {
      const remoteFunctions = remote.require(path.join(fixtures, 'module', 'function.js'))
      assert.strictEqual(remoteFunctions.aFunction(), 1127)

      remoteFunctions.aFunction = () => { return 1234 }
      assert.strictEqual(remoteFunctions.aFunction(), 1234)

      assert.strictEqual(delete remoteFunctions.aFunction, true)
    })

    it('is referenced by its members', () => {
      const stringify = remote.getGlobal('JSON').stringify
      global.gc()
      stringify({})
    })
  })

  describe('remote value in browser', () => {
    const print = path.join(fixtures, 'module', 'print_name.js')
    const printName = remote.require(print)

    it('converts NaN to undefined', () => {
      assert.strictEqual(printName.getNaN(), undefined)
      assert.strictEqual(printName.echo(NaN), undefined)
    })

    it('converts Infinity to undefined', () => {
      assert.strictEqual(printName.getInfinity(), undefined)
      assert.strictEqual(printName.echo(Infinity), undefined)
    })

    it('keeps its constructor name for objects', () => {
      const buf = Buffer.from('test')
      assert.strictEqual(printName.print(buf), 'Buffer')
    })

    it('supports instanceof Date', () => {
      const now = new Date()
      assert.strictEqual(printName.print(now), 'Date')
      assert.deepStrictEqual(printName.echo(now), now)
    })

    it('supports instanceof Buffer', () => {
      const buffer = Buffer.from('test')
      assert.ok(buffer.equals(printName.echo(buffer)))

      const objectWithBuffer = { a: 'foo', b: Buffer.from('bar') }
      assert.ok(objectWithBuffer.b.equals(printName.echo(objectWithBuffer).b))

      const arrayWithBuffer = [1, 2, Buffer.from('baz')]
      assert.ok(arrayWithBuffer[2].equals(printName.echo(arrayWithBuffer)[2]))
    })

    it('supports instanceof ArrayBuffer', () => {
      const buffer = new ArrayBuffer(8)
      const view = new DataView(buffer)

      view.setFloat64(0, Math.PI)
      assert.deepStrictEqual(printName.echo(buffer), buffer)
      assert.strictEqual(printName.print(buffer), 'ArrayBuffer')
    })

    it('supports instanceof Int8Array', () => {
      const values = [1, 2, 3, 4]
      assert.deepStrictEqual([...printName.typedArray('Int8Array', values)], values)

      const int8values = new Int8Array(values)
      assert.deepStrictEqual(printName.typedArray('Int8Array', int8values), int8values)
      assert.strictEqual(printName.print(int8values), 'Int8Array')
    })

    it('supports instanceof Uint8Array', () => {
      const values = [1, 2, 3, 4]
      assert.deepStrictEqual([...printName.typedArray('Uint8Array', values)], values)

      const uint8values = new Uint8Array(values)
      assert.deepStrictEqual(printName.typedArray('Uint8Array', uint8values), uint8values)
      assert.strictEqual(printName.print(uint8values), 'Uint8Array')
    })

    it('supports instanceof Uint8ClampedArray', () => {
      const values = [1, 2, 3, 4]
      assert.deepStrictEqual([...printName.typedArray('Uint8ClampedArray', values)], values)

      const uint8values = new Uint8ClampedArray(values)
      assert.deepStrictEqual(printName.typedArray('Uint8ClampedArray', uint8values), uint8values)
      assert.strictEqual(printName.print(uint8values), 'Uint8ClampedArray')
    })

    it('supports instanceof Int16Array', () => {
      const values = [0x1234, 0x2345, 0x3456, 0x4567]
      assert.deepStrictEqual([...printName.typedArray('Int16Array', values)], values)

      const int16values = new Int16Array(values)
      assert.deepStrictEqual(printName.typedArray('Int16Array', int16values), int16values)
      assert.strictEqual(printName.print(int16values), 'Int16Array')
    })

    it('supports instanceof Uint16Array', () => {
      const values = [0x1234, 0x2345, 0x3456, 0x4567]
      assert.deepStrictEqual([...printName.typedArray('Uint16Array', values)], values)

      const uint16values = new Uint16Array(values)
      assert.deepStrictEqual(printName.typedArray('Uint16Array', uint16values), uint16values)
      assert.strictEqual(printName.print(uint16values), 'Uint16Array')
    })

    it('supports instanceof Int32Array', () => {
      const values = [0x12345678, 0x23456789]
      assert.deepStrictEqual([...printName.typedArray('Int32Array', values)], values)

      const int32values = new Int32Array(values)
      assert.deepStrictEqual(printName.typedArray('Int32Array', int32values), int32values)
      assert.strictEqual(printName.print(int32values), 'Int32Array')
    })

    it('supports instanceof Uint32Array', () => {
      const values = [0x12345678, 0x23456789]
      assert.deepStrictEqual([...printName.typedArray('Uint32Array', values)], values)

      const uint32values = new Uint32Array(values)
      assert.deepStrictEqual(printName.typedArray('Uint32Array', uint32values), uint32values)
      assert.strictEqual(printName.print(uint32values), 'Uint32Array')
    })

    it('supports instanceof Float32Array', () => {
      const values = [0.5, 1.0, 1.5]
      assert.deepStrictEqual([...printName.typedArray('Float32Array', values)], values)

      const float32values = new Float32Array()
      assert.deepStrictEqual(printName.typedArray('Float32Array', float32values), float32values)
      assert.strictEqual(printName.print(float32values), 'Float32Array')
    })

    it('supports instanceof Float64Array', () => {
      const values = [0.5, 1.0, 1.5]
      assert.deepStrictEqual([...printName.typedArray('Float64Array', values)], values)

      const float64values = new Float64Array([0.5, 1.0, 1.5])
      assert.deepStrictEqual(printName.typedArray('Float64Array', float64values), float64values)
      assert.strictEqual(printName.print(float64values), 'Float64Array')
    })
  })

  describe('remote promise', () => {
    it('can be used as promise in each side', (done) => {
      const promise = remote.require(path.join(fixtures, 'module', 'promise.js'))
      promise.twicePromise(Promise.resolve(1234)).then((value) => {
        assert.strictEqual(value, 2468)
        done()
      })
    })

    it('handles rejections via catch(onRejected)', (done) => {
      const promise = remote.require(path.join(fixtures, 'module', 'rejected-promise.js'))
      promise.reject(Promise.resolve(1234)).catch((error) => {
        assert.strictEqual(error.message, 'rejected')
        done()
      })
    })

    it('handles rejections via then(onFulfilled, onRejected)', (done) => {
      const promise = remote.require(path.join(fixtures, 'module', 'rejected-promise.js'))
      promise.reject(Promise.resolve(1234)).then(() => {}, (error) => {
        assert.strictEqual(error.message, 'rejected')
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
        assert.strictEqual(error.message, 'rejected')
        done()
      })
    })

    it('emits unhandled rejection events in the renderer process', (done) => {
      window.addEventListener('unhandledrejection', function handler (event) {
        event.preventDefault()
        assert.strictEqual(event.reason.message, 'rejected')
        window.removeEventListener('unhandledrejection', handler)
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
      assert.strictEqual(base.method(), 'method')
    })

    it('can get properties', () => {
      assert.strictEqual(base.readonly, 'readonly')
    })

    it('can change properties', () => {
      assert.strictEqual(base.value, 'old')
      base.value = 'new'
      assert.strictEqual(base.value, 'new')
      base.value = 'old'
    })

    it('has unenumerable methods', () => {
      assert(!base.hasOwnProperty('method'))
      assert(Object.getPrototypeOf(base).hasOwnProperty('method'))
    })

    it('keeps prototype chain in derived class', () => {
      assert.strictEqual(derived.method(), 'method')
      assert.strictEqual(derived.readonly, 'readonly')
      assert(!derived.hasOwnProperty('method'))
      const proto = Object.getPrototypeOf(derived)
      assert(!proto.hasOwnProperty('method'))
      assert(Object.getPrototypeOf(proto).hasOwnProperty('method'))
    })

    it('is referenced by methods in prototype chain', () => {
      const method = derived.method
      derived = null
      global.gc()
      assert.strictEqual(method(), 'method')
    })
  })

  describe('remote exception', () => {
    const throwFunction = remote.require(path.join(fixtures, 'module', 'exception.js'))

    it('throws errors from the main process', () => {
      assert.throws(() => {
        throwFunction()
      })
    })

    it('throws custom errors from the main process', () => {
      const err = new Error('error')
      err.cause = new Error('cause')
      err.prop = 'error prop'
      try {
        throwFunction(err)
      } catch (error) {
        assert.ok(error.from)
        assert.deepStrictEqual(error.cause, ...resolveGetters(err))
      }
    })
  })

  describe('remote function in renderer', () => {
    afterEach(() => {
      ipcMain.removeAllListeners('done')
    })

    it('works when created in preload script', (done) => {
      ipcMain.once('done', () => w.close())
      const preload = path.join(fixtures, 'module', 'preload-remote-function.js')
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          preload
        }
      })
      w.once('closed', () => done())
      w.loadURL('about:blank')
    })
  })
})

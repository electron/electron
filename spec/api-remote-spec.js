'use strict'

const chai = require('chai')
const dirtyChai = require('dirty-chai')
const path = require('path')
const { resolveGetters } = require('./expect-helpers')
const { ifdescribe } = require('./spec-helpers')

const { remote, ipcRenderer } = require('electron')
const { ipcMain, BrowserWindow } = remote
const { expect } = chai

const features = process.electronBinding('features')

chai.use(dirtyChai)

const comparePaths = (path1, path2) => {
  if (process.platform === 'win32') {
    path1 = path1.toLowerCase()
    path2 = path2.toLowerCase()
  }
  expect(path1).to.equal(path2)
}

ifdescribe(features.isRemoteModuleEnabled())('remote module', () => {
  const fixtures = path.join(__dirname, 'fixtures')

  describe('remote.require', () => {
    it('should returns same object for the same module', () => {
      const dialog1 = remote.require('electron')
      const dialog2 = remote.require('electron')
      expect(dialog1).to.equal(dialog2)
    })

    it('should work when object contains id property', () => {
      const a = remote.require(path.join(fixtures, 'module', 'id.js'))
      expect(a.id).to.equal(1127)
    })

    it('should work when object has no prototype', () => {
      const a = remote.require(path.join(fixtures, 'module', 'no-prototype.js'))
      expect(a.foo.constructor.name).to.equal('')
      expect(a.foo.bar).to.equal('baz')
      expect(a.foo.baz).to.equal(false)
      expect(a.bar).to.equal(1234)
      expect(a.anonymous.constructor.name).to.equal('')
      expect(a.getConstructorName(Object.create(null))).to.equal('')
      expect(a.getConstructorName(new (class {})())).to.equal('')
    })

    it('should search module from the user app', () => {
      comparePaths(path.normalize(remote.process.mainModule.filename), path.resolve(__dirname, 'static', 'main.js'))
      comparePaths(path.normalize(remote.process.mainModule.paths[0]), path.resolve(__dirname, 'static', 'node_modules'))
    })

    it('should work with function properties', () => {
      let a = remote.require(path.join(fixtures, 'module', 'export-function-with-properties.js'))
      expect(a).to.be.a('function')
      expect(a.bar).to.equal('baz')

      a = remote.require(path.join(fixtures, 'module', 'function-with-properties.js'))
      expect(a).to.be.an('object')
      expect(a.foo()).to.equal('hello')
      expect(a.foo.bar).to.equal('baz')
      expect(a.foo.nested.prop).to.equal('yes')
      expect(a.foo.method1()).to.equal('world')
      expect(a.foo.method1.prop1()).to.equal(123)

      expect(a.foo).to.have.a.property('bar')
      expect(a.foo).to.have.a.property('nested')
      expect(a.foo).to.have.a.property('method1')

      a = remote.require(path.join(fixtures, 'module', 'function-with-missing-properties.js')).setup()
      expect(a.bar()).to.equal(true)
      expect(a.bar.baz).to.be.undefined()
    })

    it('should work with static class members', () => {
      const a = remote.require(path.join(fixtures, 'module', 'remote-static.js'))
      expect(a.Foo).to.be.a('function')
      expect(a.Foo.foo()).to.equal(3)
      expect(a.Foo.bar).to.equal('baz')

      const foo = new a.Foo()
      expect(foo.baz()).to.equal(123)
    })

    it('includes the length of functions specified as arguments', () => {
      const a = remote.require(path.join(fixtures, 'module', 'function-with-args.js'))
      expect(a((a, b, c, d, f) => {})).to.equal(5)
      expect(a((a) => {})).to.equal(1)
      expect(a((...args) => {})).to.equal(0)
    })

    it('handles circular references in arrays and objects', () => {
      const a = remote.require(path.join(fixtures, 'module', 'circular.js'))

      let arrayA = ['foo']
      const arrayB = [arrayA, 'bar']
      arrayA.push(arrayB)
      expect(a.returnArgs(arrayA, arrayB)).to.deep.equal([
        ['foo', [null, 'bar']],
        [['foo', null], 'bar']
      ])

      let objectA = { foo: 'bar' }
      const objectB = { baz: objectA }
      objectA.objectB = objectB
      expect(a.returnArgs(objectA, objectB)).to.deep.equal([
        { foo: 'bar', objectB: { baz: null } },
        { baz: { foo: 'bar', objectB: null } }
      ])

      arrayA = [1, 2, 3]
      expect(a.returnArgs({ foo: arrayA }, { bar: arrayA })).to.deep.equal([
        { foo: [1, 2, 3] },
        { bar: [1, 2, 3] }
      ])

      objectA = { foo: 'bar' }
      expect(a.returnArgs({ foo: objectA }, { bar: objectA })).to.deep.equal([
        { foo: { foo: 'bar' } },
        { bar: { foo: 'bar' } }
      ])

      arrayA = []
      arrayA.push(arrayA)
      expect(a.returnArgs(arrayA)).to.deep.equal([
        [null]
      ])

      objectA = {}
      objectA.foo = objectA
      objectA.bar = 'baz'
      expect(a.returnArgs(objectA)).to.deep.equal([
        { foo: null, bar: 'baz' }
      ])

      objectA = {}
      objectA.foo = { bar: objectA }
      objectA.bar = 'baz'
      expect(a.returnArgs(objectA)).to.deep.equal([
        { foo: { bar: null }, bar: 'baz' }
      ])
    })
  })

  describe('remote.createFunctionWithReturnValue', () => {
    it('should be called in browser synchronously', () => {
      const buf = Buffer.from('test')
      const call = remote.require(path.join(fixtures, 'module', 'call.js'))
      const result = call.call(remote.createFunctionWithReturnValue(buf))
      expect(result).to.be.an.instanceOf(Buffer)
    })
  })

  describe('remote modules', () => {
    it('includes browser process modules as properties', () => {
      expect(remote.app.getPath).to.be.a('function')
      expect(remote.webContents.getFocusedWebContents).to.be.a('function')
      expect(remote.clipboard.readText).to.be.a('function')
    })

    it('returns toString() of original function via toString()', () => {
      const { readText } = remote.clipboard
      expect(readText.toString().startsWith('function')).to.be.true()

      const { functionWithToStringProperty } = remote.require(path.join(fixtures, 'module', 'to-string-non-function.js'))
      expect(functionWithToStringProperty.toString).to.equal('hello')
    })
  })

  describe('remote object in renderer', () => {
    it('can change its properties', () => {
      const property = remote.require(path.join(fixtures, 'module', 'property.js'))
      expect(property).to.have.a.property('property').that.is.equal(1127)

      property.property = null
      expect(property).to.have.a.property('property').that.is.null()
      property.property = undefined
      expect(property).to.have.a.property('property').that.is.undefined()
      property.property = 1007
      expect(property).to.have.a.property('property').that.is.equal(1007)

      expect(property.getFunctionProperty()).to.equal('foo-browser')
      property.func.property = 'bar'
      expect(property.getFunctionProperty()).to.equal('bar-browser')
      property.func.property = 'foo' // revert back

      const property2 = remote.require(path.join(fixtures, 'module', 'property.js'))
      expect(property2.property).to.equal(1007)
      property.property = 1127
    })

    it('rethrows errors getting/setting properties', () => {
      const foo = remote.require(path.join(fixtures, 'module', 'error-properties.js'))

      expect(() => {
        // eslint-disable-next-line
        foo.bar
      }).to.throw('getting error')

      expect(() => {
        foo.bar = 'test'
      }).to.throw('setting error')
    })

    it('can set a remote property with a remote object', () => {
      const foo = remote.require(path.join(fixtures, 'module', 'remote-object-set.js'))

      expect(() => {
        foo.bar = remote.getCurrentWindow()
      }).to.not.throw()
    })

    it('can construct an object from its member', () => {
      const call = remote.require(path.join(fixtures, 'module', 'call.js'))
      const obj = new call.constructor()
      expect(obj.test).to.equal('test')
    })

    it('can reassign and delete its member functions', () => {
      const remoteFunctions = remote.require(path.join(fixtures, 'module', 'function.js'))
      expect(remoteFunctions.aFunction()).to.equal(1127)

      remoteFunctions.aFunction = () => { return 1234 }
      expect(remoteFunctions.aFunction()).to.equal(1234)

      expect(delete remoteFunctions.aFunction).to.equal(true)
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

    it('preserves NaN', () => {
      expect(printName.getNaN()).to.be.NaN()
      expect(printName.echo(NaN)).to.be.NaN()
    })

    it('preserves Infinity', () => {
      expect(printName.getInfinity()).to.equal(Infinity)
      expect(printName.echo(Infinity)).to.equal(Infinity)
    })

    it('keeps its constructor name for objects', () => {
      const buf = Buffer.from('test')
      expect(printName.print(buf)).to.equal('Buffer')
    })

    it('supports instanceof Boolean', () => {
      const obj = Boolean(true)
      expect(printName.print(obj)).to.equal('Boolean')
      expect(printName.echo(obj)).to.deep.equal(obj)
    })

    it('supports instanceof Number', () => {
      const obj = Number(42)
      expect(printName.print(obj)).to.equal('Number')
      expect(printName.echo(obj)).to.deep.equal(obj)
    })

    it('supports instanceof String', () => {
      const obj = String('Hello World!')
      expect(printName.print(obj)).to.equal('String')
      expect(printName.echo(obj)).to.deep.equal(obj)
    })

    it('supports instanceof Date', () => {
      const now = new Date()
      expect(printName.print(now)).to.equal('Date')
      expect(printName.echo(now)).to.deep.equal(now)
    })

    it('supports instanceof RegExp', () => {
      const regexp = RegExp('.*')
      expect(printName.print(regexp)).to.equal('RegExp')
      expect(printName.echo(regexp)).to.deep.equal(regexp)
    })

    it('supports instanceof Buffer', () => {
      const buffer = Buffer.from('test')
      expect(buffer.equals(printName.echo(buffer))).to.be.true()

      const objectWithBuffer = { a: 'foo', b: Buffer.from('bar') }
      expect(objectWithBuffer.b.equals(printName.echo(objectWithBuffer).b)).to.be.true()

      const arrayWithBuffer = [1, 2, Buffer.from('baz')]
      expect(arrayWithBuffer[2].equals(printName.echo(arrayWithBuffer)[2])).to.be.true()
    })

    it('supports instanceof ArrayBuffer', () => {
      const buffer = new ArrayBuffer(8)
      const view = new DataView(buffer)

      view.setFloat64(0, Math.PI)
      expect(printName.echo(buffer)).to.deep.equal(buffer)
      expect(printName.print(buffer)).to.equal('ArrayBuffer')
    })

    it('supports instanceof Int8Array', () => {
      const values = [1, 2, 3, 4]
      expect([...printName.typedArray('Int8Array', values)]).to.deep.equal(values)

      const int8values = new Int8Array(values)
      expect(printName.typedArray('Int8Array', int8values)).to.deep.equal(int8values)
      expect(printName.print(int8values)).to.equal('Int8Array')
    })

    it('supports instanceof Uint8Array', () => {
      const values = [1, 2, 3, 4]
      expect([...printName.typedArray('Uint8Array', values)]).to.deep.equal(values)

      const uint8values = new Uint8Array(values)
      expect(printName.typedArray('Uint8Array', uint8values)).to.deep.equal(uint8values)
      expect(printName.print(uint8values)).to.equal('Uint8Array')
    })

    it('supports instanceof Uint8ClampedArray', () => {
      const values = [1, 2, 3, 4]
      expect([...printName.typedArray('Uint8ClampedArray', values)]).to.deep.equal(values)

      const uint8values = new Uint8ClampedArray(values)
      expect(printName.typedArray('Uint8ClampedArray', uint8values)).to.deep.equal(uint8values)
      expect(printName.print(uint8values)).to.equal('Uint8ClampedArray')
    })

    it('supports instanceof Int16Array', () => {
      const values = [0x1234, 0x2345, 0x3456, 0x4567]
      expect([...printName.typedArray('Int16Array', values)]).to.deep.equal(values)

      const int16values = new Int16Array(values)
      expect(printName.typedArray('Int16Array', int16values)).to.deep.equal(int16values)
      expect(printName.print(int16values)).to.equal('Int16Array')
    })

    it('supports instanceof Uint16Array', () => {
      const values = [0x1234, 0x2345, 0x3456, 0x4567]
      expect([...printName.typedArray('Uint16Array', values)]).to.deep.equal(values)

      const uint16values = new Uint16Array(values)
      expect(printName.typedArray('Uint16Array', uint16values)).to.deep.equal(uint16values)
      expect(printName.print(uint16values)).to.equal('Uint16Array')
    })

    it('supports instanceof Int32Array', () => {
      const values = [0x12345678, 0x23456789]
      expect([...printName.typedArray('Int32Array', values)]).to.deep.equal(values)

      const int32values = new Int32Array(values)
      expect(printName.typedArray('Int32Array', int32values)).to.deep.equal(int32values)
      expect(printName.print(int32values)).to.equal('Int32Array')
    })

    it('supports instanceof Uint32Array', () => {
      const values = [0x12345678, 0x23456789]
      expect([...printName.typedArray('Uint32Array', values)]).to.deep.equal(values)

      const uint32values = new Uint32Array(values)
      expect(printName.typedArray('Uint32Array', uint32values)).to.deep.equal(uint32values)
      expect(printName.print(uint32values)).to.equal('Uint32Array')
    })

    it('supports instanceof Float32Array', () => {
      const values = [0.5, 1.0, 1.5]
      expect([...printName.typedArray('Float32Array', values)]).to.deep.equal(values)

      const float32values = new Float32Array()
      expect(printName.typedArray('Float32Array', float32values)).to.deep.equal(float32values)
      expect(printName.print(float32values)).to.equal('Float32Array')
    })

    it('supports instanceof Float64Array', () => {
      const values = [0.5, 1.0, 1.5]
      expect([...printName.typedArray('Float64Array', values)]).to.deep.equal(values)

      const float64values = new Float64Array([0.5, 1.0, 1.5])
      expect(printName.typedArray('Float64Array', float64values)).to.deep.equal(float64values)
      expect(printName.print(float64values)).to.equal('Float64Array')
    })
  })

  describe('remote promise', () => {
    it('can be used as promise in each side', (done) => {
      const promise = remote.require(path.join(fixtures, 'module', 'promise.js'))
      promise.twicePromise(Promise.resolve(1234)).then((value) => {
        expect(value).to.equal(2468)
        done()
      })
    })

    it('handles rejections via catch(onRejected)', (done) => {
      const promise = remote.require(path.join(fixtures, 'module', 'rejected-promise.js'))
      promise.reject(Promise.resolve(1234)).catch((error) => {
        expect(error.message).to.equal('rejected')
        done()
      })
    })

    it('handles rejections via then(onFulfilled, onRejected)', (done) => {
      const promise = remote.require(path.join(fixtures, 'module', 'rejected-promise.js'))
      promise.reject(Promise.resolve(1234)).then(() => {}, (error) => {
        expect(error.message).to.equal('rejected')
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
        expect(error.message).to.equal('rejected')
        done()
      })
    })

    it('emits unhandled rejection events in the renderer process', (done) => {
      window.addEventListener('unhandledrejection', function handler (event) {
        event.preventDefault()
        expect(event.reason.message).to.equal('rejected')
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
      expect(contents1).to.equal(contents2)
    })
  })

  describe('remote class', () => {
    const cl = remote.require(path.join(fixtures, 'module', 'class.js'))
    const base = cl.base
    let derived = cl.derived

    it('can get methods', () => {
      expect(base.method()).to.equal('method')
    })

    it('can get properties', () => {
      expect(base.readonly).to.equal('readonly')
    })

    it('can change properties', () => {
      expect(base.value).to.equal('old')
      base.value = 'new'
      expect(base.value).to.equal('new')
      base.value = 'old'
    })

    it('has unenumerable methods', () => {
      expect(base).to.not.have.own.property('method')
      expect(Object.getPrototypeOf(base)).to.have.own.property('method')
    })

    it('keeps prototype chain in derived class', () => {
      expect(derived.method()).to.equal('method')
      expect(derived.readonly).to.equal('readonly')
      expect(derived).to.not.have.own.property('method')
      const proto = Object.getPrototypeOf(derived)
      expect(proto).to.not.have.own.property('method')
      expect(Object.getPrototypeOf(proto)).to.have.own.property('method')
    })

    it('is referenced by methods in prototype chain', () => {
      const method = derived.method
      derived = null
      global.gc()
      expect(method()).to.equal('method')
    })
  })

  describe('remote exception', () => {
    const throwFunction = remote.require(path.join(fixtures, 'module', 'exception.js'))

    it('throws errors from the main process', () => {
      expect(() => {
        throwFunction()
      }).to.throw(/undefined/)
    })

    it('tracks error cause', () => {
      try {
        throwFunction(new Error('error from main'))
        expect.fail()
      } catch (e) {
        expect(e.message).to.match(/Could not call remote function/)
        expect(e.cause.message).to.equal('error from main')
      }
    })
  })

  describe('constructing a Uint8Array', () => {
    it('does not crash', () => {
      const RUint8Array = remote.getGlobal('Uint8Array')
      const arr = new RUint8Array()
    })
  })

  describe('with an overriden global Promise constrctor', () => {
    let original

    before(() => {
      original = Promise
    })

    it('using a promise based method  resolves correctly', async () => {
      expect(await remote.getGlobal('returnAPromise')(123)).to.equal(123)
      global.Promise = { resolve: () => ({}) }
      expect(await remote.getGlobal('returnAPromise')(456)).to.equal(456)
    })

    after(() => {
      global.Promise = original
    })
  })
})

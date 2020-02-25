import * as path from 'path'
import { expect } from 'chai'
import { closeAllWindows } from './window-helpers'
import { ifdescribe } from './spec-helpers'

import { ipcMain, BrowserWindow } from 'electron'
import { emittedOnce } from './events-helpers'

const features = process.electronBinding('features')

const expectPathsEqual = (path1: string, path2: string) => {
  if (process.platform === 'win32') {
    path1 = path1.toLowerCase()
    path2 = path2.toLowerCase()
  }
  expect(path1).to.equal(path2)
}

function makeRemotely (windowGetter: () => BrowserWindow) {
  async function remotely (script: Function, ...args: any[]) {
    // executeJavaScript obfuscates the error if the script throws, so catch any
    // errors manually.
    const assembledScript = `(async function() {
      try {
        return { result: await Promise.resolve((${script})(...${JSON.stringify(args)})) }
      } catch (e) {
        return { error: e.message, stack: e.stack }
      }
    })()`
    const { result, error, stack } = await windowGetter().webContents.executeJavaScript(assembledScript)
    if (error) {
      const e = new Error(error)
      e.stack = stack
      throw e
    }
    return result
  }
  remotely.it = (...vars: any[]) => (name: string, fn: Function) => {
    it(name, async () => {
      await remotely(fn, ...vars)
    })
  }
  return remotely
}

function makeWindow () {
  let w: BrowserWindow
  before(async () => {
    w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, enableRemoteModule: true } })
    await w.loadURL('about:blank')
    await w.webContents.executeJavaScript(`{
      const chai_1 = window.chai_1 = require('chai')
      chai_1.use(require('chai-as-promised'))
      chai_1.use(require('dirty-chai'))
      null
    }`)
  })
  after(closeAllWindows)
  return () => w
}

function makeEachWindow () {
  let w: BrowserWindow
  beforeEach(async () => {
    w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, enableRemoteModule: true } })
    await w.loadURL('about:blank')
    await w.webContents.executeJavaScript(`{
      const chai_1 = window.chai_1 = require('chai')
      chai_1.use(require('chai-as-promised'))
      chai_1.use(require('dirty-chai'))
      null
    }`)
  })
  afterEach(closeAllWindows)
  return () => w
}

ifdescribe(features.isRemoteModuleEnabled())('remote module', () => {
  const fixtures = path.join(__dirname, 'fixtures')

  describe('', () => {
    const w = makeWindow()
    const remotely = makeRemotely(w)

    describe('remote.getGlobal filtering', () => {
      it('can return custom values', async () => {
        w().webContents.once('remote-get-global', (event, name) => {
          event.returnValue = name
        })
        expect(await remotely(() => require('electron').remote.getGlobal('test'))).to.equal('test')
      })

      it('throws when no returnValue set', async () => {
        w().webContents.once('remote-get-global', (event) => {
          event.preventDefault()
        })
        await expect(remotely(() => require('electron').remote.getGlobal('test'))).to.eventually.be.rejected(`Blocked remote.getGlobal('test')`)
      })
    })

    describe('remote.getBuiltin filtering', () => {
      it('can return custom values', async () => {
        w().webContents.once('remote-get-builtin', (event, name) => {
          event.returnValue = name
        })
        expect(await remotely(() => (require('electron').remote as any).getBuiltin('test'))).to.equal('test')
      })

      it('throws when no returnValue set', async () => {
        w().webContents.once('remote-get-builtin', (event) => {
          event.preventDefault()
        })
        await expect(remotely(() => (require('electron').remote as any).getBuiltin('test'))).to.eventually.be.rejected(`Blocked remote.getGlobal('test')`)
      })
    })

    describe('remote.require filtering', () => {
      it('can return custom values', async () => {
        w().webContents.once('remote-require', (event, name) => {
          event.returnValue = name
        })
        expect(await remotely(() => require('electron').remote.require('test'))).to.equal('test')
      })

      it('throws when no returnValue set', async () => {
        w().webContents.once('remote-require', (event) => {
          event.preventDefault()
        })
        await expect(remotely(() => require('electron').remote.require('test'))).to.eventually.be.rejected(`Blocked remote.require('test')`)
      })
    })

    describe('remote.getCurrentWindow filtering', () => {
      it('can return custom value', async () => {
        w().webContents.once('remote-get-current-window', (e) => {
          e.returnValue = 'some window'
        })
        expect(await remotely(() => require('electron').remote.getCurrentWindow())).to.equal('some window')
      })

      it('throws when no returnValue set', async () => {
        w().webContents.once('remote-get-current-window', (event) => {
          event.preventDefault()
        })
        await expect(remotely(() => require('electron').remote.getCurrentWindow())).to.eventually.be.rejected(`Blocked remote.getCurrentWindow()`)
      })
    })

    describe('remote.getCurrentWebContents filtering', () => {
      it('can return custom value', async () => {
        w().webContents.once('remote-get-current-web-contents', (event) => {
          event.returnValue = 'some web contents'
        })
        expect(await remotely(() => require('electron').remote.getCurrentWebContents())).to.equal('some web contents')
      })

      it('throws when no returnValue set', async () => {
        w().webContents.once('remote-get-current-web-contents', (event) => {
          event.preventDefault()
        })
        await expect(remotely(() => require('electron').remote.getCurrentWebContents())).to.eventually.be.rejected(`Blocked remote.getCurrentWebContents()`)
      })
    })
  })

  describe('remote references', () => {
    const w = makeEachWindow()
    it('render-view-deleted is sent when page is destroyed', (done) => {
      w().webContents.once('render-view-deleted' as any, () => {
        done()
      })
      w().destroy()
    })

    // The ELECTRON_BROWSER_CONTEXT_RELEASE message relies on this to work.
    it('message can be sent on exit when page is being navigated', async () => {
      after(() => { ipcMain.removeAllListeners('SENT_ON_EXIT') })
      w().webContents.once('did-finish-load', () => {
        w().webContents.loadURL('about:blank')
      })
      w().loadFile(path.join(fixtures, 'api', 'send-on-exit.html'))
      await emittedOnce(ipcMain, 'SENT_ON_EXIT')
    })
  })

  describe('remote function in renderer', () => {
    afterEach(() => {
      ipcMain.removeAllListeners('done')
    })
    afterEach(closeAllWindows)

    it('works when created in preload script', async () => {
      const preload = path.join(fixtures, 'module', 'preload-remote-function.js')
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          preload,
          enableRemoteModule: true
        }
      })
      w.loadURL('about:blank')
      await emittedOnce(ipcMain, 'done')
    })
  })

  describe('remote objects registry', () => {
    it('does not dereference until the render view is deleted (regression)', (done) => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          enableRemoteModule: true
        }
      })

      ipcMain.once('error-message', (event, message) => {
        expect(message).to.match(/^Cannot call method 'getURL' on missing remote object/)
        done()
      })

      w.loadFile(path.join(fixtures, 'api', 'render-view-deleted.html'))
    })
  })

  describe('remote listeners', () => {
    afterEach(closeAllWindows)

    it('detaches listeners subscribed to destroyed renderers, and shows a warning', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          enableRemoteModule: true
        }
      })
      await w.loadFile(path.join(fixtures, 'api', 'remote-event-handler.html'))
      w.webContents.reload()
      await emittedOnce(w.webContents, 'did-finish-load')

      const expectedMessage = [
        'Attempting to call a function in a renderer window that has been closed or released.',
        'Function provided here: remote-event-handler.html:11:33',
        'Remote event names: remote-handler, other-remote-handler'
      ].join('\n')

      expect(w.webContents.listenerCount('remote-handler')).to.equal(2)
      let warnMessage: string | null = null
      const originalWarn = console.warn
      let warned: Function
      const warnPromise = new Promise(resolve => {
        warned = resolve
      })
      try {
        console.warn = (message: string) => {
          warnMessage = message
          warned()
        }
        w.webContents.emit('remote-handler', { sender: w.webContents })
        await warnPromise
      } finally {
        console.warn = originalWarn
      }
      expect(w.webContents.listenerCount('remote-handler')).to.equal(1)
      expect(warnMessage).to.equal(expectedMessage)
    })
  })

  describe('remote.require', () => {
    const w = makeWindow()
    const remotely = makeRemotely(w)

    remotely.it()('should returns same object for the same module', () => {
      const { remote } = require('electron')
      const a = remote.require('electron')
      const b = remote.require('electron')
      expect(a).to.equal(b)
    })

    remotely.it(path.join(fixtures, 'module', 'id.js'))('should work when object contains id property', (module: string) => {
      const { id } = require('electron').remote.require(module)
      expect(id).to.equal(1127)
    })

    remotely.it(path.join(fixtures, 'module', 'no-prototype.js'))('should work when object has no prototype', (module: string) => {
      const a = require('electron').remote.require(module)
      expect(a.foo.constructor.name).to.equal('')
      expect(a.foo.bar).to.equal('baz')
      expect(a.foo.baz).to.equal(false)
      expect(a.bar).to.equal(1234)
      expect(a.anonymous.constructor.name).to.equal('')
      expect(a.getConstructorName(Object.create(null))).to.equal('')
      expect(a.getConstructorName(new (class {})())).to.equal('')
    })

    it('should search module from the user app', async () => {
      expectPathsEqual(
        path.normalize(await remotely(() => require('electron').remote.process.mainModule!.filename)),
        path.resolve(__dirname, 'index.js')
      )
      expectPathsEqual(
        path.normalize(await remotely(() => require('electron').remote.process.mainModule!.paths[0])),
        path.resolve(__dirname, 'node_modules')
      )
    })

    remotely.it(fixtures)('should work with function properties', (fixtures: string) => {
      const path = require('path')

      {
        const a = require('electron').remote.require(path.join(fixtures, 'module', 'export-function-with-properties.js'))
        expect(typeof a).to.equal('function')
        expect(a.bar).to.equal('baz')
      }

      {
        const a = require('electron').remote.require(path.join(fixtures, 'module', 'function-with-properties.js'))
        expect(typeof a).to.equal('object')
        expect(a.foo()).to.equal('hello')
        expect(a.foo.bar).to.equal('baz')
        expect(a.foo.nested.prop).to.equal('yes')
        expect(a.foo.method1()).to.equal('world')
        expect(a.foo.method1.prop1()).to.equal(123)
      }

      {
        const a = require('electron').remote.require(path.join(fixtures, 'module', 'function-with-missing-properties.js')).setup()
        expect(a.bar()).to.equal(true)
        expect(a.bar.baz).to.be.undefined()
      }
    })

    remotely.it(fixtures)('should work with static class members', (fixtures: string) => {
      const path = require('path')
      const a = require('electron').remote.require(path.join(fixtures, 'module', 'remote-static.js'))
      expect(typeof a.Foo).to.equal('function')
      expect(a.Foo.foo()).to.equal(3)
      expect(a.Foo.bar).to.equal('baz')
      expect(new a.Foo().baz()).to.equal(123)
    })

    remotely.it(fixtures)('includes the length of functions specified as arguments', (fixtures: string) => {
      const path = require('path')
      const a = require('electron').remote.require(path.join(fixtures, 'module', 'function-with-args.js'))
      /* eslint-disable @typescript-eslint/no-unused-vars */
      expect(a((a: any, b: any, c: any) => {})).to.equal(3)
      expect(a((a: any) => {})).to.equal(1)
      expect(a((...args: any[]) => {})).to.equal(0)
      /* eslint-enable @typescript-eslint/no-unused-vars */
    })

    remotely.it(fixtures)('handles circular references in arrays and objects', (fixtures: string) => {
      const path = require('path')
      const a = require('electron').remote.require(path.join(fixtures, 'module', 'circular.js'))

      let arrayA: any[] = ['foo']
      const arrayB = [arrayA, 'bar']
      arrayA.push(arrayB)
      expect(a.returnArgs(arrayA, arrayB)).to.deep.equal([
        ['foo', [null, 'bar']],
        [['foo', null], 'bar']
      ])

      let objectA: any = { foo: 'bar' }
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
    const remotely = makeRemotely(makeWindow())

    remotely.it(fixtures)('should be called in browser synchronously', async (fixtures: string) => {
      const { remote } = require('electron')
      const path = require('path')
      const buf = Buffer.from('test')
      const call = remote.require(path.join(fixtures, 'module', 'call.js'))
      const result = call.call((remote as any).createFunctionWithReturnValue(buf))
      expect(result).to.be.an.instanceOf(Uint8Array)
    })
  })

  describe('remote modules', () => {
    const remotely = makeRemotely(makeWindow())

    const mainModules = Object.keys(require('electron'))
    remotely.it(mainModules)('includes browser process modules as properties', (mainModules: string[]) => {
      const { remote } = require('electron')
      const remoteModules = mainModules.filter(name => (remote as any)[name])
      expect(remoteModules).to.be.deep.equal(mainModules)
    })

    remotely.it(fixtures)('returns toString() of original function via toString()', (fixtures: string) => {
      const path = require('path')
      const { readText } = require('electron').remote.clipboard
      expect(readText.toString().startsWith('function')).to.be.true()

      const { functionWithToStringProperty } = require('electron').remote.require(path.join(fixtures, 'module', 'to-string-non-function.js'))
      expect(functionWithToStringProperty.toString).to.equal('hello')
    })
  })

  describe('remote object in renderer', () => {
    const remotely = makeRemotely(makeWindow())

    remotely.it(fixtures)('can change its properties', (fixtures: string) => {
      const module = require('path').join(fixtures, 'module', 'property.js')
      const property = require('electron').remote.require(module)
      expect(property.property).to.equal(1127)
      property.property = null
      expect(property.property).to.equal(null)
      property.property = undefined
      expect(property.property).to.equal(undefined)
      property.property = 1007
      expect(property.property).to.equal(1007)

      expect(property.getFunctionProperty()).to.equal('foo-browser')
      property.func.property = 'bar'
      expect(property.getFunctionProperty()).to.equal('bar-browser')
      property.func.property = 'foo' // revert back

      const property2 = require('electron').remote.require(module)
      expect(property2.property).to.equal(1007)

      property.property = 1127 // revert back
    })

    remotely.it(fixtures)('rethrows errors getting/setting properties', (fixtures: string) => {
      const foo = require('electron').remote.require(require('path').join(fixtures, 'module', 'error-properties.js'))

      expect(() => {
        // eslint-disable-next-line
        foo.bar
      }).to.throw('getting error')

      expect(() => {
        foo.bar = 'test'
      }).to.throw('setting error')
    })

    remotely.it(fixtures)('can set a remote property with a remote object', (fixtures: string) => {
      const { remote } = require('electron')
      const foo = remote.require(require('path').join(fixtures, 'module', 'remote-object-set.js'))
      foo.bar = remote.getCurrentWindow()
    })

    remotely.it(fixtures)('can construct an object from its member', (fixtures: string) => {
      const call = require('electron').remote.require(require('path').join(fixtures, 'module', 'call.js'))
      const obj = new call.constructor()
      expect(obj.test).to.equal('test')
    })

    remotely.it(fixtures)('can reassign and delete its member functions', (fixtures: string) => {
      const remoteFunctions = require('electron').remote.require(require('path').join(fixtures, 'module', 'function.js'))
      expect(remoteFunctions.aFunction()).to.equal(1127)

      remoteFunctions.aFunction = () => { return 1234 }
      expect(remoteFunctions.aFunction()).to.equal(1234)

      expect(delete remoteFunctions.aFunction).to.equal(true)
    })

    remotely.it('is referenced by its members', () => {
      const stringify = require('electron').remote.getGlobal('JSON').stringify
      global.gc()
      stringify({})
    })
  })

  describe('remote value in browser', () => {
    const remotely = makeRemotely(makeWindow())
    const print = path.join(fixtures, 'module', 'print_name.js')

    remotely.it(print)('preserves NaN', (print: string) => {
      const printName = require('electron').remote.require(print)
      expect(printName.getNaN()).to.be.NaN()
      expect(printName.echo(NaN)).to.be.NaN()
    })

    remotely.it(print)('preserves Infinity', (print: string) => {
      const printName = require('electron').remote.require(print)
      expect(printName.getInfinity()).to.equal(Infinity)
      expect(printName.echo(Infinity)).to.equal(Infinity)
    })

    remotely.it(print)('keeps its constructor name for objects', (print: string) => {
      const printName = require('electron').remote.require(print)
      const buf = Buffer.from('test')
      expect(printName.print(buf)).to.equal('Buffer')
    })

    remotely.it(print)('supports instanceof Boolean', (print: string) => {
      const printName = require('electron').remote.require(print)
      const obj = Boolean(true)
      expect(printName.print(obj)).to.equal('Boolean')
      expect(printName.echo(obj)).to.deep.equal(obj)
    })

    remotely.it(print)('supports instanceof Number', (print: string) => {
      const printName = require('electron').remote.require(print)
      const obj = Number(42)
      expect(printName.print(obj)).to.equal('Number')
      expect(printName.echo(obj)).to.deep.equal(obj)
    })

    remotely.it(print)('supports instanceof String', (print: string) => {
      const printName = require('electron').remote.require(print)
      const obj = String('Hello World!')
      expect(printName.print(obj)).to.equal('String')
      expect(printName.echo(obj)).to.deep.equal(obj)
    })

    remotely.it(print)('supports instanceof Date', (print: string) => {
      const printName = require('electron').remote.require(print)
      const now = new Date()
      expect(printName.print(now)).to.equal('Date')
      expect(printName.echo(now)).to.deep.equal(now)
    })

    remotely.it(print)('supports instanceof RegExp', (print: string) => {
      const printName = require('electron').remote.require(print)
      const regexp = RegExp('.*')
      expect(printName.print(regexp)).to.equal('RegExp')
      expect(printName.echo(regexp)).to.deep.equal(regexp)
    })

    remotely.it(print)('supports instanceof Buffer', (print: string) => {
      const printName = require('electron').remote.require(print)
      const buffer = Buffer.from('test')
      expect(buffer.equals(printName.echo(buffer))).to.be.true()

      const objectWithBuffer = { a: 'foo', b: Buffer.from('bar') }
      expect(objectWithBuffer.b.equals(printName.echo(objectWithBuffer).b)).to.be.true()

      const arrayWithBuffer = [1, 2, Buffer.from('baz')]
      expect((arrayWithBuffer[2] as Buffer).equals(printName.echo(arrayWithBuffer)[2])).to.be.true()
    })

    remotely.it(print)('supports instanceof ArrayBuffer', (print: string) => {
      const printName = require('electron').remote.require(print)
      const buffer = new ArrayBuffer(8)
      const view = new DataView(buffer)

      view.setFloat64(0, Math.PI)
      expect(printName.echo(buffer)).to.deep.equal(buffer)
      expect(printName.print(buffer)).to.equal('ArrayBuffer')
    })

    const arrayTests: [string, number[]][] = [
      ['Int8Array', [1, 2, 3, 4]],
      ['Uint8Array', [1, 2, 3, 4]],
      ['Uint8ClampedArray', [1, 2, 3, 4]],
      ['Int16Array', [0x1234, 0x2345, 0x3456, 0x4567]],
      ['Uint16Array', [0x1234, 0x2345, 0x3456, 0x4567]],
      ['Int32Array', [0x12345678, 0x23456789]],
      ['Uint32Array', [0x12345678, 0x23456789]],
      ['Float32Array', [0.5, 1.0, 1.5]],
      ['Float64Array', [0.5, 1.0, 1.5]]
    ]

    arrayTests.forEach(([arrayType, values]) => {
      remotely.it(print, arrayType, values)(`supports instanceof ${arrayType}`, (print: string, arrayType: string, values: number[]) => {
        const printName = require('electron').remote.require(print)
        expect([...printName.typedArray(arrayType, values)]).to.deep.equal(values)

        const int8values = new ((window as any)[arrayType])(values)
        expect(printName.typedArray(arrayType, int8values)).to.deep.equal(int8values)
        expect(printName.print(int8values)).to.equal(arrayType)
      })
    })

    describe('constructing a Uint8Array', () => {
      remotely.it()('does not crash', () => {
        const RUint8Array = require('electron').remote.getGlobal('Uint8Array')
        new RUint8Array() // eslint-disable-line
      })
    })
  })

  describe('remote promise', () => {
    const remotely = makeRemotely(makeWindow())

    remotely.it(fixtures)('can be used as promise in each side', async (fixtures: string) => {
      const promise = require('electron').remote.require(require('path').join(fixtures, 'module', 'promise.js'))
      const value = await promise.twicePromise(Promise.resolve(1234))
      expect(value).to.equal(2468)
    })

    remotely.it(fixtures)('handles rejections via catch(onRejected)', async (fixtures: string) => {
      const promise = require('electron').remote.require(require('path').join(fixtures, 'module', 'rejected-promise.js'))
      const error = await new Promise<Error>(resolve => {
        promise.reject(Promise.resolve(1234)).catch(resolve)
      })
      expect(error.message).to.equal('rejected')
    })

    remotely.it(fixtures)('handles rejections via then(onFulfilled, onRejected)', async (fixtures: string) => {
      const promise = require('electron').remote.require(require('path').join(fixtures, 'module', 'rejected-promise.js'))
      const error = await new Promise<Error>(resolve => {
        promise.reject(Promise.resolve(1234)).then(() => {}, resolve)
      })
      expect(error.message).to.equal('rejected')
    })

    it('does not emit unhandled rejection events in the main process', (done) => {
      function onUnhandledRejection () {
        done(new Error('Unexpected unhandledRejection event'))
      }
      process.once('unhandledRejection', onUnhandledRejection)

      remotely(async (fixtures: string) => {
        const promise = require('electron').remote.require(require('path').join(fixtures, 'module', 'unhandled-rejection.js'))
        return new Promise((resolve, reject) => {
          promise.reject().then(() => {
            reject(new Error('Promise was not rejected'))
          }).catch((error: Error) => {
            resolve(error)
          })
        })
      }, fixtures).then(error => {
        try {
          expect(error.message).to.equal('rejected')
          done()
        } catch (e) {
          done(e)
        } finally {
          process.off('unhandledRejection', onUnhandledRejection)
        }
      })
    })

    it('emits unhandled rejection events in the renderer process', (done) => {
      remotely((module: string) => new Promise((resolve, reject) => {
        const promise = require('electron').remote.require(module)

        window.addEventListener('unhandledrejection', function handler (event) {
          event.preventDefault()
          window.removeEventListener('unhandledrejection', handler)
          resolve(event.reason.message)
        })

        promise.reject().then(() => {
          reject(new Error('Promise was not rejected'))
        })
      }), path.join(fixtures, 'module', 'unhandled-rejection.js')).then(
        (message) => {
          try {
            expect(message).to.equal('rejected')
            done()
          } catch (e) {
            done(e)
          }
        },
        done
      )
    })

    before(() => {
      (global as any).returnAPromise = (value: any) => new Promise((resolve) => setTimeout(() => resolve(value), 100))
    })
    after(() => {
      delete (global as any).returnAPromise
    })
    remotely.it()('using a promise based method resolves correctly when global Promise is overridden', async () => {
      const { remote } = require('electron')
      const original = global.Promise
      try {
        expect(await remote.getGlobal('returnAPromise')(123)).to.equal(123)
        global.Promise = { resolve: () => ({}) } as any
        expect(await remote.getGlobal('returnAPromise')(456)).to.equal(456)
      } finally {
        global.Promise = original
      }
    })
  })

  describe('remote webContents', () => {
    const remotely = makeRemotely(makeWindow())

    it('can return same object with different getters', async () => {
      const equal = await remotely(() => {
        const { remote } = require('electron')
        const contents1 = remote.getCurrentWindow().webContents
        const contents2 = remote.getCurrentWebContents()
        return contents1 === contents2
      })
      expect(equal).to.be.true()
    })
  })

  describe('remote class', () => {
    const remotely = makeRemotely(makeWindow())

    remotely.it(fixtures)('can get methods', (fixtures: string) => {
      const { base } = require('electron').remote.require(require('path').join(fixtures, 'module', 'class.js'))
      expect(base.method()).to.equal('method')
    })

    remotely.it(fixtures)('can get properties', (fixtures: string) => {
      const { base } = require('electron').remote.require(require('path').join(fixtures, 'module', 'class.js'))
      expect(base.readonly).to.equal('readonly')
    })

    remotely.it(fixtures)('can change properties', (fixtures: string) => {
      const { base } = require('electron').remote.require(require('path').join(fixtures, 'module', 'class.js'))
      expect(base.value).to.equal('old')
      base.value = 'new'
      expect(base.value).to.equal('new')
      base.value = 'old'
    })

    remotely.it(fixtures)('has unenumerable methods', (fixtures: string) => {
      const { base } = require('electron').remote.require(require('path').join(fixtures, 'module', 'class.js'))
      expect(base).to.not.have.ownProperty('method')
      expect(Object.getPrototypeOf(base)).to.have.ownProperty('method')
    })

    remotely.it(fixtures)('keeps prototype chain in derived class', (fixtures: string) => {
      const { derived } = require('electron').remote.require(require('path').join(fixtures, 'module', 'class.js'))
      expect(derived.method()).to.equal('method')
      expect(derived.readonly).to.equal('readonly')
      expect(derived).to.not.have.ownProperty('method')
      const proto = Object.getPrototypeOf(derived)
      expect(proto).to.not.have.ownProperty('method')
      expect(Object.getPrototypeOf(proto)).to.have.ownProperty('method')
    })

    remotely.it(fixtures)('is referenced by methods in prototype chain', (fixtures: string) => {
      let { derived } = require('electron').remote.require(require('path').join(fixtures, 'module', 'class.js'))
      const method = derived.method
      derived = null
      global.gc()
      expect(method()).to.equal('method')
    })
  })

  describe('remote exception', () => {
    const remotely = makeRemotely(makeWindow())

    remotely.it(fixtures)('throws errors from the main process', (fixtures: string) => {
      const throwFunction = require('electron').remote.require(require('path').join(fixtures, 'module', 'exception.js'))
      expect(() => {
        throwFunction()
      }).to.throw(/undefined/)
    })

    remotely.it(fixtures)('tracks error cause', (fixtures: string) => {
      const throwFunction = require('electron').remote.require(require('path').join(fixtures, 'module', 'exception.js'))
      try {
        throwFunction(new Error('error from main'))
        expect.fail()
      } catch (e) {
        expect(e.message).to.match(/Could not call remote function/)
        expect(e.cause.message).to.equal('error from main')
      }
    })
  })
})

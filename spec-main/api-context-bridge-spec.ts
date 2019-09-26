import { contextBridge, BrowserWindow, ipcMain } from 'electron'
import { expect } from 'chai'
import * as fs from 'fs-extra'
import * as os from 'os'
import * as path from 'path'

import { closeWindow } from './window-helpers'
import { emittedOnce } from './events-helpers'

const fixturesPath = path.resolve(__dirname, 'fixtures', 'api', 'context-bridge')

describe('contextBridge', () => {
  let w: BrowserWindow
  let dir: string

  afterEach(async () => {
    await closeWindow(w)
    if (dir) await fs.remove(dir)
  })

  it('should not be accessible when contextIsolation is disabled', async () => {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        contextIsolation: false,
        preload: path.resolve(fixturesPath, 'can-bind-preload.js')
      }
    })
    const [,bound] = await emittedOnce(ipcMain, 'context-bridge-bound', () => w.loadFile(path.resolve(fixturesPath, 'empty.html')))
    expect(bound).to.equal(false)
  })

  it('should be accessible when contextIsolation is enabled', async () => {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        contextIsolation: true,
        preload: path.resolve(fixturesPath, 'can-bind-preload.js')
      }
    })
    const [,bound] = await emittedOnce(ipcMain, 'context-bridge-bound', () => w.loadFile(path.resolve(fixturesPath, 'empty.html')))
    expect(bound).to.equal(true)
  })

  const makeBindingWindow = async (bindingCreator: Function) => {
    const preloadContent = `const electron_1 = require('electron');(${bindingCreator.toString()})()`
    const tmpDir = await fs.mkdtemp(path.resolve(os.tmpdir(), 'electron-spec-preload-'))
    dir = tmpDir
    await fs.writeFile(path.resolve(tmpDir, 'preload.js'), preloadContent)
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        contextIsolation: true,
        preload: path.resolve(tmpDir, 'preload.js')
      }
    })
    await w.loadFile(path.resolve(fixturesPath, 'empty.html'))
  }

  const callWithBindings = async (fn: Function) => {
    return await w.webContents.executeJavaScript(`(${fn.toString()})(window)`)
  }

  it('should proxy numbers', async () => {
    await makeBindingWindow(() => {
      contextBridge.bindAPIInMainWorld('example', {
        myNumber: 123,
      })
    })
    const result = await callWithBindings((root: any) => {
      return root.example.myNumber
    })
    expect(result).to.equal(123)
  })

  it('should make properties unwriteable', async () => {
    await makeBindingWindow(() => {
      contextBridge.bindAPIInMainWorld('example', {
        myNumber: 123,
      })
    })
    const result = await callWithBindings((root: any) => {
      root.example.myNumber = 456
      return root.example.myNumber
    })
    expect(result).to.equal(123)
  })

  it('should proxy strings', async () => {
    await makeBindingWindow(() => {
      contextBridge.bindAPIInMainWorld('example', {
        myString: 'my-words',
      })
    })
    const result = await callWithBindings((root: any) => {
      return root.example.myString
    })
    expect(result).to.equal('my-words')
  })

  it('should proxy arrays', async () => {
    await makeBindingWindow(() => {
      contextBridge.bindAPIInMainWorld('example', {
        myArr: [123, 'my-words'],
      })
    })
    const result = await callWithBindings((root: any) => {
      return root.example.myArr
    })
    expect(result).to.deep.equal([123, 'my-words'])
  })

  it('should make arrays immutable', async () => {
    await makeBindingWindow(() => {
      contextBridge.bindAPIInMainWorld('example', {
        myArr: [123, 'my-words'],
      })
    })
    const immutable = await callWithBindings((root: any) => {
      try {
        root.example.myArr.push(456)
        return false
      } catch {
        return true
      }
    })
    expect(immutable).to.equal(true)
  })

  it('should proxy booleans', async () => {
    await makeBindingWindow(() => {
      contextBridge.bindAPIInMainWorld('example', {
        myBool: true,
      })
    })
    const result = await callWithBindings((root: any) => {
      return root.example.myBool
    })
    expect(result).to.equal(true)
  })

  it('should proxy promises and resolve with the correct value', async () => {
    await makeBindingWindow(() => {
      contextBridge.bindAPIInMainWorld('example', {
        myPromise: Promise.resolve('i-resolved'),
      })
    })
    const result = await callWithBindings(async (root: any) => {
      return await root.example.myPromise
    })
    expect(result).to.equal('i-resolved')
  })

  it('should proxy promises and reject with the correct value', async () => {
    await makeBindingWindow(() => {
      contextBridge.bindAPIInMainWorld('example', {
        myPromise: Promise.reject('i-rejected'),
      })
    })
    const result = await callWithBindings(async (root: any) => {
      try {
        await root.example.myPromise
        return null
      } catch (err) {
        return err
      }
    })
    expect(result).to.equal('i-rejected')
  })

  it('should proxy promises and resolve with the correct value if it resolves later', async () => {
    await makeBindingWindow(() => {
      contextBridge.bindAPIInMainWorld('example', {
        myPromise: () => new Promise(r => setTimeout(() => r('delayed'), 20)),
      })
    })
    const result = await callWithBindings(async (root: any) => {
      return await root.example.myPromise()
    })
    expect(result).to.equal('delayed')
  })

  it('should proxy nested promises correctly', async () => {
    await makeBindingWindow(() => {
      contextBridge.bindAPIInMainWorld('example', {
        myPromise: () => new Promise(r => setTimeout(() => r(Promise.resolve(123)), 20)),
      })
    })
    const result = await callWithBindings(async (root: any) => {
      return await root.example.myPromise()
    })
    expect(result).to.equal(123)
  })

  it('should proxy methods', async () => {
    await makeBindingWindow(() => {
      contextBridge.bindAPIInMainWorld('example', {
        getNumber: () => 123,
        getString: () => 'help',
        getBoolean: () => false,
        getPromise: async () => 'promise'
      })
    })
    const result = await callWithBindings(async (root: any) => {
      return [root.example.getNumber(), root.example.getString(), root.example.getBoolean(), await root.example.getPromise()]
    })
    expect(result).to.deep.equal([123, 'help', false, 'promise'])
  })

  it('should proxy methods in the reverse direction', async () => {
    await makeBindingWindow(() => {
      contextBridge.bindAPIInMainWorld('example', {
        callWithNumber: (fn: any) => fn(123),
      })
    })
    const result = await callWithBindings(async (root: any) => {
      return root.example.callWithNumber((n: number) => n + 1)
    })
    expect(result).to.equal(124)
  })

  it('should proxy promises in the reverse direction', async () => {
    await makeBindingWindow(() => {
      contextBridge.bindAPIInMainWorld('example', {
        getPromiseValue: async (p: Promise<any>) => await p,
      })
    })
    const result = await callWithBindings(async (root: any) => {
      return await root.example.getPromiseValue(Promise.resolve('my-proxied-value'))
    })
    expect(result).to.equal('my-proxied-value')
  })

  it('should not leak prototypes', async () => {
    await makeBindingWindow(() => {
      contextBridge.bindAPIInMainWorld('example', {
        number: 123,
        string: 'string',
        boolean: true,
        arr: [123, 'string', true, ['foo']],
        getNumber: () => 123,
        getString: () => 'string',
        getBoolean: () => true,
        getArr: () => [123, 'string', true, ['foo']],
        getPromise: async () => ({ number: 123, string: 'string', boolean: true, fn: () => 'string', arr: [123, 'string', true, ['foo']]}),
        getFunctionFromFunction: async () => () => null,
        object: {
          number: 123,
          string: 'string',
          boolean: true,
          arr: [123, 'string', true, ['foo']],
          getPromise: async () => ({ number: 123, string: 'string', boolean: true, fn: () => 'string', arr: [123, 'string', true, ['foo']]}),
        },
        receiveArguments: (fn: any) => fn({ key: 'value' })
      })
    })
    const result = await callWithBindings(async (root: any) => {
      const { example } = root
      let arg: any
      example.receiveArguments((o: any) => { arg = o })
      const protoChecks = [
        [example, Object],
        [example.number, Number],
        [example.string, String],
        [example.boolean, Boolean],
        [example.arr, Array],
        [example.arr[0], Number],
        [example.arr[1], String],
        [example.arr[2], Boolean],
        [example.arr[3], Array],
        [example.arr[3][0], String],
        [example.getNumber, Function],
        [example.getNumber(), Number],
        [example.getString(), String],
        [example.getBoolean(), Boolean],
        [example.getArr(), Array],
        [example.getArr()[0], Number],
        [example.getArr()[1], String],
        [example.getArr()[2], Boolean],
        [example.getArr()[3], Array],
        [example.getArr()[3][0], String],
        [example.getFunctionFromFunction, Function],
        [example.getFunctionFromFunction(), Promise],
        [await example.getFunctionFromFunction(), Function],
        [example.getPromise(), Promise],
        [await example.getPromise(), Object],
        [(await example.getPromise()).number, Number],
        [(await example.getPromise()).string, String],
        [(await example.getPromise()).boolean, Boolean],
        [(await example.getPromise()).fn, Function],
        [(await example.getPromise()).fn(), String],
        [(await example.getPromise()).arr, Array],
        [(await example.getPromise()).arr[0], Number],
        [(await example.getPromise()).arr[1], String],
        [(await example.getPromise()).arr[2], Boolean],
        [(await example.getPromise()).arr[3], Array],
        [(await example.getPromise()).arr[3][0], String],
        [example.object, Object],
        [example.object.number, Number],
        [example.object.string, String],
        [example.object.boolean, Boolean],
        [example.object.arr, Array],
        [example.object.arr[0], Number],
        [example.object.arr[1], String],
        [example.object.arr[2], Boolean],
        [example.object.arr[3], Array],
        [example.object.arr[3][0], String],
        [await example.object.getPromise(), Object],
        [(await example.object.getPromise()).number, Number],
        [(await example.object.getPromise()).string, String],
        [(await example.object.getPromise()).boolean, Boolean],
        [(await example.object.getPromise()).fn, Function],
        [(await example.object.getPromise()).fn(), String],
        [(await example.object.getPromise()).arr, Array],
        [(await example.object.getPromise()).arr[0], Number],
        [(await example.object.getPromise()).arr[1], String],
        [(await example.object.getPromise()).arr[2], Boolean],
        [(await example.object.getPromise()).arr[3], Array],
        [(await example.object.getPromise()).arr[3][0], String],
        [arg, Object],
        [arg.key, String]
      ]
      return {
        protoMatches: protoChecks.map(([a, Constructor]) => a.__proto__ === Constructor.prototype)
      }
    })
    // Every protomatch should be true
    expect(result.protoMatches).to.deep.equal(result.protoMatches.map(() => true))
  })
})

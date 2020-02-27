import { EventEmitter } from 'events'
import { expect } from 'chai'
import { BrowserWindow, ipcMain, IpcMainInvokeEvent } from 'electron'
import { closeAllWindows } from './window-helpers'
import { emittedOnce } from './events-helpers'

describe('ipc module', () => {
  describe('invoke', () => {
    let w = (null as unknown as BrowserWindow)

    before(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      await w.loadURL('about:blank')
    })
    after(async () => {
      w.destroy()
    })

    async function rendererInvoke (...args: any[]) {
      const { ipcRenderer } = require('electron')
      try {
        const result = await ipcRenderer.invoke('test', ...args)
        ipcRenderer.send('result', { result })
      } catch (e) {
        ipcRenderer.send('result', { error: e.message })
      }
    }

    it('receives a response from a synchronous handler', async () => {
      ipcMain.handleOnce('test', (e: IpcMainInvokeEvent, arg: number) => {
        expect(arg).to.equal(123)
        return 3
      })
      const done = new Promise(resolve => ipcMain.once('result', (e, arg) => {
        expect(arg).to.deep.equal({ result: 3 })
        resolve()
      }))
      await w.webContents.executeJavaScript(`(${rendererInvoke})(123)`)
      await done
    })

    it('receives a response from an asynchronous handler', async () => {
      ipcMain.handleOnce('test', async (e: IpcMainInvokeEvent, arg: number) => {
        expect(arg).to.equal(123)
        await new Promise(resolve => setImmediate(resolve))
        return 3
      })
      const done = new Promise(resolve => ipcMain.once('result', (e, arg) => {
        expect(arg).to.deep.equal({ result: 3 })
        resolve()
      }))
      await w.webContents.executeJavaScript(`(${rendererInvoke})(123)`)
      await done
    })

    it('receives an error from a synchronous handler', async () => {
      ipcMain.handleOnce('test', () => {
        throw new Error('some error')
      })
      const done = new Promise(resolve => ipcMain.once('result', (e, arg) => {
        expect(arg.error).to.match(/some error/)
        resolve()
      }))
      await w.webContents.executeJavaScript(`(${rendererInvoke})()`)
      await done
    })

    it('receives an error from an asynchronous handler', async () => {
      ipcMain.handleOnce('test', async () => {
        await new Promise(resolve => setImmediate(resolve))
        throw new Error('some error')
      })
      const done = new Promise(resolve => ipcMain.once('result', (e, arg) => {
        expect(arg.error).to.match(/some error/)
        resolve()
      }))
      await w.webContents.executeJavaScript(`(${rendererInvoke})()`)
      await done
    })

    it('throws an error if no handler is registered', async () => {
      const done = new Promise(resolve => ipcMain.once('result', (e, arg) => {
        expect(arg.error).to.match(/No handler registered/)
        resolve()
      }))
      await w.webContents.executeJavaScript(`(${rendererInvoke})()`)
      await done
    })

    it('throws an error when invoking a handler that was removed', async () => {
      ipcMain.handle('test', () => {})
      ipcMain.removeHandler('test')
      const done = new Promise(resolve => ipcMain.once('result', (e, arg) => {
        expect(arg.error).to.match(/No handler registered/)
        resolve()
      }))
      await w.webContents.executeJavaScript(`(${rendererInvoke})()`)
      await done
    })

    it('forbids multiple handlers', async () => {
      ipcMain.handle('test', () => {})
      try {
        expect(() => { ipcMain.handle('test', () => {}) }).to.throw(/second handler/)
      } finally {
        ipcMain.removeHandler('test')
      }
    })
  })

  describe('ordering', () => {
    let w = (null as unknown as BrowserWindow)

    before(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      await w.loadURL('about:blank')
    })
    after(async () => {
      w.destroy()
    })

    it('between send and sendSync is consistent', async () => {
      const received: number[] = []
      ipcMain.on('test-async', (e, i) => { received.push(i) })
      ipcMain.on('test-sync', (e, i) => { received.push(i); e.returnValue = null })
      const done = new Promise(resolve => ipcMain.once('done', () => { resolve() }))
      function rendererStressTest () {
        const { ipcRenderer } = require('electron')
        for (let i = 0; i < 1000; i++) {
          switch ((Math.random() * 2) | 0) {
            case 0:
              ipcRenderer.send('test-async', i)
              break
            case 1:
              ipcRenderer.sendSync('test-sync', i)
              break
          }
        }
        ipcRenderer.send('done')
      }
      try {
        w.webContents.executeJavaScript(`(${rendererStressTest})()`)
        await done
      } finally {
        ipcMain.removeAllListeners('test-async')
        ipcMain.removeAllListeners('test-sync')
      }
      expect(received).to.have.lengthOf(1000)
      expect(received).to.deep.equal([...received].sort((a, b) => a - b))
    })

    it('between send, sendSync, and invoke is consistent', async () => {
      const received: number[] = []
      ipcMain.handle('test-invoke', (e, i) => { received.push(i) })
      ipcMain.on('test-async', (e, i) => { received.push(i) })
      ipcMain.on('test-sync', (e, i) => { received.push(i); e.returnValue = null })
      const done = new Promise(resolve => ipcMain.once('done', () => { resolve() }))
      function rendererStressTest () {
        const { ipcRenderer } = require('electron')
        for (let i = 0; i < 1000; i++) {
          switch ((Math.random() * 3) | 0) {
            case 0:
              ipcRenderer.send('test-async', i)
              break
            case 1:
              ipcRenderer.sendSync('test-sync', i)
              break
            case 2:
              ipcRenderer.invoke('test-invoke', i)
              break
          }
        }
        ipcRenderer.send('done')
      }
      try {
        w.webContents.executeJavaScript(`(${rendererStressTest})()`)
        await done
      } finally {
        ipcMain.removeHandler('test-invoke')
        ipcMain.removeAllListeners('test-async')
        ipcMain.removeAllListeners('test-sync')
      }
      expect(received).to.have.lengthOf(1000)
      expect(received).to.deep.equal([...received].sort((a, b) => a - b))
    })
  })

  describe('MessagePort', () => {
    afterEach(closeAllWindows)
    it('can send a port to the main process', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      w.loadURL('about:blank')
      const p = emittedOnce(ipcMain, 'port')
      await w.webContents.executeJavaScript(`(${function () {
        const channel = new MessageChannel()
        require('electron').ipcRenderer.postMessage('port', 'hi', [channel.port1])
      }})()`)
      const [ev, msg] = await p
      expect(msg).to.equal('hi')
      expect(ev.ports).to.have.length(1)
      const [port] = ev.ports
      expect(port).to.be.an.instanceOf(EventEmitter)
    })

    it('can communicate between main and renderer', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      w.loadURL('about:blank')
      const p = emittedOnce(ipcMain, 'port')
      await w.webContents.executeJavaScript(`(${function () {
        const channel = new MessageChannel()
        channel.port2.onmessage = (ev) => {
          channel.port2.postMessage(ev.data * 2)
        }
        require('electron').ipcRenderer.postMessage('port', '', [channel.port1])
      }})()`)
      const [ev] = await p
      expect(ev.ports).to.have.length(1)
      const [port] = ev.ports
      port.start()
      port.postMessage(42)
      const [ev2] = await emittedOnce(port, 'message')
      expect(ev2.data).to.equal(84)
    })

    it('can send a port created in the main process to a renderer')
    it('can receive a port from a renderer over a MessagePort connection', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      w.loadURL('about:blank')
      function fn () {
        const channel1 = new MessageChannel()
        const channel2 = new MessageChannel()
        channel1.port2.postMessage('', [channel2.port1])
        channel2.port2.postMessage('matryoshka')
        require('electron').ipcRenderer.postMessage('port', '', [channel1.port1])
      }
      w.webContents.executeJavaScript(`(${fn})()`)
      const [{ ports: [port1] }] = await emittedOnce(ipcMain, 'port')
      port1.start()
      const [{ ports: [port2] }] = await emittedOnce(port1, 'message')
      port2.start()
      const [{ data }] = await emittedOnce(port2, 'message')
      expect(data).to.equal('matryoshka')
    })
    it('can send a port to a renderer over a MessagePort connection')
    it('can forward a port from one renderer to another renderer', async () => {
      const w1 = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      const w2 = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      w1.loadURL('about:blank')
      w2.loadURL('about:blank')
      w1.webContents.executeJavaScript(`(${function () {
        const channel = new MessageChannel()
        channel.port2.onmessage = (ev) => {
          require('electron').ipcRenderer.send('message received', ev.data)
        }
        require('electron').ipcRenderer.postMessage('port', '', [channel.port1])
      }})()`)
      w2.webContents.executeJavaScript(`(${function () {
        require('electron').ipcRenderer.on('port', ({ ports: [port] }: any) => {
          port.postMessage('a message')
        })
      }})()`)
      const [{ ports: [port] }] = await emittedOnce(ipcMain, 'port')
      w2.webContents.postMessage('port', '', [port])
      const [, data] = await emittedOnce(ipcMain, 'message received')
      expect(data).to.equal('a message')
    })
  })
})

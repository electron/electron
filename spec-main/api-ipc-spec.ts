import * as chai from 'chai'
import * as chaiAsPromised from 'chai-as-promised'
import { BrowserWindow, ipcMain, IpcMainInvokeEvent } from 'electron'

const { expect } = chai

chai.use(chaiAsPromised)

describe('ipc module', () => {
  describe('invoke', () => {
    let w = (null as unknown as BrowserWindow);

    before(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      await w.loadURL('about:blank')
    })
    after(async () => {
      w.destroy()
    })

    async function rendererInvoke(...args: any[]) {
      const {ipcRenderer} = require('electron')
      try {
        const result = await ipcRenderer.invoke('test', ...args)
        ipcRenderer.send('result', {result})
      } catch (e) {
        ipcRenderer.send('result', {error: e.message})
      }
    }

    it('receives a response from a synchronous handler', async () => {
      ipcMain.handleOnce('test', (e: IpcMainInvokeEvent, arg: number) => {
        expect(arg).to.equal(123)
        return 3
      })
      const done = new Promise(resolve => ipcMain.once('result', (e, arg) => {
        expect(arg).to.deep.equal({result: 3})
        resolve()
      }))
      await w.webContents.executeJavaScript(`(${rendererInvoke})(123)`)
      await done
    })

    it('receives a response from an asynchronous handler', async () => {
      ipcMain.handleOnce('test', async (e: IpcMainInvokeEvent, arg: number) => {
        expect(arg).to.equal(123)
        await new Promise(setImmediate)
        return 3
      })
      const done = new Promise(resolve => ipcMain.once('result', (e, arg) => {
        expect(arg).to.deep.equal({result: 3})
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
        await new Promise(setImmediate)
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
    let w = (null as unknown as BrowserWindow);

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
      try {
        function rendererStressTest() {
          const {ipcRenderer} = require('electron')
          for (let i = 0; i < 1000; i++) {
            switch ((Math.random() * 2) | 0) {
              case 0:
                ipcRenderer.send('test-async', i)
                break;
              case 1:
                ipcRenderer.sendSync('test-sync', i)
                break;
            }
          }
          ipcRenderer.send('done')
        }
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
      try {
        function rendererStressTest() {
          const {ipcRenderer} = require('electron')
          for (let i = 0; i < 1000; i++) {
            switch ((Math.random() * 3) | 0) {
              case 0:
                ipcRenderer.send('test-async', i)
                break;
              case 1:
                ipcRenderer.sendSync('test-sync', i)
                break;
              case 2:
                ipcRenderer.invoke('test-invoke', i)
                break;
            }
          }
          ipcRenderer.send('done')
        }
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
})

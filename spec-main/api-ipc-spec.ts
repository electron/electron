import * as chai from 'chai'
import * as chaiAsPromised from 'chai-as-promised'
import { BrowserWindow, ipcMain, IpcMainEvent } from 'electron'

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
      ipcMain.handleOnce('test', (e: IpcMainEvent, arg: number) => {
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
      ipcMain.handleOnce('test', async (e: IpcMainEvent, arg: number) => {
        expect(arg).to.equal(123)
        await new Promise(resolve => setImmediate(resolve))
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
      expect(() => { ipcMain.handle('test', () => {}) }).to.throw(/second handler/)
      ipcMain.removeHandler('test')
    })
  })
})

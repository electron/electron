import * as chai from 'chai'
import * as chaiAsPromised from 'chai-as-promised'
import { BrowserWindow, ipcMain } from 'electron'

const { expect } = chai

chai.use(chaiAsPromised)

describe('ipc module', () => {
  describe('invoke', () => {
    async function rendererInvoke(...args: any[]) {
      const {ipcRenderer} = require('electron')
      try {
        const result = await ipcRenderer.invoke('test', ...args)
        ipcRenderer.send('result', {result})
      } catch (e) {
        ipcRenderer.send('result', {error: e.message})
      }
    }

    it('receives responses', async () => {
      ipcMain.once('test', (e, arg) => {
        expect(arg).to.equal(123)
        e.reply('456')
      })
      const done = new Promise(resolve => {
        ipcMain.once('result', (e, arg) => {
          expect(arg).to.deep.equal({result: '456'})
          resolve()
        })
      })
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      try {
        await w.loadURL(`data:text/html,<script>(${rendererInvoke})(123)</script>`)
        await done
      } finally {
        w.destroy()
      }
    })

    it('receives errors', async () => {
      ipcMain.once('test', (e) => {
        e.throw('some error')
      })
      const done = new Promise(resolve => {
        ipcMain.once('result', (e, arg) => {
          expect(arg.error).to.match(/some error/)
          resolve()
        })
      })
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      try {
        await w.loadURL(`data:text/html,<script>(${rendererInvoke})()</script>`)
        await done
      } finally {
        w.destroy()
      }
    })

    it('registers a synchronous handler', async () => {
      (ipcMain as any).handle('test', (arg: number) => {
        ipcMain.removeAllListeners('test')
        expect(arg).to.equal(123)
        return 3
      })
      const done = new Promise(resolve => ipcMain.once('result', (e, arg) => {
        expect(arg).to.deep.equal({result: 3})
        resolve()
      }))
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      try {
        await w.loadURL(`data:text/html,<script>(${rendererInvoke})(123)</script>`)
        await done
      } finally {
        w.destroy()
      }
    })

    it('registers an asynchronous handler', async () => {
      (ipcMain as any).handle('test', async (arg: number) => {
        ipcMain.removeAllListeners('test')
        expect(arg).to.equal(123)
        await new Promise(resolve => setImmediate(resolve))
        return 3
      })
      const done = new Promise(resolve => ipcMain.once('result', (e, arg) => {
        expect(arg).to.deep.equal({result: 3})
        resolve()
      }))
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      try {
        await w.loadURL(`data:text/html,<script>(${rendererInvoke})(123)</script>`)
        await done
      } finally {
        w.destroy()
      }
    })

    it('receives an error from a synchronous handler', async () => {
      (ipcMain as any).handle('test', () => {
        ipcMain.removeAllListeners('test')
        throw new Error('some error')
      })
      const done = new Promise(resolve => ipcMain.once('result', (e, arg) => {
        expect(arg.error).to.match(/some error/)
        resolve()
      }))
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      try {
        await w.loadURL(`data:text/html,<script>(${rendererInvoke})()</script>`)
        await done
      } finally {
        w.destroy()
      }
    })

    it('receives an error from an asynchronous handler', async () => {
      (ipcMain as any).handle('test', async () => {
        ipcMain.removeAllListeners('test')
        await new Promise(resolve => setImmediate(resolve))
        throw new Error('some error')
      })
      const done = new Promise(resolve => ipcMain.once('result', (e, arg) => {
        expect(arg.error).to.match(/some error/)
        resolve()
      }))
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      try {
        await w.loadURL(`data:text/html,<script>(${rendererInvoke})()</script>`)
        await done
      } finally {
        w.destroy()
      }
    })
  })
})

import * as path from 'path'
import { expect } from 'chai'
import { closeWindow, closeAllWindows } from './window-helpers'
import { ifdescribe } from './spec-helpers'

import { ipcMain, BrowserWindow } from 'electron'
import { emittedOnce } from './events-helpers'

const features = process.electronBinding('features')

ifdescribe(features.isRemoteModuleEnabled())('remote module', () => {
  const fixtures = path.join(__dirname, 'fixtures')

  describe('', () => {
    let w = null as unknown as BrowserWindow
    beforeEach(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      w.loadURL('about:blank')
    })
    afterEach(async () => {
      await closeWindow(w)
    })

    async function remotely (script: string) {
      // executeJavaScript never returns if the script throws an error, so catch
      // any errors manually.
      const assembledScript = `(function() {
      try {
        return { result: ${script} }
      } catch (e) {
        return { error: e.message }
      }
    })()`
      const { result, error } = await w.webContents.executeJavaScript(assembledScript)
      if (error) {
        throw new Error(error)
      }
      return result
    }

    describe('remote.getGlobal filtering', () => {
      it('can return custom values', async () => {
        w.webContents.once('remote-get-global', (event, name) => {
          event.returnValue = name
        })
        expect(await remotely(`require('electron').remote.getGlobal('test')`)).to.equal('test')
      })

      it('throws when no returnValue set', async () => {
        w.webContents.once('remote-get-global', (event) => {
          event.preventDefault()
        })
        await expect(remotely(`require('electron').remote.getGlobal('test')`)).to.eventually.be.rejected(`Blocked remote.getGlobal('test')`)
      })
    })

    describe('remote.getBuiltin filtering', () => {
      it('can return custom values', async () => {
        w.webContents.once('remote-get-builtin', (event, name) => {
          event.returnValue = name
        })
        expect(await remotely(`require('electron').remote.getBuiltin('test')`)).to.equal('test')
      })

      it('throws when no returnValue set', async () => {
        w.webContents.once('remote-get-builtin', (event) => {
          event.preventDefault()
        })
        await expect(remotely(`require('electron').remote.getBuiltin('test')`)).to.eventually.be.rejected(`Blocked remote.getGlobal('test')`)
      })
    })

    describe('remote.require filtering', () => {
      it('can return custom values', async () => {
        w.webContents.once('remote-require', (event, name) => {
          event.returnValue = name
        })
        expect(await remotely(`require('electron').remote.require('test')`)).to.equal('test')
      })

      it('throws when no returnValue set', async () => {
        w.webContents.once('remote-require', (event) => {
          event.preventDefault()
        })
        await expect(remotely(`require('electron').remote.require('test')`)).to.eventually.be.rejected(`Blocked remote.require('test')`)
      })
    })

    describe('remote.getCurrentWindow filtering', () => {
      it('can return custom value', async () => {
        w.webContents.once('remote-get-current-window', (e) => {
          e.returnValue = 'some window'
        })
        expect(await remotely(`require('electron').remote.getCurrentWindow()`)).to.equal('some window')
      })

      it('throws when no returnValue set', async () => {
        w.webContents.once('remote-get-current-window', (event) => {
          event.preventDefault()
        })
        await expect(remotely(`require('electron').remote.getCurrentWindow()`)).to.eventually.be.rejected(`Blocked remote.getCurrentWindow()`)
      })
    })

    describe('remote.getCurrentWebContents filtering', () => {
      it('can return custom value', async () => {
        w.webContents.once('remote-get-current-web-contents', (event) => {
          event.returnValue = 'some web contents'
        })
        expect(await remotely(`require('electron').remote.getCurrentWebContents()`)).to.equal('some web contents')
      })

      it('throws when no returnValue set', async () => {
        w.webContents.once('remote-get-current-web-contents', (event) => {
          event.preventDefault()
        })
        await expect(remotely(`require('electron').remote.getCurrentWebContents()`)).to.eventually.be.rejected(`Blocked remote.getCurrentWebContents()`)
      })
    })

    describe('remote references', () => {
      it('render-view-deleted is sent when page is destroyed', (done) => {
        w.webContents.once('render-view-deleted' as any, () => {
          done()
        })
        w.destroy()
      })

      // The ELECTRON_BROWSER_CONTEXT_RELEASE message relies on this to work.
      it('message can be sent on exit when page is being navigated', async () => {
        after(() => { ipcMain.removeAllListeners('SENT_ON_EXIT') })
        await emittedOnce(w.webContents, 'did-finish-load')
        w.webContents.once('did-finish-load', () => {
          w.webContents.loadURL('about:blank')
        })
        w.loadFile(path.join(fixtures, 'api', 'send-on-exit.html'))
        await emittedOnce(ipcMain, 'SENT_ON_EXIT')
      })
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
          preload
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
          nodeIntegration: true
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
          nodeIntegration: true
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
      try {
        console.warn = (message: string) => { warnMessage = message }
        w.webContents.emit('remote-handler', { sender: w.webContents })
      } finally {
        console.warn = originalWarn
      }
      expect(w.webContents.listenerCount('remote-handler')).to.equal(1)
      expect(warnMessage).to.equal(expectedMessage)
    })
  })
})

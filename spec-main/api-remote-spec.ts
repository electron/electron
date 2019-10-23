import * as path from 'path'
import { expect } from 'chai'
import { closeWindow } from './window-helpers'

import { ipcMain, BrowserWindow } from 'electron'

describe('remote module', () => {
  const fixtures = path.join(__dirname, 'fixtures')

  let w = null as unknown as BrowserWindow
  beforeEach(async () => {
    w = new BrowserWindow({show: false, webPreferences: {nodeIntegration: true}})
    await w.loadURL('about:blank')
  })
  afterEach(async () => {
    await closeWindow(w)
  })

  async function remotely(script: string) {
    // executeJavaScript never returns if the script throws an error, so catch
    // any errors manually.
    const assembledScript = `(function() {
      try {
        return { result: ${script} }
      } catch (e) {
        return { error: e.message }
      }
    })()`
    const {result, error} = await w.webContents.executeJavaScript(assembledScript)
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
      w.webContents.once('remote-get-global', (event, name) => {
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
      w.webContents.once('remote-get-builtin', (event, name) => {
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
      w.webContents.once('remote-require', (event, name) => {
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
    it('message can be sent on exit when page is being navigated', (done) => {
      after(() => { ipcMain.removeAllListeners('SENT_ON_EXIT') })
      ipcMain.once('SENT_ON_EXIT', () => {
        done()
      })
      w.webContents.once('did-finish-load', () => {
        w.webContents.loadURL('about:blank')
      })
      w.loadFile(path.join(fixtures, 'api', 'send-on-exit.html'))
    })
  })
})

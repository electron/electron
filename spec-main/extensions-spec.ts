import { expect } from 'chai'
import { session, BrowserWindow, ipcMain, WebContents } from 'electron'
import { closeAllWindows, closeWindow } from './window-helpers'
import * as http from 'http'
import { AddressInfo } from 'net'
import * as path from 'path'
import * as fs from 'fs'
import { ifdescribe } from './spec-helpers'
import { emittedOnce } from './events-helpers'

const fixtures = path.join(__dirname, 'fixtures')

ifdescribe(process.electronBinding('features').isExtensionsEnabled())('chrome extensions', () => {
  // NB. extensions are only allowed on http://, https:// and ftp:// (!) urls by default.
  let server: http.Server
  let url: string
  before(async () => {
    server = http.createServer((req, res) => res.end())
    await new Promise(resolve => server.listen(0, '127.0.0.1', () => {
      url = `http://127.0.0.1:${(server.address() as AddressInfo).port}`
      resolve()
    }))
  })
  after(() => {
    server.close()
  })

  afterEach(closeAllWindows)
  it('loads an extension', async () => {
    // NB. we have to use a persist: session (i.e. non-OTR) because the
    // extension registry is redirected to the main session. so installing an
    // extension in an in-memory session results in it being installed in the
    // default session.
    const customSession = session.fromPartition(`persist:${require('uuid').v4()}`);
    (customSession as any).loadExtension(path.join(fixtures, 'extensions', 'red-bg'))
    const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } })
    await w.loadURL(url)
    const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor')
    expect(bg).to.equal('red')
  })

  it('removes an extension', async () => {
    const customSession = session.fromPartition(`persist:${require('uuid').v4()}`)
    const { id } = await (customSession as any).loadExtension(path.join(fixtures, 'extensions', 'red-bg'))
    {
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } })
      await w.loadURL(url)
      const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor')
      expect(bg).to.equal('red')
    }
    (customSession as any).removeExtension(id)
    {
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } })
      await w.loadURL(url)
      const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor')
      expect(bg).to.equal('')
    }
  })

  it('lists loaded extensions in getAllExtensions', async () => {
    const customSession = session.fromPartition(`persist:${require('uuid').v4()}`)
    const e = await (customSession as any).loadExtension(path.join(fixtures, 'extensions', 'red-bg'))
    expect((customSession as any).getAllExtensions()).to.deep.equal([e]);
    (customSession as any).removeExtension(e.id)
    expect((customSession as any).getAllExtensions()).to.deep.equal([])
  })

  it('gets an extension by id', async () => {
    const customSession = session.fromPartition(`persist:${require('uuid').v4()}`)
    const e = await (customSession as any).loadExtension(path.join(fixtures, 'extensions', 'red-bg'))
    expect((customSession as any).getExtension(e.id)).to.deep.equal(e)
  })

  it('confines an extension to the session it was loaded in', async () => {
    const customSession = session.fromPartition(`persist:${require('uuid').v4()}`);
    (customSession as any).loadExtension(path.join(fixtures, 'extensions', 'red-bg'))
    const w = new BrowserWindow({ show: false }) // not in the session
    await w.loadURL(url)
    const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor')
    expect(bg).to.equal('')
  })

  describe('chrome.runtime', () => {
    let content: any
    before(async () => {
      const customSession = session.fromPartition(`persist:${require('uuid').v4()}`);
      (customSession as any).loadExtension(path.join(fixtures, 'extensions', 'chrome-runtime'))
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } })
      try {
        await w.loadURL(url)
        content = JSON.parse(await w.webContents.executeJavaScript('document.documentElement.textContent'))
        expect(content).to.be.an('object')
      } finally {
        w.destroy()
      }
    })
    it('getManifest()', () => {
      expect(content.manifest).to.be.an('object').with.property('name', 'chrome-runtime')
    })
    it('id', () => {
      expect(content.id).to.be.a('string').with.lengthOf(32)
    })
    it('getURL()', () => {
      expect(content.url).to.be.a('string').and.match(/^chrome-extension:\/\/.*main.js$/)
    })
  })

  describe('chrome.storage', () => {
    it('stores and retrieves a key', async () => {
      const customSession = session.fromPartition(`persist:${require('uuid').v4()}`);
      (customSession as any).loadExtension(path.join(fixtures, 'extensions', 'chrome-storage'))
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession, nodeIntegration: true } })
      try {
        const p = emittedOnce(ipcMain, 'storage-success')
        await w.loadURL(url)
        const [, v] = await p
        expect(v).to.equal('value')
      } finally {
        w.destroy()
      }
    })
  })

  describe('chrome.tabs', () => {
    it('executeScript', async () => {
      const customSession = session.fromPartition(`persist:${require('uuid').v4()}`)
      ;(customSession as any).loadExtension(path.join(fixtures, 'extensions', 'chrome-api'))
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession, nodeIntegration: true } })
      await w.loadURL(url)

      const message = { method: 'executeScript', args: ['1 + 2'] }
      w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`)

      const [,, responseString] = await emittedOnce(w.webContents, 'console-message')
      const response = JSON.parse(responseString)

      expect(response).to.equal(3)
    })

    it('sendMessage receives the response', async function () {
      const customSession = session.fromPartition(`persist:${require('uuid').v4()}`)
      ;(customSession as any).loadExtension(path.join(fixtures, 'extensions', 'chrome-api'))
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession, nodeIntegration: true } })
      await w.loadURL(url)

      const message = { method: 'sendMessage', args: ['Hello World!'] }
      w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`)

      const [,, responseString] = await emittedOnce(w.webContents, 'console-message')
      const response = JSON.parse(responseString)

      expect(response.message).to.equal('Hello World!')
      expect(response.tabId).to.equal(w.webContents.id)
    })
  })

  describe('background pages', () => {
    it('loads a lazy background page when sending a message', async () => {
      const customSession = session.fromPartition(`persist:${require('uuid').v4()}`)
      ;(customSession as any).loadExtension(path.join(fixtures, 'extensions', 'lazy-background-page'))
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession, nodeIntegration: true } })
      try {
        w.loadURL(url)
        const [, resp] = await emittedOnce(ipcMain, 'bg-page-message-response')
        expect(resp.message).to.deep.equal({ some: 'message' })
        expect(resp.sender.id).to.be.a('string')
        expect(resp.sender.origin).to.equal(url)
        expect(resp.sender.url).to.equal(url + '/')
      } finally {
        w.destroy()
      }
    })
  })

  describe('devtools extensions', () => {
    let showPanelTimeoutId: any = null
    afterEach(() => {
      if (showPanelTimeoutId) clearTimeout(showPanelTimeoutId)
    })
    const showLastDevToolsPanel = (w: BrowserWindow) => {
      w.webContents.once('devtools-opened', () => {
        const show = () => {
          if (w == null || w.isDestroyed()) return
          const { devToolsWebContents } = w as unknown as { devToolsWebContents: WebContents | undefined }
          if (devToolsWebContents == null || devToolsWebContents.isDestroyed()) {
            return
          }

          const showLastPanel = () => {
            // this is executed in the devtools context, where UI is a global
            const { UI } = (window as any)
            const lastPanelId = UI.inspectorView._tabbedPane._tabs.peekLast().id
            UI.inspectorView.showPanel(lastPanelId)
          }
          devToolsWebContents.executeJavaScript(`(${showLastPanel})()`, false).then(() => {
            showPanelTimeoutId = setTimeout(show, 100)
          })
        }
        showPanelTimeoutId = setTimeout(show, 100)
      })
    }

    it('loads a devtools extension', async () => {
      const customSession = session.fromPartition(`persist:${require('uuid').v4()}`);
      (customSession as any).loadExtension(path.join(fixtures, 'extensions', 'devtools-extension'))
      const w = new BrowserWindow({ show: true, webPreferences: { session: customSession, nodeIntegration: true } })
      await w.loadURL('data:text/html,hello')
      w.webContents.openDevTools()
      showLastDevToolsPanel(w)
      await emittedOnce(ipcMain, 'winning')
    })
  })

  describe('deprecation shims', () => {
    afterEach(() => {
      (session.defaultSession as any).getAllExtensions().forEach((e: any) => {
        (session.defaultSession as any).removeExtension(e.id)
      })
    })

    it('loads an extension through BrowserWindow.addExtension', async () => {
      BrowserWindow.addExtension(path.join(fixtures, 'extensions', 'red-bg'))
      const w = new BrowserWindow({ show: false })
      await w.loadURL(url)
      const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor')
      expect(bg).to.equal('red')
    })

    it('loads an extension through BrowserWindow.addDevToolsExtension', async () => {
      BrowserWindow.addDevToolsExtension(path.join(fixtures, 'extensions', 'red-bg'))
      const w = new BrowserWindow({ show: false })
      await w.loadURL(url)
      const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor')
      expect(bg).to.equal('red')
    })

    it('removes an extension through BrowserWindow.removeExtension', async () => {
      await (BrowserWindow.addExtension(path.join(fixtures, 'extensions', 'red-bg')) as any)
      BrowserWindow.removeExtension('red-bg')
      const w = new BrowserWindow({ show: false })
      await w.loadURL(url)
      const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor')
      expect(bg).to.equal('')
    })
  })
})

ifdescribe(!process.electronBinding('features').isExtensionsEnabled())('chrome extensions', () => {
  const fixtures = path.resolve(__dirname, 'fixtures')
  let w: BrowserWindow

  before(() => {
    BrowserWindow.addExtension(path.join(fixtures, 'extensions/chrome-api'))
  })

  after(() => {
    BrowserWindow.removeExtension('chrome-api')
  })

  beforeEach(() => {
    w = new BrowserWindow({ show: false })
  })

  afterEach(() => closeWindow(w).then(() => { w = null as unknown as BrowserWindow }))

  it('chrome.runtime.connect parses arguments properly', async function () {
    await w.loadURL('about:blank')

    const promise = emittedOnce(w.webContents, 'console-message')

    const message = { method: 'connect' }
    w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`)

    const [,, responseString] = await promise
    const response = JSON.parse(responseString)

    expect(response).to.be.true()
  })

  it('runtime.getManifest returns extension manifest', async () => {
    const actualManifest = (() => {
      const data = fs.readFileSync(path.join(fixtures, 'extensions/chrome-api/manifest.json'), 'utf-8')
      return JSON.parse(data)
    })()

    await w.loadURL('about:blank')

    const promise = emittedOnce(w.webContents, 'console-message')

    const message = { method: 'getManifest' }
    w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`)

    const [,, manifestString] = await promise
    const manifest = JSON.parse(manifestString)

    expect(manifest.name).to.equal(actualManifest.name)
    expect(manifest.content_scripts).to.have.lengthOf(actualManifest.content_scripts.length)
  })

  it('chrome.tabs.sendMessage receives the response', async function () {
    await w.loadURL('about:blank')

    const promise = emittedOnce(w.webContents, 'console-message')

    const message = { method: 'sendMessage', args: ['Hello World!'] }
    w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`)

    const [,, responseString] = await promise
    const response = JSON.parse(responseString)

    expect(response.message).to.equal('Hello World!')
    expect(response.tabId).to.equal(w.webContents.id)
  })

  it('chrome.tabs.executeScript receives the response', async function () {
    await w.loadURL('about:blank')

    const promise = emittedOnce(w.webContents, 'console-message')

    const message = { method: 'executeScript', args: ['1 + 2'] }
    w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`)

    const [,, responseString] = await promise
    const response = JSON.parse(responseString)

    expect(response).to.equal(3)
  })
})

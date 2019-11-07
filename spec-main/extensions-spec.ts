import { expect } from 'chai'
import { session, BrowserWindow, ipcMain } from 'electron'
import { closeAllWindows } from './window-helpers'
import * as http from 'http'
import { AddressInfo } from 'net'
import * as path from 'path'
import * as fs from 'fs'
import { ifdescribe } from './spec-helpers'
import { emittedOnce } from './events-helpers'
import { closeWindow } from './window-helpers';

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
    (customSession as any).loadChromeExtension(path.join(fixtures, 'extensions', 'red-bg'))
    const w = new BrowserWindow({show: false, webPreferences: {session: customSession}})
    await w.loadURL(url)
    const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor')
    expect(bg).to.equal('red')
  })

  it('confines an extension to the session it was loaded in', async () => {
    const customSession = session.fromPartition(`persist:${require('uuid').v4()}`);
    (customSession as any).loadChromeExtension(path.join(fixtures, 'extensions', 'red-bg'))
    const w = new BrowserWindow({show: false}) // not in the session
    await w.loadURL(url)
    const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor')
    expect(bg).to.equal('')
  })

  describe('chrome.runtime', () => {
    let content: any
    before(async () => {
      const customSession = session.fromPartition(`persist:${require('uuid').v4()}`);
      (customSession as any).loadChromeExtension(path.join(fixtures, 'extensions', 'chrome-runtime'))
      const w = new BrowserWindow({show: false, webPreferences: { session: customSession }})
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
      (customSession as any).loadChromeExtension(path.join(fixtures, 'extensions', 'chrome-storage'))
      const w = new BrowserWindow({show: false, webPreferences: { session: customSession, nodeIntegration: true }})
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
})

ifdescribe(!process.electronBinding('features').isExtensionsEnabled())('chrome extensions', () => {
  const fixtures = path.resolve(__dirname, '..', 'spec', 'fixtures')
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

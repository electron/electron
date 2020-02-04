import { expect } from 'chai'
import { app, session, BrowserWindow, ipcMain, WebContents } from 'electron'
import { closeAllWindows, closeWindow } from './window-helpers'
import * as http from 'http'
import { AddressInfo } from 'net'
import * as path from 'path'
import * as fs from 'fs'
import { ifdescribe } from './spec-helpers'
import { emittedOnce, emittedNTimes } from './events-helpers'

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
    const customSession = session.fromPartition(`persist:${require('uuid').v4()}`)
    await customSession.loadExtension(path.join(fixtures, 'extensions', 'red-bg'))
    const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } })
    await w.loadURL(url)
    const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor')
    expect(bg).to.equal('red')
  })

  it('removes an extension', async () => {
    const customSession = session.fromPartition(`persist:${require('uuid').v4()}`)
    const { id } = await customSession.loadExtension(path.join(fixtures, 'extensions', 'red-bg'))
    {
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } })
      await w.loadURL(url)
      const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor')
      expect(bg).to.equal('red')
    }
    customSession.removeExtension(id)
    {
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } })
      await w.loadURL(url)
      const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor')
      expect(bg).to.equal('')
    }
  })

  it('lists loaded extensions in getAllExtensions', async () => {
    const customSession = session.fromPartition(`persist:${require('uuid').v4()}`)
    const e = await customSession.loadExtension(path.join(fixtures, 'extensions', 'red-bg'))
    expect(customSession.getAllExtensions()).to.deep.equal([e])
    customSession.removeExtension(e.id)
    expect(customSession.getAllExtensions()).to.deep.equal([])
  })

  it('gets an extension by id', async () => {
    const customSession = session.fromPartition(`persist:${require('uuid').v4()}`)
    const e = await customSession.loadExtension(path.join(fixtures, 'extensions', 'red-bg'))
    expect(customSession.getExtension(e.id)).to.deep.equal(e)
  })

  it('confines an extension to the session it was loaded in', async () => {
    const customSession = session.fromPartition(`persist:${require('uuid').v4()}`)
    customSession.loadExtension(path.join(fixtures, 'extensions', 'red-bg'))
    const w = new BrowserWindow({ show: false }) // not in the session
    await w.loadURL(url)
    const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor')
    expect(bg).to.equal('')
  })

  describe('chrome.runtime', () => {
    let content: any
    before(async () => {
      const customSession = session.fromPartition(`persist:${require('uuid').v4()}`)
      customSession.loadExtension(path.join(fixtures, 'extensions', 'chrome-runtime'))
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
      const customSession = session.fromPartition(`persist:${require('uuid').v4()}`)
      await customSession.loadExtension(path.join(fixtures, 'extensions', 'chrome-storage'))
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
      await customSession.loadExtension(path.join(fixtures, 'extensions', 'chrome-api'))
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
      await customSession.loadExtension(path.join(fixtures, 'extensions', 'chrome-api'))
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
      await customSession.loadExtension(path.join(fixtures, 'extensions', 'lazy-background-page'))
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

    it('can use extension.getBackgroundPage from a ui page', async () => {
      const customSession = session.fromPartition(`persist:${require('uuid').v4()}`)
      const { id } = await customSession.loadExtension(path.join(fixtures, 'extensions', 'lazy-background-page'))
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } })
      await w.loadURL(`chrome-extension://${id}/page-get-background.html`)
      const receivedMessage = await w.webContents.executeJavaScript(`window.completionPromise`)
      expect(receivedMessage).to.deep.equal({ some: 'message' })
    })

    it('can use extension.getBackgroundPage from a ui page', async () => {
      const customSession = session.fromPartition(`persist:${require('uuid').v4()}`)
      const { id } = await customSession.loadExtension(path.join(fixtures, 'extensions', 'lazy-background-page'))
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } })
      await w.loadURL(`chrome-extension://${id}/page-get-background.html`)
      const receivedMessage = await w.webContents.executeJavaScript(`window.completionPromise`)
      expect(receivedMessage).to.deep.equal({ some: 'message' })
    })

    it('can use runtime.getBackgroundPage from a ui page', async () => {
      const customSession = session.fromPartition(`persist:${require('uuid').v4()}`)
      const { id } = await customSession.loadExtension(path.join(fixtures, 'extensions', 'lazy-background-page'))
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } })
      await w.loadURL(`chrome-extension://${id}/page-runtime-get-background.html`)
      const receivedMessage = await w.webContents.executeJavaScript(`window.completionPromise`)
      expect(receivedMessage).to.deep.equal({ some: 'message' })
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
      const customSession = session.fromPartition(`persist:${require('uuid').v4()}`)
      customSession.loadExtension(path.join(fixtures, 'extensions', 'devtools-extension'))
      const w = new BrowserWindow({ show: true, webPreferences: { session: customSession, nodeIntegration: true } })
      await w.loadURL('data:text/html,hello')
      w.webContents.openDevTools()
      showLastDevToolsPanel(w)
      await emittedOnce(ipcMain, 'winning')
    })
  })

  describe('deprecation shims', () => {
    afterEach(() => {
      session.defaultSession.getAllExtensions().forEach((e: any) => {
        session.defaultSession.removeExtension(e.id)
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

  describe('chrome extension content scripts', () => {
    const fixtures = path.resolve(__dirname, 'fixtures')
    const extensionPath = path.resolve(fixtures, 'extensions')

    const addExtension = (name: string) => session.defaultSession.loadExtension(path.resolve(extensionPath, name))
    const removeAllExtensions = () => {
      Object.keys(session.defaultSession.getAllExtensions()).map(extName => {
        session.defaultSession.removeExtension(extName)
      })
    }

    let responseIdCounter = 0
    const executeJavaScriptInFrame = (webContents: WebContents, frameRoutingId: number, code: string) => {
      return new Promise(resolve => {
        const responseId = responseIdCounter++
        ipcMain.once(`executeJavaScriptInFrame_${responseId}`, (event, result) => {
          resolve(result)
        })
        webContents.send('executeJavaScriptInFrame', frameRoutingId, code, responseId)
      })
    }

    const generateTests = (sandboxEnabled: boolean, contextIsolationEnabled: boolean) => {
      describe(`with sandbox ${sandboxEnabled ? 'enabled' : 'disabled'} and context isolation ${contextIsolationEnabled ? 'enabled' : 'disabled'}`, () => {
        let w: BrowserWindow

        describe('supports "run_at" option', () => {
          beforeEach(async () => {
            await closeWindow(w)
            w = new BrowserWindow({
              show: false,
              width: 400,
              height: 400,
              webPreferences: {
                contextIsolation: contextIsolationEnabled,
                sandbox: sandboxEnabled
              }
            })
          })

          afterEach(() => {
            removeAllExtensions()
            return closeWindow(w).then(() => { w = null as unknown as BrowserWindow })
          })

          it('should run content script at document_start', async () => {
            await addExtension('content-script-document-start')
            w.webContents.once('dom-ready', async () => {
              const result = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor')
              expect(result).to.equal('red')
            })
            w.loadURL(url)
          })

          it('should run content script at document_idle', async () => {
            await addExtension('content-script-document-idle')
            w.loadURL(url)
            const result = await w.webContents.executeJavaScript('document.body.style.backgroundColor')
            expect(result).to.equal('red')
          })

          it('should run content script at document_end', async () => {
            await addExtension('content-script-document-end')
            w.webContents.once('did-finish-load', async () => {
              const result = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor')
              expect(result).to.equal('red')
            })
            w.loadURL(url)
          })
        })

        // TODO(nornagon): real extensions don't load on file: urls, so this
        // test needs to be updated to serve its content over http.
        describe.skip('supports "all_frames" option', () => {
          const contentScript = path.resolve(fixtures, 'extensions/content-script')

          // Computed style values
          const COLOR_RED = `rgb(255, 0, 0)`
          const COLOR_BLUE = `rgb(0, 0, 255)`
          const COLOR_TRANSPARENT = `rgba(0, 0, 0, 0)`

          before(() => {
            BrowserWindow.addExtension(contentScript)
          })

          after(() => {
            BrowserWindow.removeExtension('content-script-test')
          })

          beforeEach(() => {
            w = new BrowserWindow({
              show: false,
              webPreferences: {
                // enable content script injection in subframes
                nodeIntegrationInSubFrames: true,
                preload: path.join(contentScript, 'all_frames-preload.js')
              }
            })
          })

          afterEach(() =>
            closeWindow(w).then(() => {
              w = null as unknown as BrowserWindow
            })
          )

          it('applies matching rules in subframes', async () => {
            const detailsPromise = emittedNTimes(w.webContents, 'did-frame-finish-load', 2)
            w.loadFile(path.join(contentScript, 'frame-with-frame.html'))
            const frameEvents = await detailsPromise
            await Promise.all(
              frameEvents.map(async frameEvent => {
                const [, isMainFrame, , frameRoutingId] = frameEvent
                const result: any = await executeJavaScriptInFrame(
                  w.webContents,
                  frameRoutingId,
                  `(() => {
                    const a = document.getElementById('all_frames_enabled')
                    const b = document.getElementById('all_frames_disabled')
                    return {
                      enabledColor: getComputedStyle(a).backgroundColor,
                      disabledColor: getComputedStyle(b).backgroundColor
                    }
                  })()`
                )
                expect(result.enabledColor).to.equal(COLOR_RED)
                if (isMainFrame) {
                  expect(result.disabledColor).to.equal(COLOR_BLUE)
                } else {
                  expect(result.disabledColor).to.equal(COLOR_TRANSPARENT) // null color
                }
              })
            )
          })
        })
      })
    }

    generateTests(false, false)
    generateTests(false, true)
    generateTests(true, false)
    generateTests(true, true)
  })

  describe('extension ui pages', () => {
    afterEach(() => {
      session.defaultSession.getAllExtensions().forEach(e => {
        session.defaultSession.removeExtension(e.id)
      })
    })

    it('loads a ui page of an extension', async () => {
      const { id } = await session.defaultSession.loadExtension(path.join(fixtures, 'extensions', 'ui-page'))
      const w = new BrowserWindow({ show: false })
      await w.loadURL(`chrome-extension://${id}/bare-page.html`)
      const textContent = await w.webContents.executeJavaScript(`document.body.textContent`)
      expect(textContent).to.equal('ui page loaded ok\n')
    })

    it('can load resources', async () => {
      const { id } = await session.defaultSession.loadExtension(path.join(fixtures, 'extensions', 'ui-page'))
      const w = new BrowserWindow({ show: false })
      await w.loadURL(`chrome-extension://${id}/page-script-load.html`)
      const textContent = await w.webContents.executeJavaScript(`document.body.textContent`)
      expect(textContent).to.equal('script loaded ok\n')
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

  describe('extensions and dev tools extensions', () => {
    let showPanelTimeoutId: NodeJS.Timeout | null = null

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

    afterEach(() => {
      if (showPanelTimeoutId != null) {
        clearTimeout(showPanelTimeoutId)
        showPanelTimeoutId = null
      }
    })

    describe('BrowserWindow.addDevToolsExtension', () => {
      describe('for invalid extensions', () => {
        it('throws errors for missing manifest.json files', () => {
          const nonexistentExtensionPath = path.join(__dirname, 'does-not-exist')
          expect(() => {
            BrowserWindow.addDevToolsExtension(nonexistentExtensionPath)
          }).to.throw(/ENOENT: no such file or directory/)
        })

        it('throws errors for invalid manifest.json files', () => {
          const badManifestExtensionPath = path.join(__dirname, 'fixtures', 'devtools-extensions', 'bad-manifest')
          expect(() => {
            BrowserWindow.addDevToolsExtension(badManifestExtensionPath)
          }).to.throw(/Unexpected token }/)
        })
      })

      describe('for a valid extension', () => {
        const extensionName = 'foo'

        before(() => {
          const extensionPath = path.join(__dirname, 'fixtures', 'devtools-extensions', 'foo')
          BrowserWindow.addDevToolsExtension(extensionPath)
          expect(BrowserWindow.getDevToolsExtensions()).to.have.property(extensionName)
        })

        after(() => {
          BrowserWindow.removeDevToolsExtension('foo')
          expect(BrowserWindow.getDevToolsExtensions()).to.not.have.property(extensionName)
        })

        describe('when the devtools is docked', () => {
          let message: any
          let w: BrowserWindow
          before(async () => {
            w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
            const p = new Promise(resolve => ipcMain.once('answer', (event, message) => {
              resolve(message)
            }))
            showLastDevToolsPanel(w)
            w.loadURL('about:blank')
            w.webContents.openDevTools({ mode: 'bottom' })
            message = await p
          })
          after(closeAllWindows)

          describe('created extension info', function () {
            it('has proper "runtimeId"', async function () {
              expect(message).to.have.ownProperty('runtimeId')
              expect(message.runtimeId).to.equal(extensionName)
            })
            it('has "tabId" matching webContents id', function () {
              expect(message).to.have.ownProperty('tabId')
              expect(message.tabId).to.equal(w.webContents.id)
            })
            it('has "i18nString" with proper contents', function () {
              expect(message).to.have.ownProperty('i18nString')
              expect(message.i18nString).to.equal('foo - bar (baz)')
            })
            it('has "storageItems" with proper contents', function () {
              expect(message).to.have.ownProperty('storageItems')
              expect(message.storageItems).to.deep.equal({
                local: {
                  set: { hello: 'world', world: 'hello' },
                  remove: { world: 'hello' },
                  clear: {}
                },
                sync: {
                  set: { foo: 'bar', bar: 'foo' },
                  remove: { foo: 'bar' },
                  clear: {}
                }
              })
            })
          })
        })

        describe('when the devtools is undocked', () => {
          let message: any
          let w: BrowserWindow
          before(async () => {
            w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
            showLastDevToolsPanel(w)
            w.loadURL('about:blank')
            w.webContents.openDevTools({ mode: 'undocked' })
            message = await new Promise(resolve => ipcMain.once('answer', (event, message) => {
              resolve(message)
            }))
          })
          after(closeAllWindows)

          describe('created extension info', function () {
            it('has proper "runtimeId"', function () {
              expect(message).to.have.ownProperty('runtimeId')
              expect(message.runtimeId).to.equal(extensionName)
            })
            it('has "tabId" matching webContents id', function () {
              expect(message).to.have.ownProperty('tabId')
              expect(message.tabId).to.equal(w.webContents.id)
            })
          })
        })
      })
    })

    it('works when used with partitions', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          partition: 'temp'
        }
      })

      const extensionPath = path.join(__dirname, 'fixtures', 'devtools-extensions', 'foo')
      BrowserWindow.addDevToolsExtension(extensionPath)
      try {
        showLastDevToolsPanel(w)

        const p: Promise<any> = new Promise(resolve => ipcMain.once('answer', function (event, message) {
          resolve(message)
        }))

        w.loadURL('about:blank')
        w.webContents.openDevTools({ mode: 'bottom' })
        const message = await p
        expect(message.runtimeId).to.equal('foo')
      } finally {
        BrowserWindow.removeDevToolsExtension('foo')
        await closeAllWindows()
      }
    })

    it('serializes the registered extensions on quit', () => {
      const extensionName = 'foo'
      const extensionPath = path.join(__dirname, 'fixtures', 'devtools-extensions', extensionName)
      const serializedPath = path.join(app.getPath('userData'), 'DevTools Extensions')

      BrowserWindow.addDevToolsExtension(extensionPath)
      app.emit('will-quit')
      expect(JSON.parse(fs.readFileSync(serializedPath, 'utf8'))).to.deep.equal([extensionPath])

      BrowserWindow.removeDevToolsExtension(extensionName)
      app.emit('will-quit')
      expect(fs.existsSync(serializedPath)).to.be.false('file exists')
    })

    describe('BrowserWindow.addExtension', () => {
      it('throws errors for missing manifest.json files', () => {
        expect(() => {
          BrowserWindow.addExtension(path.join(__dirname, 'does-not-exist'))
        }).to.throw('ENOENT: no such file or directory')
      })

      it('throws errors for invalid manifest.json files', () => {
        expect(() => {
          BrowserWindow.addExtension(path.join(__dirname, 'fixtures', 'devtools-extensions', 'bad-manifest'))
        }).to.throw('Unexpected token }')
      })
    })
  })
})

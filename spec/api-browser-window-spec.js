'use strict'

const chai = require('chai')
const dirtyChai = require('dirty-chai')
const fs = require('fs')
const path = require('path')
const os = require('os')
const qs = require('querystring')
const http = require('http')
const { closeWindow } = require('./window-helpers')
const { emittedOnce } = require('./events-helpers')
const { createNetworkSandbox } = require('./network-helper')
const { ipcRenderer, remote } = require('electron')
const { app, ipcMain, BrowserWindow, BrowserView, protocol, session, screen, webContents } = remote

const features = process.electronBinding('features')
const { expect } = chai
const isCI = remote.getGlobal('isCi')
const nativeModulesEnabled = remote.getGlobal('nativeModulesEnabled')

chai.use(dirtyChai)

describe('BrowserWindow module', () => {
  const fixtures = path.resolve(__dirname, 'fixtures')
  let w = null
  let iw = null
  let ws = null
  let server
  let postData

  const defaultOptions = {
    show: false,
    width: 400,
    height: 400,
    webPreferences: {
      backgroundThrottling: false,
      nodeIntegration: true
    }
  }

  const openTheWindow = async (options = defaultOptions) => {
    // The `afterEach` hook isn't called if a test fails,
    // we should make sure that the window is closed ourselves.
    await closeTheWindow()

    w = new BrowserWindow(options)
    return w
  }

  const closeTheWindow = function () {
    return closeWindow(w).then(() => { w = null })
  }

  before((done) => {
    const filePath = path.join(fixtures, 'pages', 'a.html')
    const fileStats = fs.statSync(filePath)
    postData = [
      {
        type: 'rawData',
        bytes: Buffer.from('username=test&file=')
      },
      {
        type: 'file',
        filePath: filePath,
        offset: 0,
        length: fileStats.size,
        modificationTime: fileStats.mtime.getTime() / 1000
      }
    ]
    server = http.createServer((req, res) => {
      function respond () {
        if (req.method === 'POST') {
          let body = ''
          req.on('data', (data) => {
            if (data) body += data
          })
          req.on('end', () => {
            const parsedData = qs.parse(body)
            fs.readFile(filePath, (err, data) => {
              if (err) return
              if (parsedData.username === 'test' &&
                  parsedData.file === data.toString()) {
                res.end()
              }
            })
          })
        } else if (req.url === '/302') {
          res.setHeader('Location', '/200')
          res.statusCode = 302
          res.end()
        } else if (req.url === '/navigate-302') {
          res.end(`<html><body><script>window.location='${server.url}/302'</script></body></html>`)
        } else if (req.url === '/cross-site') {
          res.end(`<html><body><h1>${req.url}</h1></body></html>`)
        } else {
          res.end()
        }
      }
      setTimeout(respond, req.url.includes('slow') ? 200 : 0)
    })
    server.listen(0, '127.0.0.1', () => {
      server.url = `http://127.0.0.1:${server.address().port}`
      done()
    })
  })

  after(() => {
    server.close()
    server = null
  })

  beforeEach(openTheWindow)

  afterEach(closeTheWindow)

  describe('BrowserWindow.setAutoHideCursor(autoHide)', () => {
    describe('on macOS', () => {
      before(function () {
        if (process.platform !== 'darwin') {
          this.skip()
        }
      })

      it('allows changing cursor auto-hiding', () => {
        expect(() => {
          w.setAutoHideCursor(false)
          w.setAutoHideCursor(true)
        }).to.not.throw()
      })
    })

    describe('on non-macOS platforms', () => {
      before(function () {
        if (process.platform === 'darwin') {
          this.skip()
        }
      })

      it('is not available', () => {
        expect(w.setAutoHideCursor).to.be.undefined()
      })
    })
  })

  describe('BrowserWindow.setWindowButtonVisibility()', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('does not throw', () => {
      expect(() => {
        w.setWindowButtonVisibility(true)
        w.setWindowButtonVisibility(false)
      }).to.not.throw()
    })

    it('throws with custom title bar buttons', () => {
      expect(() => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          titleBarStyle: 'customButtonsOnHover',
          frame: false
        })
        w.setWindowButtonVisibility(true)
      }).to.throw('Not supported for this window')
    })
  })

  describe('BrowserWindow.setVibrancy(type)', () => {
    it('allows setting, changing, and removing the vibrancy', () => {
      expect(() => {
        w.setVibrancy('light')
        w.setVibrancy('dark')
        w.setVibrancy(null)
        w.setVibrancy('ultra-dark')
        w.setVibrancy('')
      }).to.not.throw()
    })
  })

  describe('BrowserWindow.setAppDetails(options)', () => {
    before(function () {
      if (process.platform !== 'win32') {
        this.skip()
      }
    })

    it('supports setting the app details', () => {
      const iconPath = path.join(fixtures, 'assets', 'icon.ico')

      expect(() => {
        w.setAppDetails({ appId: 'my.app.id' })
        w.setAppDetails({ appIconPath: iconPath, appIconIndex: 0 })
        w.setAppDetails({ appIconPath: iconPath })
        w.setAppDetails({ relaunchCommand: 'my-app.exe arg1 arg2', relaunchDisplayName: 'My app name' })
        w.setAppDetails({ relaunchCommand: 'my-app.exe arg1 arg2' })
        w.setAppDetails({ relaunchDisplayName: 'My app name' })
        w.setAppDetails({
          appId: 'my.app.id',
          appIconPath: iconPath,
          appIconIndex: 0,
          relaunchCommand: 'my-app.exe arg1 arg2',
          relaunchDisplayName: 'My app name'
        })
        w.setAppDetails({})
      }).to.not.throw()

      expect(() => {
        w.setAppDetails()
      }).to.throw('Insufficient number of arguments.')
    })
  })

  describe('BrowserWindow.fromId(id)', () => {
    it('returns the window with id', () => {
      expect(BrowserWindow.fromId(w.id).id).to.equal(w.id)
    })
  })

  describe('BrowserWindow.fromWebContents(webContents)', () => {
    let contents = null

    beforeEach(() => { contents = webContents.create({}) })

    afterEach(() => { contents.destroy() })

    it('returns the window with the webContents', () => {
      expect(BrowserWindow.fromWebContents(w.webContents).id).to.equal(w.id)
      expect(BrowserWindow.fromWebContents(contents)).to.be.undefined()
    })
  })

  describe('BrowserWindow.fromDevToolsWebContents(webContents)', () => {
    let contents = null

    beforeEach(() => { contents = webContents.create({}) })

    afterEach(() => { contents.destroy() })

    it('returns the window with the webContents', (done) => {
      w.webContents.once('devtools-opened', () => {
        expect(BrowserWindow.fromDevToolsWebContents(w.devToolsWebContents).id).to.equal(w.id)
        expect(BrowserWindow.fromDevToolsWebContents(w.webContents)).to.be.undefined()
        expect(BrowserWindow.fromDevToolsWebContents(contents)).to.be.undefined()
        done()
      })
      w.webContents.openDevTools()
    })
  })

  describe('BrowserWindow.openDevTools()', () => {
    it('does not crash for frameless window', () => {
      w.destroy()
      w = new BrowserWindow({ show: false })
      w.openDevTools()
    })
  })

  describe('BrowserWindow.fromBrowserView(browserView)', () => {
    let bv = null

    beforeEach(() => {
      bv = new BrowserView()
      w.setBrowserView(bv)
    })

    afterEach(() => {
      w.setBrowserView(null)
      bv.destroy()
    })

    it('returns the window with the browserView', () => {
      expect(BrowserWindow.fromBrowserView(bv).id).to.equal(w.id)
    })

    it('returns undefined if not attached', () => {
      w.setBrowserView(null)
      expect(BrowserWindow.fromBrowserView(bv)).to.be.null()
    })
  })

  describe('BrowserWindow.setOpacity(opacity)', () => {
    it('make window with initial opacity', () => {
      w.destroy()
      w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        opacity: 0.5
      })
      expect(w.getOpacity()).to.equal(0.5)
    })
    it('allows setting the opacity', () => {
      expect(() => {
        w.setOpacity(0.0)
        expect(w.getOpacity()).to.equal(0.0)
        w.setOpacity(0.5)
        expect(w.getOpacity()).to.equal(0.5)
        w.setOpacity(1.0)
        expect(w.getOpacity()).to.equal(1.0)
      }).to.not.throw()
    })
  })

  describe('BrowserWindow.setShape(rects)', () => {
    it('allows setting shape', () => {
      expect(() => {
        w.setShape([])
        w.setShape([{ x: 0, y: 0, width: 100, height: 100 }])
        w.setShape([{ x: 0, y: 0, width: 100, height: 100 }, { x: 0, y: 200, width: 1000, height: 100 }])
        w.setShape([])
      }).to.not.throw()
    })
  })

  describe('"useContentSize" option', () => {
    it('make window created with content size when used', () => {
      w.destroy()
      w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        useContentSize: true
      })
      const contentSize = w.getContentSize()
      expect(contentSize).to.deep.equal([400, 400])
    })
    it('make window created with window size when not used', () => {
      const size = w.getSize()
      expect(size).to.deep.equal([400, 400])
    })
    it('works for a frameless window', () => {
      w.destroy()
      w = new BrowserWindow({
        show: false,
        frame: false,
        width: 400,
        height: 400,
        useContentSize: true
      })
      const contentSize = w.getContentSize()
      expect(contentSize).to.deep.equal([400, 400])
      const size = w.getSize()
      expect(size).to.deep.equal([400, 400])
    })
  })

  describe('"titleBarStyle" option', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }

      if (parseInt(os.release().split('.')[0]) < 14) {
        this.skip()
      }
    })

    it('creates browser window with hidden title bar', () => {
      w.destroy()
      w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        titleBarStyle: 'hidden'
      })
      const contentSize = w.getContentSize()
      expect(contentSize).to.deep.equal([400, 400])
    })
    it('creates browser window with hidden inset title bar', () => {
      w.destroy()
      w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        titleBarStyle: 'hiddenInset'
      })
      const contentSize = w.getContentSize()
      expect(contentSize).to.deep.equal([400, 400])
    })
  })

  describe('enableLargerThanScreen" option', () => {
    before(function () {
      if (process.platform === 'linux') {
        this.skip()
      }
    })

    beforeEach(() => {
      w.destroy()
      w = new BrowserWindow({
        show: true,
        width: 400,
        height: 400,
        enableLargerThanScreen: true
      })
    })

    it('can move the window out of screen', () => {
      w.setPosition(-10, -10)
      const after = w.getPosition()
      expect(after).to.deep.equal([-10, -10])
    })
    it('can set the window larger than screen', () => {
      const size = screen.getPrimaryDisplay().size
      size.width += 100
      size.height += 100
      w.setSize(size.width, size.height)
      expectBoundsEqual(w.getSize(), [size.width, size.height])
    })
  })

  describe('"zoomToPageWidth" option', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('sets the window width to the page width when used', () => {
      w.destroy()
      w = new BrowserWindow({
        show: false,
        width: 500,
        height: 400,
        zoomToPageWidth: true
      })
      w.maximize()
      expect(w.getSize()[0]).to.equal(500)
    })
  })

  describe('"tabbingIdentifier" option', () => {
    it('can be set on a window', () => {
      w.destroy()
      w = new BrowserWindow({
        tabbingIdentifier: 'group1'
      })
      w.destroy()
      w = new BrowserWindow({
        tabbingIdentifier: 'group2',
        frame: false
      })
    })
  })

  describe('"webPreferences" option', () => {
    afterEach(() => { ipcMain.removeAllListeners('answer') })

    describe('"preload" option', () => {
      const doesNotLeakSpec = (name, webPrefs) => {
        it(name, async function () {
          w.destroy()
          w = new BrowserWindow({
            webPreferences: {
              ...webPrefs,
              preload: path.resolve(fixtures, 'module', 'empty.js')
            },
            show: false
          })
          const leakResult = emittedOnce(ipcMain, 'leak-result')
          w.loadFile(path.join(fixtures, 'api', 'no-leak.html'))
          const [, result] = await leakResult
          expect(result).to.have.property('require', 'undefined')
          expect(result).to.have.property('exports', 'undefined')
          expect(result).to.have.property('windowExports', 'undefined')
          expect(result).to.have.property('windowPreload', 'undefined')
          expect(result).to.have.property('windowRequire', 'undefined')
        })
      }
      doesNotLeakSpec('does not leak require', {
        nodeIntegration: false,
        sandbox: false,
        contextIsolation: false
      })
      doesNotLeakSpec('does not leak require when sandbox is enabled', {
        nodeIntegration: false,
        sandbox: true,
        contextIsolation: false
      })
      doesNotLeakSpec('does not leak require when context isolation is enabled', {
        nodeIntegration: false,
        sandbox: false,
        contextIsolation: true
      })
      doesNotLeakSpec('does not leak require when context isolation and sandbox are enabled', {
        nodeIntegration: false,
        sandbox: true,
        contextIsolation: true
      })

      it('loads the script before other scripts in window', async () => {
        const preload = path.join(fixtures, 'module', 'set-global.js')
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            preload
          }
        })
        const p = emittedOnce(ipcMain, 'answer')
        w.loadFile(path.join(fixtures, 'api', 'preload.html'))
        const [, test] = await p
        expect(test).to.eql('preload')
      })
      it('can successfully delete the Buffer global', async () => {
        const preload = path.join(fixtures, 'module', 'delete-buffer.js')
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            preload
          }
        })
        const p = emittedOnce(ipcMain, 'answer')
        w.loadFile(path.join(fixtures, 'api', 'preload.html'))
        const [, test] = await p
        expect(test.toString()).to.eql('buffer')
      })
      it('has synchronous access to all eventual window APIs', async () => {
        const preload = path.join(fixtures, 'module', 'access-blink-apis.js')
        const w = await openTheWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            preload
          }
        })
        const p = emittedOnce(ipcMain, 'answer')
        w.loadFile(path.join(fixtures, 'api', 'preload.html'))
        const [, test] = await p
        expect(test).to.be.an('object')
        expect(test.atPreload).to.be.an('array')
        expect(test.atLoad).to.be.an('array')
        expect(test.atPreload).to.deep.equal(test.atLoad, 'should have access to the same window APIs')
      })
    })

    describe('session preload scripts', function () {
      const preloads = [
        path.join(fixtures, 'module', 'set-global-preload-1.js'),
        path.join(fixtures, 'module', 'set-global-preload-2.js')
      ]
      const defaultSession = session.defaultSession

      beforeEach(() => {
        expect(defaultSession.getPreloads()).to.deep.equal([])
        defaultSession.setPreloads(preloads)
      })
      afterEach(() => {
        defaultSession.setPreloads([])
      })

      it('can set multiple session preload script', function () {
        expect(defaultSession.getPreloads()).to.deep.equal(preloads)
      })

      const generateSpecs = (description, sandbox) => {
        describe(description, () => {
          it('loads the script before other scripts in window including normal preloads', function (done) {
            ipcMain.once('vars', function (event, preload1, preload2) {
              expect(preload1).to.equal('preload-1')
              expect(preload2).to.equal('preload-1-2')
              done()
            })
            w.destroy()
            w = new BrowserWindow({
              show: false,
              webPreferences: {
                sandbox,
                preload: path.join(fixtures, 'module', 'get-global-preload.js')
              }
            })
            w.loadURL('about:blank')
          })
        })
      }

      generateSpecs('without sandbox', false)
      generateSpecs('with sandbox', true)
    })

    describe('"additionalArguments" option', () => {
      it('adds extra args to process.argv in the renderer process', (done) => {
        const preload = path.join(fixtures, 'module', 'check-arguments.js')
        ipcMain.once('answer', (event, argv) => {
          expect(argv).to.include('--my-magic-arg')
          done()
        })
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            preload,
            additionalArguments: ['--my-magic-arg']
          }
        })
        w.loadFile(path.join(fixtures, 'api', 'blank.html'))
      })

      it('adds extra value args to process.argv in the renderer process', (done) => {
        const preload = path.join(fixtures, 'module', 'check-arguments.js')
        ipcMain.once('answer', (event, argv) => {
          expect(argv).to.include('--my-magic-arg=foo')
          done()
        })
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            preload,
            additionalArguments: ['--my-magic-arg=foo']
          }
        })
        w.loadFile(path.join(fixtures, 'api', 'blank.html'))
      })
    })

    describe('"node-integration" option', () => {
      it('disables node integration by default', (done) => {
        const preload = path.join(fixtures, 'module', 'send-later.js')
        ipcMain.once('answer', (event, typeofProcess, typeofBuffer) => {
          expect(typeofProcess).to.equal('undefined')
          expect(typeofBuffer).to.equal('undefined')
          done()
        })
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            preload
          }
        })
        w.loadFile(path.join(fixtures, 'api', 'blank.html'))
      })
    })

    describe('"enableRemoteModule" option', () => {
      const generateSpecs = (description, sandbox) => {
        describe(description, () => {
          const preload = path.join(fixtures, 'module', 'preload-remote.js')

          it('enables the remote module by default', async () => {
            const w = await openTheWindow({
              show: false,
              webPreferences: {
                preload,
                sandbox
              }
            })
            const p = emittedOnce(ipcMain, 'remote')
            w.loadFile(path.join(fixtures, 'api', 'blank.html'))
            const [, remote] = await p
            expect(remote).to.equal('object')
          })

          it('disables the remote module when false', async () => {
            const w = await openTheWindow({
              show: false,
              webPreferences: {
                preload,
                sandbox,
                enableRemoteModule: false
              }
            })
            const p = emittedOnce(ipcMain, 'remote')
            w.loadFile(path.join(fixtures, 'api', 'blank.html'))
            const [, remote] = await p
            expect(remote).to.equal('undefined')
          })
        })
      }

      generateSpecs('without sandbox', false)
      generateSpecs('with sandbox', true)
    })

    describe('"sandbox" option', () => {
      function waitForEvents (emitter, events, callback) {
        let count = events.length
        for (const event of events) {
          emitter.once(event, () => {
            if (!--count) callback()
          })
        }
      }

      const preload = path.join(fixtures, 'module', 'preload-sandbox.js')

      it('exposes ipcRenderer to preload script', (done) => {
        ipcMain.once('answer', function (event, test) {
          expect(test).to.equal('preload')
          done()
        })
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload
          }
        })
        w.loadFile(path.join(fixtures, 'api', 'preload.html'))
      })

      it('exposes ipcRenderer to preload script (path has special chars)', function (done) {
        const preloadSpecialChars = path.join(fixtures, 'module', 'preload-sandboxæø åü.js')
        ipcMain.once('answer', function (event, test) {
          expect(test).to.equal('preload')
          done()
        })
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload: preloadSpecialChars
          }
        })
        w.loadFile(path.join(fixtures, 'api', 'preload.html'))
      })

      it('exposes "loaded" event to preload script', function (done) {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload
          }
        })
        ipcMain.once('process-loaded', () => done())
        w.loadURL('about:blank')
      })

      it('exposes "exit" event to preload script', function (done) {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload
          }
        })
        const htmlPath = path.join(fixtures, 'api', 'sandbox.html?exit-event')
        const pageUrl = 'file://' + htmlPath
        ipcMain.once('answer', function (event, url) {
          let expectedUrl = pageUrl
          if (process.platform === 'win32') {
            expectedUrl = 'file:///' + htmlPath.replace(/\\/g, '/')
          }
          expect(url).to.equal(expectedUrl)
          done()
        })
        w.loadURL(pageUrl)
      })

      it('should open windows in same domain with cross-scripting enabled', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload
          }
        })
        ipcRenderer.send('set-web-preferences-on-next-new-window', w.webContents.id, 'preload', preload)
        const htmlPath = path.join(fixtures, 'api', 'sandbox.html?window-open')
        const pageUrl = 'file://' + htmlPath
        w.webContents.once('new-window', (e, url, frameName, disposition, options) => {
          let expectedUrl = pageUrl
          if (process.platform === 'win32') {
            expectedUrl = 'file:///' + htmlPath.replace(/\\/g, '/')
          }
          expect(url).to.equal(expectedUrl)
          expect(frameName).to.equal('popup!')
          expect(options.width).to.equal(500)
          expect(options.height).to.equal(600)
          ipcMain.once('answer', function (event, html) {
            expect(html).to.equal('<h1>scripting from opener</h1>')
            done()
          })
        })
        w.loadURL(pageUrl)
      })

      it('should open windows in another domain with cross-scripting disabled', async () => {
        const w = await openTheWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload
          }
        })

        ipcRenderer.send('set-web-preferences-on-next-new-window', w.webContents.id, 'preload', preload)
        const openerWindowOpen = emittedOnce(ipcMain, 'opener-loaded')
        w.loadFile(
          path.join(fixtures, 'api', 'sandbox.html'),
          { search: 'window-open-external' }
        )

        // Wait for a message from the main window saying that it's ready.
        await openerWindowOpen

        // Ask the opener to open a popup with window.opener.
        const expectedPopupUrl = `${server.url}/cross-site` // Set in "sandbox.html".

        const browserWindowCreated = emittedOnce(app, 'browser-window-created')
        w.webContents.send('open-the-popup', expectedPopupUrl)

        // The page is going to open a popup that it won't be able to close.
        // We have to close it from here later.
        // XXX(alexeykuzmin): It will leak if the test fails too soon.
        const [, popupWindow] = await browserWindowCreated

        // Ask the popup window for details.
        const detailsAnswer = emittedOnce(ipcMain, 'child-loaded')
        popupWindow.webContents.send('provide-details')
        const [, openerIsNull, , locationHref] = await detailsAnswer
        expect(openerIsNull).to.be.false('window.opener is null')
        expect(locationHref).to.equal(expectedPopupUrl)

        // Ask the page to access the popup.
        const touchPopupResult = emittedOnce(ipcMain, 'answer')
        w.webContents.send('touch-the-popup')
        const [, popupAccessMessage] = await touchPopupResult

        // Ask the popup to access the opener.
        const touchOpenerResult = emittedOnce(ipcMain, 'answer')
        popupWindow.webContents.send('touch-the-opener')
        const [, openerAccessMessage] = await touchOpenerResult

        // We don't need the popup anymore, and its parent page can't close it,
        // so let's close it from here before we run any checks.
        await closeWindow(popupWindow, { assertSingleWindow: false })

        expect(popupAccessMessage).to.be.a('string',
          `child's .document is accessible from its parent window`)
        expect(popupAccessMessage).to.match(/^Blocked a frame with origin/)
        expect(openerAccessMessage).to.be.a('string',
          `opener .document is accessible from a popup window`)
        expect(openerAccessMessage).to.match(/^Blocked a frame with origin/)
      })

      it('should inherit the sandbox setting in opened windows', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true
          }
        })

        const preloadPath = path.join(fixtures, 'api', 'new-window-preload.js')
        ipcRenderer.send('set-web-preferences-on-next-new-window', w.webContents.id, 'preload', preloadPath)
        ipcMain.once('answer', (event, args) => {
          expect(args).to.include('--enable-sandbox')
          done()
        })
        w.loadFile(path.join(fixtures, 'api', 'new-window.html'))
      })

      it('should open windows with the options configured via new-window event listeners', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true
          }
        })

        const preloadPath = path.join(fixtures, 'api', 'new-window-preload.js')
        ipcRenderer.send('set-web-preferences-on-next-new-window', w.webContents.id, 'preload', preloadPath)
        ipcRenderer.send('set-web-preferences-on-next-new-window', w.webContents.id, 'foo', 'bar')
        ipcMain.once('answer', (event, args, webPreferences) => {
          expect(webPreferences.foo).to.equal('bar')
          done()
        })
        w.loadFile(path.join(fixtures, 'api', 'new-window.html'))
      })

      it('should set ipc event sender correctly', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload
          }
        })
        ipcRenderer.send('set-web-preferences-on-next-new-window', w.webContents.id, 'preload', preload)

        let childWc
        w.webContents.once('new-window', (e, url, frameName, disposition, options) => {
          childWc = options.webContents
          expect(w.webContents).to.not.equal(childWc)
        })
        ipcMain.once('parent-ready', function (event) {
          expect(w.webContents).to.equal(event.sender)
          event.sender.send('verified')
        })
        ipcMain.once('child-ready', function (event) {
          expect(childWc).to.be.an('object')
          expect(childWc).to.equal(event.sender)
          event.sender.send('verified')
        })
        waitForEvents(ipcMain, [
          'parent-answer',
          'child-answer'
        ], done)
        w.loadFile(path.join(fixtures, 'api', 'sandbox.html'), { search: 'verify-ipc-sender' })
      })

      describe('event handling', () => {
        it('works for window events', (done) => {
          waitForEvents(w, [
            'page-title-updated'
          ], done)
          w.loadFile(path.join(fixtures, 'api', 'sandbox.html'), { search: 'window-events' })
        })

        it('works for stop events', (done) => {
          waitForEvents(w.webContents, [
            'did-navigate',
            'did-fail-load',
            'did-stop-loading'
          ], done)
          w.loadFile(path.join(fixtures, 'api', 'sandbox.html'), { search: 'webcontents-stop' })
        })

        it('works for web contents events', (done) => {
          waitForEvents(w.webContents, [
            'did-finish-load',
            'did-frame-finish-load',
            'did-navigate-in-page',
            'will-navigate',
            'did-start-loading',
            'did-stop-loading',
            'did-frame-finish-load',
            'dom-ready'
          ], done)
          w.loadFile(path.join(fixtures, 'api', 'sandbox.html'), { search: 'webcontents-events' })
        })
      })

      it('supports calling preventDefault on new-window events', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true
          }
        })
        const initialWebContents = webContents.getAllWebContents().map((i) => i.id)
        ipcRenderer.send('prevent-next-new-window', w.webContents.id)
        w.webContents.once('new-window', () => {
          // We need to give it some time so the windows get properly disposed (at least on OSX).
          setTimeout(() => {
            const currentWebContents = webContents.getAllWebContents().map((i) => i.id)
            expect(currentWebContents).to.deep.equal(initialWebContents)
            done()
          }, 100)
        })
        w.loadFile(path.join(fixtures, 'pages', 'window-open.html'))
      })

      // see #9387
      it('properly manages remote object references after page reload', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            preload,
            sandbox: true
          }
        })
        w.loadFile(path.join(fixtures, 'api', 'sandbox.html'), { search: 'reload-remote' })

        ipcMain.on('get-remote-module-path', (event) => {
          event.returnValue = path.join(fixtures, 'module', 'hello.js')
        })

        let reload = false
        ipcMain.on('reloaded', (event) => {
          event.returnValue = reload
          reload = !reload
        })

        ipcMain.once('reload', (event) => {
          event.sender.reload()
        })

        ipcMain.once('answer', (event, arg) => {
          ipcMain.removeAllListeners('reloaded')
          ipcMain.removeAllListeners('get-remote-module-path')
          expect(arg).to.equal('hi')
          done()
        })
      })

      it('properly manages remote object references after page reload in child window', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            preload,
            sandbox: true
          }
        })
        ipcRenderer.send('set-web-preferences-on-next-new-window', w.webContents.id, 'preload', preload)

        w.loadFile(path.join(fixtures, 'api', 'sandbox.html'), { search: 'reload-remote-child' })

        ipcMain.on('get-remote-module-path', (event) => {
          event.returnValue = path.join(fixtures, 'module', 'hello-child.js')
        })

        let reload = false
        ipcMain.on('reloaded', (event) => {
          event.returnValue = reload
          reload = !reload
        })

        ipcMain.once('reload', (event) => {
          event.sender.reload()
        })

        ipcMain.once('answer', (event, arg) => {
          ipcMain.removeAllListeners('reloaded')
          ipcMain.removeAllListeners('get-remote-module-path')
          expect(arg).to.equal('hi child window')
          done()
        })
      })

      it('validates process APIs access in sandboxed renderer', (done) => {
        ipcMain.once('answer', function (event, test) {
          expect(test.hasCrash).to.be.true()
          expect(test.hasHang).to.be.true()
          expect(test.heapStatistics).to.be.an('object')
          expect(test.blinkMemoryInfo).to.be.an('object')
          expect(test.processMemoryInfo).to.be.an('object')
          expect(test.systemVersion).to.be.a('string')
          expect(test.cpuUsage).to.be.an('object')
          expect(test.ioCounters).to.be.an('object')
          expect(test.arch).to.equal(remote.process.arch)
          expect(test.platform).to.equal(remote.process.platform)
          expect(test.env).to.deep.equal(remote.process.env)
          expect(test.execPath).to.equal(remote.process.helperExecPath)
          expect(test.sandboxed).to.be.true()
          expect(test.type).to.equal('renderer')
          expect(test.version).to.equal(remote.process.version)
          expect(test.versions).to.deep.equal(remote.process.versions)

          if (process.platform === 'linux' && test.osSandbox) {
            expect(test.creationTime).to.be.null()
            expect(test.systemMemoryInfo).to.be.null()
          } else {
            expect(test.creationTime).to.be.a('number')
            expect(test.systemMemoryInfo).to.be.an('object')
          }

          done()
        })
        remote.process.env.sandboxmain = 'foo'
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload
          }
        })
        w.webContents.once('preload-error', (event, preloadPath, error) => {
          done(error)
        })
        w.loadFile(path.join(fixtures, 'api', 'preload.html'))
      })

      it('webview in sandbox renderer', async () => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload,
            webviewTag: true
          }
        })
        const didAttachWebview = emittedOnce(w.webContents, 'did-attach-webview')
        const webviewDomReady = emittedOnce(ipcMain, 'webview-dom-ready')
        w.loadFile(path.join(fixtures, 'pages', 'webview-did-attach-event.html'))

        const [, webContents] = await didAttachWebview
        const [, id] = await webviewDomReady
        expect(webContents.id).to.equal(id)
      })
    })

    describe('nativeWindowOpen option', () => {
      const networkSandbox = createNetworkSandbox(protocol)

      beforeEach(async () => {
        // used to create cross-origin navigation situations
        await networkSandbox.serveFileFromProtocol('foo', path.join(fixtures, 'api', 'window-open-location-change.html'))
        await networkSandbox.serveFileFromProtocol('bar', path.join(fixtures, 'api', 'window-open-location-final.html'))

        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            nativeWindowOpen: true,
            // tests relies on preloads in opened windows
            nodeIntegrationInSubFrames: true
          }
        })
      })

      afterEach(async () => {
        await networkSandbox.reset()
      })

      it('opens window of about:blank with cross-scripting enabled', (done) => {
        ipcMain.once('answer', (event, content) => {
          expect(content).to.equal('Hello')
          done()
        })
        w.loadFile(path.join(fixtures, 'api', 'native-window-open-blank.html'))
      })
      it('opens window of same domain with cross-scripting enabled', (done) => {
        ipcMain.once('answer', (event, content) => {
          expect(content).to.equal('Hello')
          done()
        })
        w.loadFile(path.join(fixtures, 'api', 'native-window-open-file.html'))
      })
      it('blocks accessing cross-origin frames', (done) => {
        ipcMain.once('answer', (event, content) => {
          expect(content).to.equal('Blocked a frame with origin "file://" from accessing a cross-origin frame.')
          done()
        })
        w.loadFile(path.join(fixtures, 'api', 'native-window-open-cross-origin.html'))
      })
      it('opens window from <iframe> tags', (done) => {
        ipcMain.once('answer', (event, content) => {
          expect(content).to.equal('Hello')
          done()
        })
        w.loadFile(path.join(fixtures, 'api', 'native-window-open-iframe.html'))
      });
      (nativeModulesEnabled ? it : it.skip)('loads native addons correctly after reload', (done) => {
        ipcMain.once('answer', (event, content) => {
          expect(content).to.equal('function')
          ipcMain.once('answer', (event, content) => {
            expect(content).to.equal('function')
            done()
          })
          w.reload()
        })
        w.loadFile(path.join(fixtures, 'api', 'native-window-open-native-addon.html'))
      })
      it('should inherit the nativeWindowOpen setting in opened windows', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            nativeWindowOpen: true,
            // test relies on preloads in opened window
            nodeIntegrationInSubFrames: true
          }
        })

        const preloadPath = path.join(fixtures, 'api', 'new-window-preload.js')
        ipcRenderer.send('set-web-preferences-on-next-new-window', w.webContents.id, 'preload', preloadPath)
        ipcMain.once('answer', (event, args) => {
          expect(args).to.include('--native-window-open')
          done()
        })
        w.loadFile(path.join(fixtures, 'api', 'new-window.html'))
      })
      it('should open windows with the options configured via new-window event listeners', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            nativeWindowOpen: true,
            // test relies on preloads in opened window
            nodeIntegrationInSubFrames: true
          }
        })

        const preloadPath = path.join(fixtures, 'api', 'new-window-preload.js')
        ipcRenderer.send('set-web-preferences-on-next-new-window', w.webContents.id, 'preload', preloadPath)
        ipcRenderer.send('set-web-preferences-on-next-new-window', w.webContents.id, 'foo', 'bar')
        ipcMain.once('answer', (event, args, webPreferences) => {
          expect(webPreferences.foo).to.equal('bar')
          done()
        })
        w.loadFile(path.join(fixtures, 'api', 'new-window.html'))
      })
      it('retains the original web preferences when window.location is changed to a new origin', async () => {
        w.destroy()
        w = new BrowserWindow({
          show: true,
          webPreferences: {
            nativeWindowOpen: true,
            // test relies on preloads in opened window
            nodeIntegrationInSubFrames: true
          }
        })

        ipcRenderer.send('set-web-preferences-on-next-new-window', w.webContents.id, 'preload', path.join(fixtures, 'api', 'window-open-preload.js'))
        const p = emittedOnce(ipcMain, 'answer')
        w.loadFile(path.join(fixtures, 'api', 'window-open-location-open.html'))
        const [, args, typeofProcess] = await p
        expect(args).not.to.include('--node-integration')
        expect(args).to.include('--native-window-open')
        expect(typeofProcess).to.eql('undefined')
      })

      it('window.opener is not null when window.location is changed to a new origin', async () => {
        w.destroy()
        w = new BrowserWindow({
          show: true,
          webPreferences: {
            nativeWindowOpen: true,
            // test relies on preloads in opened window
            nodeIntegrationInSubFrames: true
          }
        })

        ipcRenderer.send('set-web-preferences-on-next-new-window', w.webContents.id, 'preload', path.join(fixtures, 'api', 'window-open-preload.js'))
        const p = emittedOnce(ipcMain, 'answer')
        w.loadFile(path.join(fixtures, 'api', 'window-open-location-open.html'))
        const [, , , windowOpenerIsNull] = await p
        expect(windowOpenerIsNull).to.be.false('window.opener is null')
      })

      it('should have nodeIntegration disabled in child windows', async () => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            nativeWindowOpen: true
          }
        })
        const p = emittedOnce(ipcMain, 'answer')
        w.loadFile(path.join(fixtures, 'api', 'native-window-open-argv.html'))
        const [, typeofProcess] = await p
        expect(typeofProcess).to.eql('undefined')
      })
    })

    describe('"disableHtmlFullscreenWindowResize" option', () => {
      it('prevents window from resizing when set', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            disableHtmlFullscreenWindowResize: true
          }
        })
        w.webContents.once('did-finish-load', () => {
          const size = w.getSize()
          w.webContents.once('enter-html-full-screen', () => {
            const newSize = w.getSize()
            expect(newSize).to.deep.equal(size)
            done()
          })
          w.webContents.executeJavaScript('document.body.webkitRequestFullscreen()', true)
        })
        w.loadURL('about:blank')
      })
    })
  })

  describe('nativeWindowOpen + contextIsolation options', () => {
    beforeEach(() => {
      w.destroy()
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nativeWindowOpen: true,
          contextIsolation: true,
          preload: path.join(fixtures, 'api', 'native-window-open-isolated-preload.js')
        }
      })
    })

    it('opens window with cross-scripting enabled from isolated context', (done) => {
      ipcMain.once('answer', (event, content) => {
        expect(content).to.equal('Hello')
        done()
      })
      w.loadFile(path.join(fixtures, 'api', 'native-window-open-isolated.html'))
    })
  })

  describe('beforeunload handler', () => {
    it('returning undefined would not prevent close', (done) => {
      w.once('closed', () => { done() })
      w.loadFile(path.join(fixtures, 'api', 'close-beforeunload-undefined.html'))
    })
    it('returning false would prevent close', (done) => {
      w.once('onbeforeunload', () => { done() })
      w.loadFile(path.join(fixtures, 'api', 'close-beforeunload-false.html'))
    })
    it('returning empty string would prevent close', (done) => {
      w.once('onbeforeunload', () => { done() })
      w.loadFile(path.join(fixtures, 'api', 'close-beforeunload-empty-string.html'))
    })
    it('emits for each close attempt', (done) => {
      let beforeUnloadCount = 0
      w.on('onbeforeunload', () => {
        beforeUnloadCount += 1
        if (beforeUnloadCount < 3) {
          w.close()
        } else if (beforeUnloadCount === 3) {
          done()
        }
      })
      w.webContents.once('did-finish-load', () => { w.close() })
      w.loadFile(path.join(fixtures, 'api', 'beforeunload-false-prevent3.html'))
    })
    it('emits for each reload attempt', (done) => {
      let beforeUnloadCount = 0
      w.on('onbeforeunload', () => {
        beforeUnloadCount += 1
        if (beforeUnloadCount < 3) {
          w.reload()
        } else if (beforeUnloadCount === 3) {
          done()
        }
      })
      w.webContents.once('did-finish-load', () => {
        w.webContents.once('did-finish-load', () => {
          expect.fail('Reload was not prevented')
        })
        w.reload()
      })
      w.loadFile(path.join(fixtures, 'api', 'beforeunload-false-prevent3.html'))
    })
    it('emits for each navigation attempt', (done) => {
      let beforeUnloadCount = 0
      w.on('onbeforeunload', () => {
        beforeUnloadCount += 1
        if (beforeUnloadCount < 3) {
          w.loadURL('about:blank')
        } else if (beforeUnloadCount === 3) {
          done()
        }
      })
      w.webContents.once('did-finish-load', () => {
        w.webContents.once('did-finish-load', () => {
          expect.fail('Navigation was not prevented')
        })
        w.loadURL('about:blank')
      })
      w.loadFile(path.join(fixtures, 'api', 'beforeunload-false-prevent3.html'))
    })
  })

  // visibilitychange event is broken upstream, see crbug.com/920839
  xdescribe('document.visibilityState/hidden', () => {
    beforeEach(() => { w.destroy() })

    function onVisibilityChange (callback) {
      ipcMain.on('pong', (event, visibilityState, hidden) => {
        if (event.sender.id === w.webContents.id) {
          callback(visibilityState, hidden)
        }
      })
    }

    function onNextVisibilityChange (callback) {
      ipcMain.once('pong', (event, visibilityState, hidden) => {
        if (event.sender.id === w.webContents.id) {
          callback(visibilityState, hidden)
        }
      })
    }

    afterEach(() => { ipcMain.removeAllListeners('pong') })

    it('visibilityState is initially visible despite window being hidden', (done) => {
      w = new BrowserWindow({
        show: false,
        width: 100,
        height: 100,
        webPreferences: {
          nodeIntegration: true
        }
      })

      let readyToShow = false
      w.once('ready-to-show', () => {
        readyToShow = true
      })

      onNextVisibilityChange((visibilityState, hidden) => {
        expect(readyToShow).to.be.false()
        expect(visibilityState).to.equal('visible')
        expect(hidden).to.be.false()

        done()
      })

      w.loadFile(path.join(fixtures, 'pages', 'visibilitychange.html'))
    })
    it('visibilityState changes when window is hidden', (done) => {
      w = new BrowserWindow({
        width: 100,
        height: 100,
        webPreferences: {
          nodeIntegration: true
        }
      })

      onNextVisibilityChange((visibilityState, hidden) => {
        expect(visibilityState).to.equal('visible')
        expect(hidden).to.be.false()

        onNextVisibilityChange((visibilityState, hidden) => {
          expect(visibilityState).to.equal('hidden')
          expect(hidden).to.be.true()
          done()
        })

        w.hide()
      })

      w.loadFile(path.join(fixtures, 'pages', 'visibilitychange.html'))
    })
    it('visibilityState changes when window is shown', (done) => {
      w = new BrowserWindow({
        width: 100,
        height: 100,
        webPreferences: {
          nodeIntegration: true
        }
      })

      onNextVisibilityChange((visibilityState, hidden) => {
        onVisibilityChange((visibilityState, hidden) => {
          if (!hidden) {
            expect(visibilityState).to.equal('visible')
            done()
          }
        })

        w.hide()
        w.show()
      })

      w.loadFile(path.join(fixtures, 'pages', 'visibilitychange.html'))
    })
    it('visibilityState changes when window is shown inactive', function (done) {
      if (isCI && process.platform === 'win32') {
        // FIXME(alexeykuzmin): Skip the test instead of marking it as passed.
        // afterEach hook won't be run if a test is skipped dynamically.
        // If afterEach isn't run current window won't be destroyed
        // and the next test will fail on assertion in `closeWindow()`.
        // this.skip()
        return done()
      }

      w = new BrowserWindow({
        width: 100,
        height: 100,
        webPreferences: {
          nodeIntegration: true
        }
      })

      onNextVisibilityChange((visibilityState, hidden) => {
        onVisibilityChange((visibilityState, hidden) => {
          if (!hidden) {
            expect(visibilityState).to.equal('visible')
            done()
          }
        })

        w.hide()
        w.showInactive()
      })

      w.loadFile(path.join(fixtures, 'pages', 'visibilitychange.html'))
    })
    it('visibilityState changes when window is minimized', function (done) {
      if (isCI && process.platform === 'linux') {
        // FIXME(alexeykuzmin): Skip the test instead of marking it as passed.
        // afterEach hook won't be run if a test is skipped dynamically.
        // If afterEach isn't run current window won't be destroyed
        // and the next test will fail on assertion in `closeWindow()`.
        // this.skip()
        return done()
      }

      w = new BrowserWindow({
        width: 100,
        height: 100,
        webPreferences: {
          nodeIntegration: true
        }
      })

      onNextVisibilityChange((visibilityState, hidden) => {
        expect(visibilityState).to.equal('visible')
        expect(hidden).to.be.false()

        onNextVisibilityChange((visibilityState, hidden) => {
          expect(visibilityState).to.equal('hidden')
          expect(hidden).to.be.true()
          done()
        })

        w.minimize()
      })

      w.loadFile(path.join(fixtures, 'pages', 'visibilitychange.html'))
    })
    it('visibilityState remains visible if backgroundThrottling is disabled', (done) => {
      w = new BrowserWindow({
        show: false,
        width: 100,
        height: 100,
        webPreferences: {
          backgroundThrottling: false,
          nodeIntegration: true
        }
      })

      onNextVisibilityChange((visibilityState, hidden) => {
        expect(visibilityState).to.equal('visible')
        expect(hidden).to.be.false()

        onNextVisibilityChange((visibilityState, hidden) => {
          done(new Error(`Unexpected visibility change event. visibilityState: ${visibilityState} hidden: ${hidden}`))
        })
      })

      w.once('show', () => {
        w.once('hide', () => {
          w.once('show', () => {
            done()
          })
          w.show()
        })
        w.hide()
      })
      w.show()

      w.loadFile(path.join(fixtures, 'pages', 'visibilitychange.html'))
    })
  })

  describe('new-window event', () => {
    before(function () {
      if (isCI && process.platform === 'darwin') {
        this.skip()
      }
    })

    it('emits when window.open is called', (done) => {
      w.webContents.once('new-window', (e, url, frameName, disposition, options, additionalFeatures) => {
        e.preventDefault()
        expect(url).to.equal('http://host/')
        expect(frameName).to.equal('host')
        expect(additionalFeatures[0]).to.equal('this-is-not-a-standard-feature')
        done()
      })
      w.loadFile(path.join(fixtures, 'pages', 'window-open.html'))
    })
    it('emits when window.open is called with no webPreferences', (done) => {
      w.destroy()
      w = new BrowserWindow({ show: false })
      w.webContents.once('new-window', function (e, url, frameName, disposition, options, additionalFeatures) {
        e.preventDefault()
        expect(url).to.equal('http://host/')
        expect(frameName).to.equal('host')
        expect(additionalFeatures[0]).to.equal('this-is-not-a-standard-feature')
        done()
      })
      w.loadFile(path.join(fixtures, 'pages', 'window-open.html'))
    })

    it('emits when link with target is called', (done) => {
      w.webContents.once('new-window', (e, url, frameName) => {
        e.preventDefault()
        expect(url).to.equal('http://host/')
        expect(frameName).to.equal('target')
        done()
      })
      w.loadFile(path.join(fixtures, 'pages', 'target-name.html'))
    })
  })

  describe('maximize event', () => {
    if (isCI) return

    it('emits when window is maximized', (done) => {
      w.once('maximize', () => { done() })
      w.show()
      w.maximize()
    })
  })

  describe('unmaximize event', () => {
    if (isCI) return

    it('emits when window is unmaximized', (done) => {
      w.once('unmaximize', () => { done() })
      w.show()
      w.maximize()
      w.unmaximize()
    })
  })

  describe('minimize event', () => {
    if (isCI) return

    it('emits when window is minimized', (done) => {
      w.once('minimize', () => { done() })
      w.show()
      w.minimize()
    })
  })

  describe('focus event', () => {
    it('should not emit if focusing on a main window with a modal open', (done) => {
      const childWindowClosed = false
      const child = new BrowserWindow({
        parent: w,
        modal: true,
        show: false
      })

      child.once('ready-to-show', () => {
        child.show()
      })

      child.on('show', () => {
        w.once('focus', () => {
          expect(child.isDestroyed()).to.equal(true)
          done()
        })
        w.focus() // this should not trigger the above listener
        child.close()
      })

      // act
      child.loadURL(server.url)
      w.show()
    })
  })

  describe('sheet-begin event', () => {
    let sheet = null

    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    afterEach(() => {
      return closeWindow(sheet, { assertSingleWindow: false }).then(() => { sheet = null })
    })

    it('emits when window opens a sheet', (done) => {
      w.show()
      w.once('sheet-begin', () => {
        sheet.close()
        done()
      })
      sheet = new BrowserWindow({
        modal: true,
        parent: w
      })
    })
  })

  describe('sheet-end event', () => {
    let sheet = null

    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    afterEach(() => {
      return closeWindow(sheet, { assertSingleWindow: false }).then(() => { sheet = null })
    })

    it('emits when window has closed a sheet', (done) => {
      w.show()
      sheet = new BrowserWindow({
        modal: true,
        parent: w
      })
      w.once('sheet-end', () => { done() })
      sheet.close()
    })
  })

  describe('beginFrameSubscription method', () => {
    it('does not crash when callback returns nothing', (done) => {
      w.loadFile(path.join(fixtures, 'api', 'frame-subscriber.html'))
      w.webContents.on('dom-ready', () => {
        w.webContents.beginFrameSubscription(function (data) {
          // Pending endFrameSubscription to next tick can reliably reproduce
          // a crash which happens when nothing is returned in the callback.
          setTimeout(() => {
            w.webContents.endFrameSubscription()
            done()
          })
        })
      })
    })

    it('subscribes to frame updates', (done) => {
      let called = false
      w.loadFile(path.join(fixtures, 'api', 'frame-subscriber.html'))
      w.webContents.on('dom-ready', () => {
        w.webContents.beginFrameSubscription(function (data) {
          // This callback might be called twice.
          if (called) return
          called = true

          expect(data.constructor.name).to.equal('NativeImage')
          expect(data.isEmpty()).to.be.false()

          w.webContents.endFrameSubscription()
          done()
        })
      })
    })

    it('subscribes to frame updates (only dirty rectangle)', (done) => {
      let called = false
      let gotInitialFullSizeFrame = false
      const [contentWidth, contentHeight] = w.getContentSize()
      w.webContents.on('did-finish-load', () => {
        w.webContents.beginFrameSubscription(true, (image, rect) => {
          if (image.isEmpty()) {
            // Chromium sometimes sends a 0x0 frame at the beginning of the
            // page load.
            return
          }
          if (rect.height === contentHeight && rect.width === contentWidth &&
              !gotInitialFullSizeFrame) {
            // The initial frame is full-size, but we're looking for a call
            // with just the dirty-rect. The next frame should be a smaller
            // rect.
            gotInitialFullSizeFrame = true
            return
          }
          // This callback might be called twice.
          if (called) return
          // We asked for just the dirty rectangle, so we expect to receive a
          // rect smaller than the full size.
          // TODO(jeremy): this is failing on windows currently; investigate.
          // assert(rect.width < contentWidth || rect.height < contentHeight)
          called = true

          const expectedSize = rect.width * rect.height * 4
          expect(image.getBitmap()).to.be.an.instanceOf(Buffer).with.lengthOf(expectedSize)
          w.webContents.endFrameSubscription()
          done()
        })
      })
      w.loadFile(path.join(fixtures, 'api', 'frame-subscriber.html'))
    })

    it('throws error when subscriber is not well defined', (done) => {
      w.loadFile(path.join(fixtures, 'api', 'frame-subscriber.html'))
      try {
        w.webContents.beginFrameSubscription(true, true)
      } catch (e) {
        done()
      }
    })
  })

  describe('savePage method', () => {
    const savePageDir = path.join(fixtures, 'save_page')
    const savePageHtmlPath = path.join(savePageDir, 'save_page.html')
    const savePageJsPath = path.join(savePageDir, 'save_page_files', 'test.js')
    const savePageCssPath = path.join(savePageDir, 'save_page_files', 'test.css')

    after(() => {
      try {
        fs.unlinkSync(savePageCssPath)
        fs.unlinkSync(savePageJsPath)
        fs.unlinkSync(savePageHtmlPath)
        fs.rmdirSync(path.join(savePageDir, 'save_page_files'))
        fs.rmdirSync(savePageDir)
      } catch (e) {
        // Ignore error
      }
    })

    it('should save page to disk', async () => {
      await w.loadFile(path.join(fixtures, 'pages', 'save_page', 'index.html'))
      await w.webContents.savePage(savePageHtmlPath, 'HTMLComplete')

      expect(fs.existsSync(savePageHtmlPath)).to.be.true()
      expect(fs.existsSync(savePageJsPath)).to.be.true()
      expect(fs.existsSync(savePageCssPath)).to.be.true()
    })
  })

  describe('BrowserWindow options argument is optional', () => {
    it('should create a window with default size (800x600)', () => {
      w.destroy()
      w = new BrowserWindow()
      const size = w.getSize()
      expect(size).to.deep.equal([800, 600])
    })
  })

  describe('window states', () => {
    it('does not resize frameless windows when states change', () => {
      w.destroy()
      w = new BrowserWindow({
        frame: false,
        width: 300,
        height: 200,
        show: false
      })

      w.minimizable = false
      w.minimizable = true
      expect(w.getSize()).to.deep.equal([300, 200])

      w.resizable = false
      w.resizable = true
      expect(w.getSize()).to.deep.equal([300, 200])

      w.maximizable = false
      w.maximizable = true
      expect(w.getSize()).to.deep.equal([300, 200])

      w.fullScreenable = false
      w.fullScreenable = true
      expect(w.getSize()).to.deep.equal([300, 200])

      w.closable = false
      w.closable = true
      expect(w.getSize()).to.deep.equal([300, 200])
    })

    describe('resizable state', () => {
      it('can be changed with resizable option', () => {
        w.destroy()
        w = new BrowserWindow({ show: false, resizable: false })
        expect(w.resizable).to.be.false()

        if (process.platform === 'darwin') {
          expect(w.maximizable).to.to.true()
        }
      })

      // TODO(codebytere): remove when propertyification is complete
      it('can be changed with setResizable method', () => {
        expect(w.isResizable()).to.be.true()
        w.setResizable(false)
        expect(w.isResizable()).to.be.false()
        w.setResizable(true)
        expect(w.isResizable()).to.be.true()
      })

      it('can be changed with resizable property', () => {
        expect(w.resizable).to.be.true()
        w.resizable = false
        expect(w.resizable).to.be.false()
        w.resizable = true
        expect(w.resizable).to.be.true()
      })

      it('works for a frameless window', () => {
        w.destroy()
        w = new BrowserWindow({ show: false, frame: false })
        expect(w.resizable).to.be.true()

        if (process.platform === 'win32') {
          w.destroy()
          w = new BrowserWindow({ show: false, thickFrame: false })
          expect(w.resizable).to.be.false()
        }
      })

      if (process.platform === 'win32') {
        it('works for a window smaller than 64x64', () => {
          w.destroy()
          w = new BrowserWindow({
            show: false,
            frame: false,
            resizable: false,
            transparent: true
          })
          w.setContentSize(60, 60)
          expectBoundsEqual(w.getContentSize(), [60, 60])
          w.setContentSize(30, 30)
          expectBoundsEqual(w.getContentSize(), [30, 30])
          w.setContentSize(10, 10)
          expectBoundsEqual(w.getContentSize(), [10, 10])
        })
      }
    })

    describe('loading main frame state', () => {
      it('is true when the main frame is loading', (done) => {
        w.webContents.on('did-start-loading', () => {
          expect(w.webContents.isLoadingMainFrame()).to.be.true()
          done()
        })
        w.webContents.loadURL(server.url)
      })
      it('is false when only a subframe is loading', (done) => {
        w.webContents.once('did-finish-load', () => {
          expect(w.webContents.isLoadingMainFrame()).to.be.false()
          w.webContents.on('did-start-loading', () => {
            expect(w.webContents.isLoadingMainFrame()).to.be.false()
            done()
          })
          w.webContents.executeJavaScript(`
            var iframe = document.createElement('iframe')
            iframe.src = '${server.url}/page2'
            document.body.appendChild(iframe)
          `)
        })
        w.webContents.loadURL(server.url)
      })
      it('is true when navigating to pages from the same origin', (done) => {
        w.webContents.once('did-finish-load', () => {
          expect(w.webContents.isLoadingMainFrame()).to.be.false()
          w.webContents.on('did-start-loading', () => {
            expect(w.webContents.isLoadingMainFrame()).to.be.true()
            done()
          })
          w.webContents.loadURL(`${server.url}/page2`)
        })
        w.webContents.loadURL(server.url)
      })
    })
  })

  describe('window states (excluding Linux)', () => {
    // FIXME(alexeykuzmin): Skip the tests instead of using the `return` here.
    // Why it cannot be done now:
    // - `.skip()` called in the 'before' hook doesn't affect
    //     nested `describe`s.
    // - `.skip()` called in the 'beforeEach' hook prevents 'afterEach'
    //     hook from being called.
    // Not implemented on Linux.
    if (process.platform === 'linux') {
      return
    }

    describe('movable state (property)', () => {
      it('can be changed with movable option', () => {
        w.destroy()
        w = new BrowserWindow({ show: false, movable: false })
        expect(w.movable).to.be.false()
      })
      it('can be changed with movable property', () => {
        expect(w.movable).to.be.true()
        w.movable = false
        expect(w.movable).to.be.false()
        w.movable = true
        expect(w.movable).to.be.true()
      })
    })

    // TODO(codebytere): remove when propertyification is complete
    describe('movable state (methods)', () => {
      it('can be changed with movable option', () => {
        w.destroy()
        w = new BrowserWindow({ show: false, movable: false })
        expect(w.isMovable()).to.be.false()
      })
      it('can be changed with setMovable method', () => {
        expect(w.isMovable()).to.be.true()
        w.setMovable(false)
        expect(w.isMovable()).to.be.false()
        w.setMovable(true)
        expect(w.isMovable()).to.be.true()
      })
    })

    describe('minimizable state (property)', () => {
      it('can be changed with minimizable option', () => {
        w.destroy()
        w = new BrowserWindow({ show: false, minimizable: false })
        expect(w.minimizable).to.be.false()
      })

      it('can be changed with minimizable property', () => {
        expect(w.minimizable).to.be.true()
        w.minimizable = false
        expect(w.minimizable).to.be.false()
        w.minimizable = true
        expect(w.minimizable).to.be.true()
      })
    })

    // TODO(codebytere): remove when propertyification is complete
    describe('minimizable state (methods)', () => {
      it('can be changed with minimizable option', () => {
        w.destroy()
        w = new BrowserWindow({ show: false, minimizable: false })
        expect(w.isMinimizable()).to.be.false()
      })

      it('can be changed with setMinimizable method', () => {
        expect(w.isMinimizable()).to.be.true()
        w.setMinimizable(false)
        expect(w.isMinimizable()).to.be.false()
        w.setMinimizable(true)
        expect(w.isMinimizable()).to.be.true()
      })
    })

    describe('maximizable state (property)', () => {
      it('can be changed with maximizable option', () => {
        w.destroy()
        w = new BrowserWindow({ show: false, maximizable: false })
        expect(w.maximizable).to.be.false()
      })

      it('can be changed with maximizable property', () => {
        expect(w.maximizable).to.be.true()
        w.maximizable = false
        expect(w.maximizable).to.be.false()
        w.maximizable = true
        expect(w.maximizable).to.be.true()
      })

      it('is not affected when changing other states', () => {
        w.maximizable = false
        expect(w.maximizable).to.be.false()
        w.minimizable = false
        expect(w.maximizable).to.be.false()
        w.closable = false
        expect(w.maximizable).to.be.false()

        w.maximizable = true
        expect(w.maximizable).to.be.true()
        w.closable = true
        expect(w.maximizable).to.be.true()
        w.fullScreenable = false
        expect(w.maximizable).to.be.true()
      })
    })

    // TODO(codebytere): remove when propertyification is complete
    describe('maximizable state (methods)', () => {
      it('can be changed with maximizable option', () => {
        w.destroy()
        w = new BrowserWindow({ show: false, maximizable: false })
        expect(w.isMaximizable()).to.be.false()
      })

      it('can be changed with setMaximizable method', () => {
        expect(w.isMaximizable()).to.be.true()
        w.setMaximizable(false)
        expect(w.isMaximizable()).to.be.false()
        w.setMaximizable(true)
        expect(w.isMaximizable()).to.be.true()
      })

      it('is not affected when changing other states', () => {
        w.setMaximizable(false)
        expect(w.isMaximizable()).to.be.false()
        w.setMinimizable(false)
        expect(w.isMaximizable()).to.be.false()
        w.setClosable(false)
        expect(w.isMaximizable()).to.be.false()

        w.setMaximizable(true)
        expect(w.isMaximizable()).to.be.true()
        w.setClosable(true)
        expect(w.isMaximizable()).to.be.true()
        w.setFullScreenable(false)
        expect(w.isMaximizable()).to.be.true()
      })
    })

    describe('maximizable state (Windows only)', () => {
      // Only implemented on windows.
      if (process.platform !== 'win32') return

      it('is reset to its former state', () => {
        w.maximizable = false
        w.resizable = false
        w.resizable = true
        expect(w.maximizable).to.be.false()
        w.maximizable = true
        w.resizable = false
        w.resizable = true
        expect(w.maximizable).to.be.true()
      })
    })

    // TODO(codebytere): remove when propertyification is complete
    describe('maximizable state (Windows only) (methods)', () => {
      // Only implemented on windows.
      if (process.platform !== 'win32') return

      it('is reset to its former state', () => {
        w.setMaximizable(false)
        w.setResizable(false)
        w.setResizable(true)
        expect(w.isMaximizable()).to.be.false()
        w.setMaximizable(true)
        w.setResizable(false)
        w.setResizable(true)
        expect(w.isMaximizable()).to.be.true()
      })
    })

    describe('fullscreenable state (property)', () => {
      before(function () {
        if (process.platform !== 'darwin') this.skip()
      })

      it('can be changed with fullscreenable option', () => {
        w.destroy()
        w = new BrowserWindow({ show: false, fullscreenable: false })
        expect(w.fullScreenable).to.be.false()
      })

      it('can be changed with fullScreenable property', () => {
        expect(w.fullScreenable).to.be.true()
        w.fullScreenable = false
        expect(w.fullScreenable).to.be.false()
        w.fullScreenable = true
        expect(w.fullScreenable).to.be.true()
      })
    })

    // TODO(codebytere): remove when propertyification is complete
    describe('fullscreenable state (methods)', () => {
      before(function () {
        if (process.platform !== 'darwin') this.skip()
      })

      it('can be changed with fullscreenable option', () => {
        w.destroy()
        w = new BrowserWindow({ show: false, fullscreenable: false })
        expect(w.isFullScreenable()).to.be.false()
      })

      it('can be changed with setFullScreenable method', () => {
        expect(w.isFullScreenable()).to.be.true()
        w.setFullScreenable(false)
        expect(w.isFullScreenable()).to.be.false()
        w.setFullScreenable(true)
        expect(w.isFullScreenable()).to.be.true()
      })
    })

    describe('kiosk state', () => {
      before(function () {
        // Only implemented on macOS.
        if (process.platform !== 'darwin') {
          this.skip()
        }
      })

      it('can be changed with setKiosk method', (done) => {
        w.destroy()
        w = new BrowserWindow()
        w.setKiosk(true)
        expect(w.isKiosk()).to.be.true()

        w.once('enter-full-screen', () => {
          w.setKiosk(false)
          expect(w.isKiosk()).to.be.false()
        })
        w.once('leave-full-screen', () => {
          done()
        })
      })
    })

    describe('fullscreen state with resizable set', () => {
      before(function () {
        if (process.platform !== 'darwin') this.skip()
      })

      it('resizable flag should be set to true and restored', (done) => {
        w.destroy()
        w = new BrowserWindow({ resizable: false })
        w.once('enter-full-screen', () => {
          expect(w.resizable).to.be.true()
          w.setFullScreen(false)
        })
        w.once('leave-full-screen', () => {
          expect(w.resizable).to.be.false()
          done()
        })
        w.setFullScreen(true)
      })
    })

    describe('fullscreen state', () => {
      before(function () {
        // Only implemented on macOS.
        if (process.platform !== 'darwin') {
          this.skip()
        }
      })

      it('can be changed with setFullScreen method', (done) => {
        w.destroy()
        w = new BrowserWindow()
        w.once('enter-full-screen', () => {
          expect(w.isFullScreen()).to.be.true()
          w.setFullScreen(false)
        })
        w.once('leave-full-screen', () => {
          expect(w.isFullScreen()).to.be.false()
          done()
        })
        w.setFullScreen(true)
      })

      it('should not be changed by setKiosk method', (done) => {
        w.destroy()
        w = new BrowserWindow()
        w.once('enter-full-screen', () => {
          expect(w.isFullScreen()).to.be.true()
          w.setKiosk(true)
          w.setKiosk(false)
          expect(w.isFullScreen()).to.be.true()
          w.setFullScreen(false)
        })
        w.once('leave-full-screen', () => {
          expect(w.isFullScreen()).to.be.false()
          done()
        })
        w.setFullScreen(true)
      })
    })

    describe('closable state (property)', () => {
      it('can be changed with closable option', () => {
        w.destroy()
        w = new BrowserWindow({ show: false, closable: false })
        expect(w.closable).to.be.false()
      })

      it('can be changed with setClosable method', () => {
        expect(w.closable).to.be.true()
        w.closable = false
        expect(w.closable).to.be.false()
        w.closable = true
        expect(w.closable).to.be.true()
      })
    })

    // TODO(codebytere): remove when propertyification is complete
    describe('closable state (methods)', () => {
      it('can be changed with closable option', () => {
        w.destroy()
        w = new BrowserWindow({ show: false, closable: false })
        expect(w.isClosable()).to.be.false()
      })

      it('can be changed with setClosable method', () => {
        expect(w.isClosable()).to.be.true()
        w.setClosable(false)
        expect(w.isClosable()).to.be.false()
        w.setClosable(true)
        expect(w.isClosable()).to.be.true()
      })
    })

    describe('hasShadow state', () => {
      // On Window there is no shadow by default and it can not be changed
      // dynamically.
      it('can be changed with hasShadow option', () => {
        w.destroy()
        const hasShadow = process.platform !== 'darwin'
        w = new BrowserWindow({ show: false, hasShadow: hasShadow })
        expect(w.hasShadow()).to.equal(hasShadow)
      })

      it('can be changed with setHasShadow method', () => {
        if (process.platform !== 'darwin') return

        expect(w.hasShadow()).to.be.true()
        w.setHasShadow(false)
        expect(w.hasShadow()).to.be.false()
        w.setHasShadow(true)
        expect(w.hasShadow()).to.be.true()
      })
    })
  })

  describe('BrowserWindow.restore()', () => {
    it('should restore the previous window size', () => {
      if (w != null) w.destroy()

      w = new BrowserWindow({
        minWidth: 800,
        width: 800
      })

      const initialSize = w.getSize()
      w.minimize()
      w.restore()
      expectBoundsEqual(w.getSize(), initialSize)
    })
  })

  describe('BrowserWindow.unmaximize()', () => {
    it('should restore the previous window position', () => {
      if (w != null) w.destroy()
      w = new BrowserWindow()

      const initialPosition = w.getPosition()
      w.maximize()
      w.unmaximize()
      expectBoundsEqual(w.getPosition(), initialPosition)
    })
  })

  describe('BrowserWindow.setFullScreen(false)', () => {
    before(function () {
      // only applicable to windows: https://github.com/electron/electron/issues/6036
      if (process.platform !== 'win32') {
        this.skip()
      }
    })

    it('should restore a normal visible window from a fullscreen startup state', (done) => {
      w.webContents.once('did-finish-load', () => {
        // start fullscreen and hidden
        w.setFullScreen(true)
        w.once('show', () => { w.setFullScreen(false) })
        w.once('leave-full-screen', () => {
          expect(w.isVisible()).to.be.true()
          expect(w.isFullScreen()).to.be.false()
          done()
        })
        w.show()
      })
      w.loadURL('about:blank')
    })
    it('should keep window hidden if already in hidden state', (done) => {
      w.webContents.once('did-finish-load', () => {
        w.once('leave-full-screen', () => {
          expect(w.isVisible()).to.be.false()
          expect(w.isFullScreen()).to.be.false()
          done()
        })
        w.setFullScreen(false)
      })
      w.loadURL('about:blank')
    })
  })

  describe('BrowserWindow.setFullScreen(false) when HTML fullscreen', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('exits HTML fullscreen when window leaves fullscreen', (done) => {
      w.destroy()
      w = new BrowserWindow()
      w.webContents.once('did-finish-load', () => {
        w.webContents.executeJavaScript('document.body.webkitRequestFullscreen()', true).then(() => {
          w.once('enter-full-screen', () => {
            w.once('leave-html-full-screen', () => {
              done()
            })
            w.setFullScreen(false)
          })
        })
      })
      w.loadURL('about:blank')
    })
  })

  describe('parent window', () => {
    let c = null

    beforeEach(() => {
      if (c != null) c.destroy()
      c = new BrowserWindow({ show: false, parent: w })
    })

    afterEach(() => {
      if (c != null) c.destroy()
      c = null
    })

    describe('parent option', () => {
      it('sets parent window', () => {
        expect(c.getParentWindow()).to.equal(w)
      })
      it('adds window to child windows of parent', () => {
        expect(w.getChildWindows()).to.deep.equal([c])
      })
      it('removes from child windows of parent when window is closed', (done) => {
        c.once('closed', () => {
          expect(w.getChildWindows()).to.deep.equal([])
          done()
        })
        c.close()
      })

      it('should not affect the show option', () => {
        expect(c.isVisible()).to.be.false()
        expect(c.getParentWindow().isVisible()).to.be.false()
      })
    })

    describe('win.setParentWindow(parent)', () => {
      beforeEach(() => {
        if (c != null) c.destroy()
        c = new BrowserWindow({ show: false })
      })

      it('sets parent window', () => {
        expect(w.getParentWindow()).to.be.null()
        expect(c.getParentWindow()).to.be.null()
        c.setParentWindow(w)
        expect(c.getParentWindow()).to.equal(w)
        c.setParentWindow(null)
        expect(c.getParentWindow()).to.be.null()
      })
      it('adds window to child windows of parent', () => {
        expect(w.getChildWindows()).to.deep.equal([])
        c.setParentWindow(w)
        expect(w.getChildWindows()).to.deep.equal([c])
        c.setParentWindow(null)
        expect(w.getChildWindows()).to.deep.equal([])
      })
      it('removes from child windows of parent when window is closed', (done) => {
        c.once('closed', () => {
          expect(w.getChildWindows()).to.deep.equal([])
          done()
        })
        c.setParentWindow(w)
        c.close()
      })
    })

    describe('modal option', () => {
      before(function () {
        // The isEnabled API is not reliable on macOS.
        if (process.platform === 'darwin') {
          this.skip()
        }
      })

      beforeEach(() => {
        if (c != null) c.destroy()
        c = new BrowserWindow({ show: false, parent: w, modal: true })
      })

      it('disables parent window', () => {
        expect(w.isEnabled()).to.be.true()
        c.show()
        expect(w.isEnabled()).to.be.false()
      })
      it('re-enables an enabled parent window when closed', (done) => {
        c.once('closed', () => {
          expect(w.isEnabled()).to.be.true()
          done()
        })
        c.show()
        c.close()
      })
      it('does not re-enable a disabled parent window when closed', (done) => {
        c.once('closed', () => {
          expect(w.isEnabled()).to.be.false()
          done()
        })
        w.setEnabled(false)
        c.show()
        c.close()
      })
      it('disables parent window recursively', () => {
        const c2 = new BrowserWindow({ show: false, parent: w, modal: true })
        c.show()
        expect(w.isEnabled()).to.be.false()
        c2.show()
        expect(w.isEnabled()).to.be.false()
        c.destroy()
        expect(w.isEnabled()).to.be.false()
        c2.destroy()
        expect(w.isEnabled()).to.be.true()
      })
    })
  })

  describe('window.webContents.send(channel, args...)', () => {
    it('throws an error when the channel is missing', () => {
      expect(() => {
        w.webContents.send()
      }).to.throw('Missing required channel argument')

      expect(() => {
        w.webContents.send(null)
      }).to.throw('Missing required channel argument')
    })
  })

  describe('window.getNativeWindowHandle()', () => {
    before(function () {
      if (!nativeModulesEnabled) {
        this.skip()
      }
    })

    it('returns valid handle', () => {
      // The module's source code is hosted at
      // https://github.com/electron/node-is-valid-window
      const isValidWindow = remote.require('is-valid-window')
      expect(isValidWindow(w.getNativeWindowHandle())).to.be.true()
    })
  })

  describe('extensions and dev tools extensions', () => {
    let showPanelTimeoutId

    const showLastDevToolsPanel = () => {
      w.webContents.once('devtools-opened', () => {
        const show = () => {
          if (w == null || w.isDestroyed()) return
          const { devToolsWebContents } = w
          if (devToolsWebContents == null || devToolsWebContents.isDestroyed()) {
            return
          }

          const showLastPanel = () => {
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
      clearTimeout(showPanelTimeoutId)
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

        const removeExtension = () => {
          BrowserWindow.removeDevToolsExtension('foo')
          expect(BrowserWindow.getDevToolsExtensions()).to.not.have.a.property(extensionName)
        }

        const addExtension = () => {
          const extensionPath = path.join(__dirname, 'fixtures', 'devtools-extensions', 'foo')
          BrowserWindow.addDevToolsExtension(extensionPath)
          expect(BrowserWindow.getDevToolsExtensions()).to.have.a.property(extensionName)

          showLastDevToolsPanel()

          w.loadURL('about:blank')
        }

        // After* hooks won't be called if a test fail.
        // So let's make a clean-up in the before hook.
        beforeEach(removeExtension)

        describe('when the devtools is docked', () => {
          beforeEach(function (done) {
            addExtension()
            w.webContents.openDevTools({ mode: 'bottom' })
            ipcMain.once('answer', (event, message) => {
              this.message = message
              done()
            })
          })

          describe('created extension info', function () {
            it('has proper "runtimeId"', function () {
              expect(this.message).to.have.own.property('runtimeId')
              expect(this.message.runtimeId).to.equal(extensionName)
            })
            it('has "tabId" matching webContents id', function () {
              expect(this.message).to.have.own.property('tabId')
              expect(this.message.tabId).to.equal(w.webContents.id)
            })
            it('has "i18nString" with proper contents', function () {
              expect(this.message).to.have.own.property('i18nString')
              expect(this.message.i18nString).to.equal('foo - bar (baz)')
            })
            it('has "storageItems" with proper contents', function () {
              expect(this.message).to.have.own.property('storageItems')
              expect(this.message.storageItems).to.deep.equal({
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
          beforeEach(function (done) {
            addExtension()
            w.webContents.openDevTools({ mode: 'undocked' })
            ipcMain.once('answer', (event, message, extensionId) => {
              this.message = message
              done()
            })
          })

          describe('created extension info', function () {
            it('has proper "runtimeId"', function () {
              expect(this.message).to.have.own.property('runtimeId')
              expect(this.message.runtimeId).to.equal(extensionName)
            })
            it('has "tabId" matching webContents id', function () {
              expect(this.message).to.have.own.property('tabId')
              expect(this.message.tabId).to.equal(w.webContents.id)
            })
          })
        })
      })
    })

    it('works when used with partitions', (done) => {
      if (w != null) {
        w.destroy()
      }
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          partition: 'temp'
        }
      })

      const extensionPath = path.join(__dirname, 'fixtures', 'devtools-extensions', 'foo')
      BrowserWindow.removeDevToolsExtension('foo')
      BrowserWindow.addDevToolsExtension(extensionPath)

      showLastDevToolsPanel()

      ipcMain.once('answer', function (event, message) {
        expect(message.runtimeId).to.equal('foo')
        done()
      })

      w.loadURL('about:blank')
      w.webContents.openDevTools({ mode: 'bottom' })
    })

    it('serializes the registered extensions on quit', () => {
      const extensionName = 'foo'
      const extensionPath = path.join(__dirname, 'fixtures', 'devtools-extensions', extensionName)
      const serializedPath = path.join(app.getPath('userData'), 'DevTools Extensions')

      BrowserWindow.addDevToolsExtension(extensionPath)
      app.emit('will-quit')
      expect(JSON.parse(fs.readFileSync(serializedPath))).to.deep.equal([extensionPath])

      BrowserWindow.removeDevToolsExtension(extensionName)
      app.emit('will-quit')
      expect(fs.existsSync(serializedPath)).to.be.false()
    })

    describe('BrowserWindow.addExtension', () => {
      beforeEach(() => {
        BrowserWindow.removeExtension('foo')
        expect(BrowserWindow.getExtensions()).to.not.have.property('foo')

        const extensionPath = path.join(__dirname, 'fixtures', 'devtools-extensions', 'foo')
        BrowserWindow.addExtension(extensionPath)
        expect(BrowserWindow.getExtensions()).to.have.property('foo')

        showLastDevToolsPanel()

        w.loadURL('about:blank')
      })

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

  describe('window.webContents.executeJavaScript', () => {
    const expected = 'hello, world!'
    const expectedErrorMsg = 'woops!'
    const code = `(() => "${expected}")()`
    const asyncCode = `(() => new Promise(r => setTimeout(() => r("${expected}"), 500)))()`
    const badAsyncCode = `(() => new Promise((r, e) => setTimeout(() => e("${expectedErrorMsg}"), 500)))()`
    const errorTypes = new Set([
      Error,
      ReferenceError,
      EvalError,
      RangeError,
      SyntaxError,
      TypeError,
      URIError
    ])

    it('resolves the returned promise with the result', (done) => {
      ipcRenderer.send('executeJavaScript', code)
      ipcRenderer.once('executeJavaScript-promise-response', (event, result) => {
        expect(result).to.equal(expected)
        done()
      })
    })
    it('resolves the returned promise with the result if the code returns an asyncronous promise', (done) => {
      ipcRenderer.send('executeJavaScript', asyncCode)
      ipcRenderer.once('executeJavaScript-promise-response', (event, result) => {
        expect(result).to.equal(expected)
        done()
      })
    })
    it('rejects the returned promise if an async error is thrown', (done) => {
      ipcRenderer.send('executeJavaScript', badAsyncCode)
      ipcRenderer.once('executeJavaScript-promise-error', (event, error) => {
        expect(error).to.equal(expectedErrorMsg)
        done()
      })
    })
    it('rejects the returned promise with an error if an Error.prototype is thrown', async () => {
      for (const error in errorTypes) {
        await new Promise((resolve) => {
          ipcRenderer.send('executeJavaScript', `Promise.reject(new ${error.name}("Wamp-wamp")`)
          ipcRenderer.once('executeJavaScript-promise-error-name', (event, name) => {
            expect(name).to.equal(error.name)
            resolve()
          })
        })
      }
    })

    it('works after page load and during subframe load', (done) => {
      w.webContents.once('did-finish-load', () => {
        // initiate a sub-frame load, then try and execute script during it
        w.webContents.executeJavaScript(`
          var iframe = document.createElement('iframe')
          iframe.src = '${server.url}/slow'
          document.body.appendChild(iframe)
        `).then(() => {
          w.webContents.executeJavaScript('console.log(\'hello\')').then(() => {
            done()
          })
        })
      })
      w.loadURL(server.url)
    })

    it('executes after page load', (done) => {
      w.webContents.executeJavaScript(code).then(result => {
        expect(result).to.equal(expected)
        done()
      })
      w.loadURL(server.url)
    })

    it('works with result objects that have DOM class prototypes', (done) => {
      w.webContents.executeJavaScript('document.location').then(result => {
        expect(result.origin).to.equal(server.url)
        expect(result.protocol).to.equal('http:')
        done()
      })
      w.loadURL(server.url)
    })
  })

  describe('previewFile', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('opens the path in Quick Look on macOS', () => {
      expect(() => {
        w.previewFile(__filename)
        w.closeFilePreview()
      }).to.not.throw()
    })
  })

  describe('contextIsolation option with and without sandbox option', () => {
    const expectedContextData = {
      preloadContext: {
        preloadProperty: 'number',
        pageProperty: 'undefined',
        typeofRequire: 'function',
        typeofProcess: 'object',
        typeofArrayPush: 'function',
        typeofFunctionApply: 'function',
        typeofPreloadExecuteJavaScriptProperty: 'undefined'
      },
      pageContext: {
        preloadProperty: 'undefined',
        pageProperty: 'string',
        typeofRequire: 'undefined',
        typeofProcess: 'undefined',
        typeofArrayPush: 'number',
        typeofFunctionApply: 'boolean',
        typeofPreloadExecuteJavaScriptProperty: 'number',
        typeofOpenedWindow: 'object'
      }
    }

    beforeEach(() => {
      if (iw != null) iw.destroy()
      iw = new BrowserWindow({
        show: false,
        webPreferences: {
          contextIsolation: true,
          preload: path.join(fixtures, 'api', 'isolated-preload.js')
        }
      })
      if (ws != null) ws.destroy()
      ws = new BrowserWindow({
        show: false,
        webPreferences: {
          sandbox: true,
          contextIsolation: true,
          preload: path.join(fixtures, 'api', 'isolated-preload.js')
        }
      })
    })

    afterEach(() => {
      if (iw != null) iw.destroy()
      if (ws != null) ws.destroy()
    })

    it('separates the page context from the Electron/preload context', async () => {
      const p = emittedOnce(ipcMain, 'isolated-world')
      iw.loadFile(path.join(fixtures, 'api', 'isolated.html'))
      const [, data] = await p
      expect(data).to.deep.equal(expectedContextData)
    })
    it('recreates the contexts on reload', async () => {
      await iw.loadFile(path.join(fixtures, 'api', 'isolated.html'))
      const isolatedWorld = emittedOnce(ipcMain, 'isolated-world')
      iw.webContents.reload()
      const [, data] = await isolatedWorld
      expect(data).to.deep.equal(expectedContextData)
    })
    it('enables context isolation on child windows', async () => {
      const browserWindowCreated = emittedOnce(app, 'browser-window-created')
      iw.loadFile(path.join(fixtures, 'pages', 'window-open.html'))
      const [, window] = await browserWindowCreated
      expect(window.webContents.getLastWebPreferences().contextIsolation).to.be.true()
    })
    it('separates the page context from the Electron/preload context with sandbox on', async () => {
      const p = emittedOnce(ipcMain, 'isolated-world')
      ws.loadFile(path.join(fixtures, 'api', 'isolated.html'))
      const [, data] = await p
      expect(data).to.deep.equal(expectedContextData)
    })
    it('recreates the contexts on reload with sandbox on', async () => {
      await ws.loadFile(path.join(fixtures, 'api', 'isolated.html'))
      const isolatedWorld = emittedOnce(ipcMain, 'isolated-world')
      ws.webContents.reload()
      const [, data] = await isolatedWorld
      expect(data).to.deep.equal(expectedContextData)
    })
    it('supports fetch api', async () => {
      const fetchWindow = new BrowserWindow({
        show: false,
        webPreferences: {
          contextIsolation: true,
          preload: path.join(fixtures, 'api', 'isolated-fetch-preload.js')
        }
      })
      const p = emittedOnce(ipcMain, 'isolated-fetch-error')
      fetchWindow.loadURL('about:blank')
      const [, error] = await p
      fetchWindow.destroy()
      expect(error).to.equal('Failed to fetch')
    })
    it('doesn\'t break ipc serialization', async () => {
      const p = emittedOnce(ipcMain, 'isolated-world')
      iw.loadURL('about:blank')
      iw.webContents.executeJavaScript(`
        const opened = window.open()
        openedLocation = opened.location.href
        opened.close()
        window.postMessage({openedLocation}, '*')
      `)
      const [, data] = await p
      expect(data.pageContext.openedLocation).to.equal('')
    })
  })

  describe('offscreen rendering', () => {
    beforeEach(function () {
      if (!features.isOffscreenRenderingEnabled()) {
        // XXX(alexeykuzmin): "afterEach" hook is not called
        // for skipped tests, we have to close the window manually.
        return closeTheWindow().then(() => { this.skip() })
      }

      if (w != null) w.destroy()
      w = new BrowserWindow({
        width: 100,
        height: 100,
        show: false,
        webPreferences: {
          backgroundThrottling: false,
          offscreen: true
        }
      })
    })

    it('creates offscreen window with correct size', (done) => {
      w.webContents.once('paint', function (event, rect, data) {
        expect(data.constructor.name).to.equal('NativeImage')
        expect(data.isEmpty()).to.be.false()
        const size = data.getSize()
        expect(size.width).to.be.closeTo(100 * devicePixelRatio, 2)
        expect(size.height).to.be.closeTo(100 * devicePixelRatio, 2)
        done()
      })
      w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'))
    })

    it('does not crash after navigation', () => {
      w.webContents.loadURL('about:blank')
      w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'))
    })

    describe('window.webContents.isOffscreen()', () => {
      it('is true for offscreen type', () => {
        w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'))
        expect(w.webContents.isOffscreen()).to.be.true()
      })

      it('is false for regular window', () => {
        const c = new BrowserWindow({ show: false })
        expect(c.webContents.isOffscreen()).to.be.false()
        c.destroy()
      })
    })

    describe('window.webContents.isPainting()', () => {
      it('returns whether is currently painting', (done) => {
        w.webContents.once('paint', function (event, rect, data) {
          expect(w.webContents.isPainting()).to.be.true()
          done()
        })
        w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'))
      })
    })

    describe('window.webContents.stopPainting()', () => {
      it('stops painting', (done) => {
        w.webContents.on('dom-ready', () => {
          w.webContents.stopPainting()
          expect(w.webContents.isPainting()).to.be.false()
          done()
        })
        w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'))
      })
    })

    describe('window.webContents.startPainting()', () => {
      it('starts painting', (done) => {
        w.webContents.on('dom-ready', () => {
          w.webContents.stopPainting()
          w.webContents.startPainting()
          w.webContents.once('paint', function (event, rect, data) {
            expect(w.webContents.isPainting()).to.be.true()
            done()
          })
        })
        w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'))
      })
    })

    // TODO(codebytere): remove in Electron v8.0.0
    describe('window.webContents.getFrameRate()', () => {
      it('has default frame rate', (done) => {
        w.webContents.once('paint', function (event, rect, data) {
          expect(w.webContents.getFrameRate()).to.equal(60)
          done()
        })
        w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'))
      })
    })

    // TODO(codebytere): remove in Electron v8.0.0
    describe('window.webContents.setFrameRate(frameRate)', () => {
      it('sets custom frame rate', (done) => {
        w.webContents.on('dom-ready', () => {
          w.webContents.setFrameRate(30)
          w.webContents.once('paint', function (event, rect, data) {
            expect(w.webContents.getFrameRate()).to.equal(30)
            done()
          })
        })
        w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'))
      })
    })

    describe('window.webContents.FrameRate', () => {
      it('has default frame rate', (done) => {
        w.webContents.once('paint', function (event, rect, data) {
          expect(w.webContents.frameRate).to.equal(60)
          done()
        })
        w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'))
      })

      it('sets custom frame rate', (done) => {
        w.webContents.on('dom-ready', () => {
          w.webContents.frameRate = 30
          w.webContents.once('paint', function (event, rect, data) {
            expect(w.webContents.frameRate).to.equal(30)
            done()
          })
        })
        w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'))
      })
    })
  })
})

const expectBoundsEqual = (actual, expected) => {
  if (!isScaleFactorRounding()) {
    expect(expected).to.deep.equal(actual)
  } else if (Array.isArray(actual)) {
    expect(actual[0]).to.be.closeTo(expected[0], 1)
    expect(actual[1]).to.be.closeTo(expected[1], 1)
  } else {
    expect(actual.x).to.be.closeTo(expected.x, 1)
    expect(actual.y).to.be.closeTo(expected.y, 1)
    expect(actual.width).to.be.closeTo(expected.width, 1)
    expect(actual.height).to.be.closeTo(expected.height, 1)
  }
}

// Is the display's scale factor possibly causing rounding of pixel coordinate
// values?
const isScaleFactorRounding = () => {
  const { scaleFactor } = screen.getPrimaryDisplay()
  // Return true if scale factor is non-integer value
  if (Math.round(scaleFactor) !== scaleFactor) return true
  // Return true if scale factor is odd number above 2
  return scaleFactor > 2 && scaleFactor % 2 === 1
}

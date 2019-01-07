'use strict'

const assert = require('assert')
const chai = require('chai')
const dirtyChai = require('dirty-chai')
const fs = require('fs')
const path = require('path')
const os = require('os')
const qs = require('querystring')
const http = require('http')
const { closeWindow } = require('./window-helpers')
const { emittedOnce } = require('./events-helpers')
const { resolveGetters } = require('./assert-helpers')
const { ipcRenderer, remote } = require('electron')
const { app, ipcMain, BrowserWindow, BrowserView, protocol, session, screen, webContents } = remote

const features = process.atomBinding('features')
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

  describe('BrowserWindow constructor', () => {
    it('allows passing void 0 as the webContents', async () => {
      await openTheWindow({
        webContents: void 0
      })
    })
  })

  describe('BrowserWindow.close()', () => {
    let server

    before((done) => {
      server = http.createServer((request, response) => {
        switch (request.url) {
          case '/404':
            response.statusCode = '404'
            response.end()
            break
          case '/301':
            response.statusCode = '301'
            response.setHeader('Location', '/200')
            response.end()
            break
          case '/200':
            response.statusCode = '200'
            response.end('hello')
            break
          case '/title':
            response.statusCode = '200'
            response.end('<title>Hello</title>')
            break
          default:
            done('unsupported endpoint')
        }
      }).listen(0, '127.0.0.1', () => {
        server.url = 'http://127.0.0.1:' + server.address().port
        done()
      })
    })

    after(() => {
      server.close()
      server = null
    })

    it('should emit unload handler', (done) => {
      w.webContents.once('did-finish-load', () => { w.close() })
      w.once('closed', () => {
        const test = path.join(fixtures, 'api', 'unload')
        const content = fs.readFileSync(test)
        fs.unlinkSync(test)
        assert.strictEqual(String(content), 'unload')
        done()
      })
      w.loadFile(path.join(fixtures, 'api', 'unload.html'))
    })
    it('should emit beforeunload handler', (done) => {
      w.once('onbeforeunload', () => { done() })
      w.webContents.once('did-finish-load', () => { w.close() })
      w.loadFile(path.join(fixtures, 'api', 'beforeunload-false.html'))
    })
    it('should not crash when invoked synchronously inside navigation observer', (done) => {
      const events = [
        { name: 'did-start-loading', url: `${server.url}/200` },
        { name: 'dom-ready', url: `${server.url}/200` },
        { name: 'page-title-updated', url: `${server.url}/title` },
        { name: 'did-stop-loading', url: `${server.url}/200` },
        { name: 'did-finish-load', url: `${server.url}/200` },
        { name: 'did-frame-finish-load', url: `${server.url}/200` },
        { name: 'did-fail-load', url: `${server.url}/404` }
      ]
      const responseEvent = 'window-webContents-destroyed'

      function * genNavigationEvent () {
        let eventOptions = null
        while ((eventOptions = events.shift()) && events.length) {
          const w = new BrowserWindow({ show: false })
          eventOptions.id = w.id
          eventOptions.responseEvent = responseEvent
          ipcRenderer.send('test-webcontents-navigation-observer', eventOptions)
          yield 1
        }
      }

      const gen = genNavigationEvent()
      ipcRenderer.on(responseEvent, () => {
        if (!gen.next().value) done()
      })
      gen.next()
    })
  })

  describe('window.close()', () => {
    it('should emit unload handler', (done) => {
      w.once('closed', () => {
        const test = path.join(fixtures, 'api', 'close')
        const content = fs.readFileSync(test)
        fs.unlinkSync(test)
        assert.strictEqual(String(content), 'close')
        done()
      })
      w.loadFile(path.join(fixtures, 'api', 'close.html'))
    })
    it('should emit beforeunload handler', (done) => {
      w.once('onbeforeunload', () => { done() })
      w.loadFile(path.join(fixtures, 'api', 'close-beforeunload-false.html'))
    })
  })

  describe('BrowserWindow.destroy()', () => {
    it('prevents users to access methods of webContents', () => {
      const contents = w.webContents
      w.destroy()
      assert.throws(() => {
        contents.getProcessId()
      }, /Object has been destroyed/)
    })
    it('should not crash when destroying windows with pending events', (done) => {
      const responseEvent = 'destroy-test-completed'
      ipcRenderer.on(responseEvent, () => done())
      ipcRenderer.send('test-browserwindow-destroy', { responseEvent })
    })
  })

  describe('BrowserWindow.loadURL(url)', () => {
    it('should emit did-start-loading event', (done) => {
      w.webContents.on('did-start-loading', () => { done() })
      w.loadURL('about:blank')
    })
    it('should emit ready-to-show event', (done) => {
      w.on('ready-to-show', () => { done() })
      w.loadURL('about:blank')
    })
    it('should emit did-fail-load event for files that do not exist', (done) => {
      w.webContents.on('did-fail-load', (event, code, desc, url, isMainFrame) => {
        assert.strictEqual(code, -6)
        assert.strictEqual(desc, 'ERR_FILE_NOT_FOUND')
        assert.strictEqual(isMainFrame, true)
        done()
      })
      w.loadURL('file://a.txt')
    })
    it('should emit did-fail-load event for invalid URL', (done) => {
      w.webContents.on('did-fail-load', (event, code, desc, url, isMainFrame) => {
        assert.strictEqual(desc, 'ERR_INVALID_URL')
        assert.strictEqual(code, -300)
        assert.strictEqual(isMainFrame, true)
        done()
      })
      w.loadURL('http://example:port')
    })
    it('should set `mainFrame = false` on did-fail-load events in iframes', (done) => {
      w.webContents.on('did-fail-load', (event, code, desc, url, isMainFrame) => {
        assert.strictEqual(isMainFrame, false)
        done()
      })
      w.loadFile(path.join(fixtures, 'api', 'did-fail-load-iframe.html'))
    })
    it('does not crash in did-fail-provisional-load handler', (done) => {
      w.webContents.once('did-fail-provisional-load', () => {
        w.loadURL('http://127.0.0.1:11111')
        done()
      })
      w.loadURL('http://127.0.0.1:11111')
    })
    it('should emit did-fail-load event for URL exceeding character limit', (done) => {
      w.webContents.on('did-fail-load', (event, code, desc, url, isMainFrame) => {
        assert.strictEqual(desc, 'ERR_INVALID_URL')
        assert.strictEqual(code, -300)
        assert.strictEqual(isMainFrame, true)
        done()
      })
      const data = Buffer.alloc(2 * 1024 * 1024).toString('base64')
      w.loadURL(`data:image/png;base64,${data}`)
    })

    describe('POST navigations', () => {
      afterEach(() => { w.webContents.session.webRequest.onBeforeSendHeaders(null) })

      it('supports specifying POST data', async () => {
        await w.loadURL(server.url, { postData: postData })
      })
      it('sets the content type header on URL encoded forms', async () => {
        await w.loadURL(server.url)
        const requestDetails = new Promise(resolve => {
          w.webContents.session.webRequest.onBeforeSendHeaders((details, callback) => {
            resolve(details)
          })
        })
        w.webContents.executeJavaScript(`
          form = document.createElement('form')
          document.body.appendChild(form)
          form.method = 'POST'
          form.target = '_blank'
          form.submit()
        `)
        const details = await requestDetails
        assert.strictEqual(details.requestHeaders['content-type'], 'application/x-www-form-urlencoded')
      })
      it('sets the content type header on multi part forms', async () => {
        await w.loadURL(server.url)
        const requestDetails = new Promise(resolve => {
          w.webContents.session.webRequest.onBeforeSendHeaders((details, callback) => {
            resolve(details)
          })
        })
        w.webContents.executeJavaScript(`
          form = document.createElement('form')
          document.body.appendChild(form)
          form.method = 'POST'
          form.target = '_blank'
          form.enctype = 'multipart/form-data'
          file = document.createElement('input')
          file.type = 'file'
          file.name = 'file'
          form.appendChild(file)
          form.submit()
        `)
        const details = await requestDetails
        assert(details.requestHeaders['content-type'].startsWith('multipart/form-data; boundary=----WebKitFormBoundary'))
      })
    })

    it('should support support base url for data urls', (done) => {
      ipcMain.once('answer', (event, test) => {
        assert.strictEqual(test, 'test')
        done()
      })
      w.loadURL('data:text/html,<script src="loaded-from-dataurl.js"></script>', { baseURLForDataURL: `file://${path.join(fixtures, 'api')}${path.sep}` })
    })
  })

  describe('will-navigate event', () => {
    it('allows the window to be closed from the event listener', (done) => {
      ipcRenderer.send('close-on-will-navigate', w.id)
      ipcRenderer.once('closed-on-will-navigate', () => { done() })
      w.loadFile(path.join(fixtures, 'pages', 'will-navigate.html'))
    })
  })

  describe('will-redirect event', () => {
    it('is emitted on redirects', (done) => {
      w.webContents.on('will-redirect', (event, url) => {
        done()
      })
      w.loadURL(`${server.url}/302`)
    })

    it('is emitted after will-navigate on redirects', (done) => {
      let navigateCalled = false
      w.webContents.on('will-navigate', () => {
        navigateCalled = true
      })
      w.webContents.on('will-redirect', (event, url) => {
        expect(navigateCalled).to.equal(true, 'should have called will-navigate first')
        done()
      })
      w.loadURL(`${server.url}/navigate-302`)
    })

    it('is emitted before did-stop-loading on redirects', (done) => {
      let stopCalled = false
      w.webContents.on('did-stop-loading', () => {
        stopCalled = true
      })
      w.webContents.on('will-redirect', (event, url) => {
        expect(stopCalled).to.equal(false, 'should not have called did-stop-loading first')
        done()
      })
      w.loadURL(`${server.url}/302`)
    })

    it('allows the window to be closed from the event listener', (done) => {
      ipcRenderer.send('close-on-will-redirect', w.id)
      ipcRenderer.once('closed-on-will-redirect', () => { done() })
      w.loadURL(`${server.url}/302`)
    })

    it('can be prevented', (done) => {
      ipcRenderer.send('prevent-will-redirect', w.id)
      w.webContents.on('will-navigate', (e, url) => {
        expect(url).to.equal(`${server.url}/302`)
      })
      w.webContents.on('did-stop-loading', () => {
        expect(w.webContents.getURL()).to.equal(
          `${server.url}/navigate-302`,
          'url should not have changed after navigation event'
        )
        done()
      })
      w.webContents.on('will-redirect', (e, url) => {
        expect(url).to.equal(`${server.url}/200`)
      })
      w.loadURL(`${server.url}/navigate-302`)
    })
  })

  describe('BrowserWindow.show()', () => {
    before(function () {
      if (isCI) {
        this.skip()
      }
    })

    it('should focus on window', () => {
      w.show()
      assert(w.isFocused())
    })
    it('should make the window visible', () => {
      w.show()
      assert(w.isVisible())
    })
    it('emits when window is shown', (done) => {
      w.once('show', () => {
        assert.strictEqual(w.isVisible(), true)
        done()
      })
      w.show()
    })
  })

  describe('BrowserWindow.hide()', () => {
    before(function () {
      if (isCI) {
        this.skip()
      }
    })

    it('should defocus on window', () => {
      w.hide()
      assert(!w.isFocused())
    })
    it('should make the window not visible', () => {
      w.show()
      w.hide()
      assert(!w.isVisible())
    })
    it('emits when window is hidden', (done) => {
      w.show()
      w.once('hide', () => {
        assert.strictEqual(w.isVisible(), false)
        done()
      })
      w.hide()
    })
  })

  describe('BrowserWindow.showInactive()', () => {
    it('should not focus on window', () => {
      w.showInactive()
      assert(!w.isFocused())
    })
  })

  describe('BrowserWindow.focus()', () => {
    it('does not make the window become visible', () => {
      assert.strictEqual(w.isVisible(), false)
      w.focus()
      assert.strictEqual(w.isVisible(), false)
    })
  })

  describe('BrowserWindow.blur()', () => {
    it('removes focus from window', () => {
      w.blur()
      assert(!w.isFocused())
    })
  })

  describe('BrowserWindow.getFocusedWindow()', (done) => {
    it('returns the opener window when dev tools window is focused', (done) => {
      w.show()
      w.webContents.once('devtools-focused', () => {
        assert.deepStrictEqual(BrowserWindow.getFocusedWindow(), w)
        done()
      })
      w.webContents.openDevTools({ mode: 'undocked' })
    })
  })

  describe('BrowserWindow.capturePage(rect)', () => {
    it('returns a Promise with a Buffer', async () => {
      const image = await w.capturePage({
        x: 0,
        y: 0,
        width: 100,
        height: 100
      })

      expect(image.isEmpty()).to.be.true()
    })

    it('preserves transparency', async () => {
      const w = await openTheWindow({
        show: false,
        width: 400,
        height: 400,
        transparent: true
      })
      const p = emittedOnce(w, 'ready-to-show')
      w.loadURL('data:text/html,<html><body background-color: rgba(255,255,255,0)></body></html>')
      await p
      w.show()

      const image = await w.capturePage()
      const imgBuffer = image.toPNG()

      // Check the 25th byte in the PNG.
      // Values can be 0,2,3,4, or 6. We want 6, which is RGB + Alpha
      expect(imgBuffer[25]).to.equal(6)
    })
  })

  describe('BrowserWindow.setBounds(bounds[, animate])', () => {
    it('sets the window bounds with full bounds', () => {
      const fullBounds = { x: 440, y: 225, width: 500, height: 400 }
      w.setBounds(fullBounds)

      assertBoundsEqual(w.getBounds(), fullBounds)
    })

    it('sets the window bounds with partial bounds', () => {
      const fullBounds = { x: 440, y: 225, width: 500, height: 400 }
      w.setBounds(fullBounds)

      const boundsUpdate = { width: 200 }
      w.setBounds(boundsUpdate)

      const expectedBounds = Object.assign(fullBounds, boundsUpdate)
      assertBoundsEqual(w.getBounds(), expectedBounds)
    })
  })

  describe('BrowserWindow.setSize(width, height)', () => {
    it('sets the window size', async () => {
      const size = [300, 400]

      const resized = emittedOnce(w, 'resize')
      w.setSize(size[0], size[1])
      await resized

      assertBoundsEqual(w.getSize(), size)
    })
  })

  describe('BrowserWindow.setMinimum/MaximumSize(width, height)', () => {
    it('sets the maximum and minimum size of the window', () => {
      assert.deepStrictEqual(w.getMinimumSize(), [0, 0])
      assert.deepStrictEqual(w.getMaximumSize(), [0, 0])

      w.setMinimumSize(100, 100)
      assertBoundsEqual(w.getMinimumSize(), [100, 100])
      assertBoundsEqual(w.getMaximumSize(), [0, 0])

      w.setMaximumSize(900, 600)
      assertBoundsEqual(w.getMinimumSize(), [100, 100])
      assertBoundsEqual(w.getMaximumSize(), [900, 600])
    })
  })

  describe('BrowserWindow.setAspectRatio(ratio)', () => {
    it('resets the behaviour when passing in 0', (done) => {
      const size = [300, 400]
      w.setAspectRatio(1 / 2)
      w.setAspectRatio(0)
      w.once('resize', () => {
        assertBoundsEqual(w.getSize(), size)
        done()
      })
      w.setSize(size[0], size[1])
    })
  })

  describe('BrowserWindow.setPosition(x, y)', () => {
    it('sets the window position', (done) => {
      const pos = [10, 10]
      w.once('move', () => {
        const newPos = w.getPosition()
        assert.strictEqual(newPos[0], pos[0])
        assert.strictEqual(newPos[1], pos[1])
        done()
      })
      w.setPosition(pos[0], pos[1])
    })
  })

  describe('BrowserWindow.setContentSize(width, height)', () => {
    it('sets the content size', () => {
      const size = [400, 400]
      w.setContentSize(size[0], size[1])
      const after = w.getContentSize()
      assert.strictEqual(after[0], size[0])
      assert.strictEqual(after[1], size[1])
    })
    it('works for a frameless window', () => {
      w.destroy()
      w = new BrowserWindow({
        show: false,
        frame: false,
        width: 400,
        height: 400
      })
      const size = [400, 400]
      w.setContentSize(size[0], size[1])
      const after = w.getContentSize()
      assert.strictEqual(after[0], size[0])
      assert.strictEqual(after[1], size[1])
    })
  })

  describe('BrowserWindow.setContentBounds(bounds)', () => {
    it('sets the content size and position', (done) => {
      const bounds = { x: 10, y: 10, width: 250, height: 250 }
      w.once('resize', () => {
        assertBoundsEqual(w.getContentBounds(), bounds)
        done()
      })
      w.setContentBounds(bounds)
    })
    it('works for a frameless window', (done) => {
      w.destroy()
      w = new BrowserWindow({
        show: false,
        frame: false,
        width: 300,
        height: 300
      })
      const bounds = { x: 10, y: 10, width: 250, height: 250 }
      w.once('resize', () => {
        assert.deepStrictEqual(w.getContentBounds(), bounds)
        done()
      })
      w.setContentBounds(bounds)
    })
  })

  describe(`BrowserWindow.getNormalBounds()`, () => {
    describe(`Normal state`, () => {
      it(`checks normal bounds after resize`, (done) => {
        const size = [300, 400]
        w.once('resize', () => {
          assertBoundsEqual(w.getNormalBounds(), w.getBounds())
          done()
        })
        w.setSize(size[0], size[1])
      })
      it(`checks normal bounds after move`, (done) => {
        const pos = [10, 10]
        w.once('move', () => {
          assertBoundsEqual(w.getNormalBounds(), w.getBounds())
          done()
        })
        w.setPosition(pos[0], pos[1])
      })
    })
    describe(`Maximized state`, () => {
      before(function () {
        if (isCI) {
          this.skip()
        }
      })
      it(`checks normal bounds when maximized`, (done) => {
        const bounds = w.getBounds()
        w.once('maximize', () => {
          assertBoundsEqual(w.getNormalBounds(), bounds)
          done()
        })
        w.show()
        w.maximize()
      })
      it(`checks normal bounds when unmaximized`, (done) => {
        const bounds = w.getBounds()
        w.once('maximize', () => {
          w.unmaximize()
        })
        w.once('unmaximize', () => {
          assertBoundsEqual(w.getNormalBounds(), bounds)
          done()
        })
        w.show()
        w.maximize()
      })
    })
    describe(`Minimized state`, () => {
      before(function () {
        if (isCI) {
          this.skip()
        }
      })
      it(`checks normal bounds when minimized`, (done) => {
        const bounds = w.getBounds()
        w.once('minimize', () => {
          assertBoundsEqual(w.getNormalBounds(), bounds)
          done()
        })
        w.show()
        w.minimize()
      })
      it(`checks normal bounds when restored`, (done) => {
        const bounds = w.getBounds()
        w.once('minimize', () => {
          w.restore()
        })
        w.once('restore', () => {
          assertBoundsEqual(w.getNormalBounds(), bounds)
          done()
        })
        w.show()
        w.minimize()
      })
    })
    describe(`Fullscreen state`, () => {
      before(function () {
        if (isCI) {
          this.skip()
        }
        if (process.platform === 'darwin') {
          this.skip()
        }
      })
      it(`checks normal bounds when fullscreen'ed`, (done) => {
        const bounds = w.getBounds()
        w.once('enter-full-screen', () => {
          assertBoundsEqual(w.getNormalBounds(), bounds)
          done()
        })
        w.show()
        w.setFullScreen(true)
      })
      it(`checks normal bounds when unfullscreen'ed`, (done) => {
        const bounds = w.getBounds()
        w.once('enter-full-screen', () => {
          w.setFullScreen(false)
        })
        w.once('leave-full-screen', () => {
          assertBoundsEqual(w.getNormalBounds(), bounds)
          done()
        })
        w.show()
        w.setFullScreen(true)
      })
    })
  })

  describe('BrowserWindow.setProgressBar(progress)', () => {
    it('sets the progress', () => {
      assert.doesNotThrow(() => {
        if (process.platform === 'darwin') {
          app.dock.setIcon(path.join(fixtures, 'assets', 'logo.png'))
        }
        w.setProgressBar(0.5)

        if (process.platform === 'darwin') {
          app.dock.setIcon(null)
        }
        w.setProgressBar(-1)
      })
    })
    it('sets the progress using "paused" mode', () => {
      assert.doesNotThrow(() => {
        w.setProgressBar(0.5, { mode: 'paused' })
      })
    })
    it('sets the progress using "error" mode', () => {
      assert.doesNotThrow(() => {
        w.setProgressBar(0.5, { mode: 'error' })
      })
    })
    it('sets the progress using "normal" mode', () => {
      assert.doesNotThrow(() => {
        w.setProgressBar(0.5, { mode: 'normal' })
      })
    })
  })

  describe('BrowserWindow.setAlwaysOnTop(flag, level)', () => {
    it('sets the window as always on top', () => {
      assert.strictEqual(w.isAlwaysOnTop(), false)
      w.setAlwaysOnTop(true, 'screen-saver')
      assert.strictEqual(w.isAlwaysOnTop(), true)
      w.setAlwaysOnTop(false)
      assert.strictEqual(w.isAlwaysOnTop(), false)
      w.setAlwaysOnTop(true)
      assert.strictEqual(w.isAlwaysOnTop(), true)
    })
    it('raises an error when relativeLevel is out of bounds', function () {
      if (process.platform !== 'darwin') {
        // FIXME(alexeykuzmin): Skip the test instead of marking it as passed.
        // afterEach hook won't be run if a test is skipped dynamically.
        // If afterEach isn't run current window won't be destroyed
        // and the next test will fail on assertion in `closeWindow()`.
        // this.skip()
        return
      }

      assert.throws(() => {
        w.setAlwaysOnTop(true, '', -2147483644)
      })

      assert.throws(() => {
        w.setAlwaysOnTop(true, '', 2147483632)
      })
    })
  })

  describe('BrowserWindow.alwaysOnTop() resets level on minimize', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('resets the windows level on minimize', () => {
      assert.strictEqual(w.isAlwaysOnTop(), false)
      w.setAlwaysOnTop(true, 'screen-saver')
      assert.strictEqual(w.isAlwaysOnTop(), true)
      w.minimize()
      assert.strictEqual(w.isAlwaysOnTop(), false)
      w.restore()
      assert.strictEqual(w.isAlwaysOnTop(), true)
    })
  })

  describe('BrowserWindow.setAutoHideCursor(autoHide)', () => {
    describe('on macOS', () => {
      before(function () {
        if (process.platform !== 'darwin') {
          this.skip()
        }
      })

      it('allows changing cursor auto-hiding', () => {
        assert.doesNotThrow(() => {
          w.setAutoHideCursor(false)
          w.setAutoHideCursor(true)
        })
      })
    })

    describe('on non-macOS platforms', () => {
      before(function () {
        if (process.platform === 'darwin') {
          this.skip()
        }
      })

      it('is not available', () => {
        assert.ok(!w.setAutoHideCursor)
      })
    })
  })

  describe('BrowserWindow.selectPreviousTab()', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('does not throw', () => {
      assert.doesNotThrow(() => {
        w.selectPreviousTab()
      })
    })
  })

  describe('BrowserWindow.selectNextTab()', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('does not throw', () => {
      assert.doesNotThrow(() => {
        w.selectNextTab()
      })
    })
  })

  describe('BrowserWindow.mergeAllWindows()', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('does not throw', () => {
      assert.doesNotThrow(() => {
        w.mergeAllWindows()
      })
    })
  })

  describe('BrowserWindow.moveTabToNewWindow()', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('does not throw', () => {
      assert.doesNotThrow(() => {
        w.moveTabToNewWindow()
      })
    })
  })

  describe('BrowserWindow.toggleTabBar()', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('does not throw', () => {
      assert.doesNotThrow(() => {
        w.toggleTabBar()
      })
    })
  })

  describe('BrowserWindow.addTabbedWindow()', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('does not throw', (done) => {
      const tabbedWindow = new BrowserWindow({})
      assert.doesNotThrow(() => {
        w.addTabbedWindow(tabbedWindow)
      })

      assert.strictEqual(BrowserWindow.getAllWindows().length, 3) // Test window + w + tabbedWindow

      closeWindow(tabbedWindow, { assertSingleWindow: false }).then(() => {
        assert.strictEqual(BrowserWindow.getAllWindows().length, 2) // Test window + w
        done()
      })
    })

    it('throws when called on itself', () => {
      assert.throws(() => {
        w.addTabbedWindow(w)
      }, /AddTabbedWindow cannot be called by a window on itself./)
    })
  })

  describe('BrowserWindow.setWindowButtonVisibility()', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('does not throw', () => {
      assert.doesNotThrow(() => {
        w.setWindowButtonVisibility(true)
        w.setWindowButtonVisibility(false)
      })
    })

    it('throws with custom title bar buttons', () => {
      assert.throws(() => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          titleBarStyle: 'customButtonsOnHover',
          frame: false
        })
        w.setWindowButtonVisibility(true)
      }, /Not supported for this window/)
    })
  })

  describe('BrowserWindow.setVibrancy(type)', () => {
    it('allows setting, changing, and removing the vibrancy', () => {
      assert.doesNotThrow(() => {
        w.setVibrancy('light')
        w.setVibrancy('dark')
        w.setVibrancy(null)
        w.setVibrancy('ultra-dark')
        w.setVibrancy('')
      })
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

      assert.doesNotThrow(() => {
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
      })

      assert.throws(() => {
        w.setAppDetails()
      }, /Insufficient number of arguments\./)
    })
  })

  describe('BrowserWindow.fromId(id)', () => {
    it('returns the window with id', () => {
      assert.strictEqual(w.id, BrowserWindow.fromId(w.id).id)
    })
  })

  describe('BrowserWindow.fromWebContents(webContents)', () => {
    let contents = null

    beforeEach(() => { contents = webContents.create({}) })

    afterEach(() => { contents.destroy() })

    it('returns the window with the webContents', () => {
      assert.strictEqual(BrowserWindow.fromWebContents(w.webContents).id, w.id)
      assert.strictEqual(BrowserWindow.fromWebContents(contents), undefined)
    })
  })

  describe('BrowserWindow.fromDevToolsWebContents(webContents)', () => {
    let contents = null

    beforeEach(() => { contents = webContents.create({}) })

    afterEach(() => { contents.destroy() })

    it('returns the window with the webContents', (done) => {
      w.webContents.once('devtools-opened', () => {
        assert.strictEqual(BrowserWindow.fromDevToolsWebContents(w.devToolsWebContents).id, w.id)
        assert.strictEqual(BrowserWindow.fromDevToolsWebContents(w.webContents), undefined)
        assert.strictEqual(BrowserWindow.fromDevToolsWebContents(contents), undefined)
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
      assert.strictEqual(BrowserWindow.fromBrowserView(bv).id, w.id)
    })

    it('returns undefined if not attached', () => {
      w.setBrowserView(null)
      assert.strictEqual(BrowserWindow.fromBrowserView(bv), null)
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
      assert.strictEqual(w.getOpacity(), 0.5)
    })
    it('allows setting the opacity', () => {
      assert.doesNotThrow(() => {
        w.setOpacity(0.0)
        assert.strictEqual(w.getOpacity(), 0.0)
        w.setOpacity(0.5)
        assert.strictEqual(w.getOpacity(), 0.5)
        w.setOpacity(1.0)
        assert.strictEqual(w.getOpacity(), 1.0)
      })
    })
  })

  describe('BrowserWindow.setShape(rects)', () => {
    it('allows setting shape', () => {
      assert.doesNotThrow(() => {
        w.setShape([])
        w.setShape([{ x: 0, y: 0, width: 100, height: 100 }])
        w.setShape([{ x: 0, y: 0, width: 100, height: 100 }, { x: 0, y: 200, width: 1000, height: 100 }])
        w.setShape([])
      })
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
      assert.strictEqual(contentSize[0], 400)
      assert.strictEqual(contentSize[1], 400)
    })
    it('make window created with window size when not used', () => {
      const size = w.getSize()
      assert.strictEqual(size[0], 400)
      assert.strictEqual(size[1], 400)
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
      assert.strictEqual(contentSize[0], 400)
      assert.strictEqual(contentSize[1], 400)
      const size = w.getSize()
      assert.strictEqual(size[0], 400)
      assert.strictEqual(size[1], 400)
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
      assert.strictEqual(contentSize[1], 400)
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
      assert.strictEqual(contentSize[1], 400)
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
      assert.strictEqual(after[0], -10)
      assert.strictEqual(after[1], -10)
    })
    it('can set the window larger than screen', () => {
      const size = screen.getPrimaryDisplay().size
      size.width += 100
      size.height += 100
      w.setSize(size.width, size.height)
      assertBoundsEqual(w.getSize(), [size.width, size.height])
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
      assert.strictEqual(w.getSize()[0], 500)
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
        assert.deepStrictEqual(defaultSession.getPreloads(), [])
        defaultSession.setPreloads(preloads)
      })
      afterEach(() => {
        defaultSession.setPreloads([])
      })

      it('can set multiple session preload script', function () {
        assert.deepStrictEqual(defaultSession.getPreloads(), preloads)
      })

      it('loads the script before other scripts in window including normal preloads', function (done) {
        ipcMain.once('vars', function (event, preload1, preload2, preload3) {
          assert.strictEqual(preload1, 'preload-1')
          assert.strictEqual(preload2, 'preload-1-2')
          assert.strictEqual(preload3, 'preload-1-2-3')
          done()
        })
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            preload: path.join(fixtures, 'module', 'set-global-preload-3.js')
          }
        })
        w.loadFile(path.join(fixtures, 'api', 'preloads.html'))
      })
    })

    describe('"additionalArguments" option', () => {
      it('adds extra args to process.argv in the renderer process', (done) => {
        const preload = path.join(fixtures, 'module', 'check-arguments.js')
        ipcMain.once('answer', (event, argv) => {
          assert.ok(argv.includes('--my-magic-arg'))
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
          assert.ok(argv.includes('--my-magic-arg=foo'))
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
          assert.strictEqual(typeofProcess, 'undefined')
          assert.strictEqual(typeofBuffer, 'undefined')
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
          assert.strictEqual(test, 'preload')
          done()
        })
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            sandbox: true,
            preload
          }
        })
        w.loadFile(path.join(fixtures, 'api', 'preload.html'))
      })

      it('exposes ipcRenderer to preload script (path has special chars)', function (done) {
        const preloadSpecialChars = path.join(fixtures, 'module', 'preload-sandboxæø åü.js')
        ipcMain.once('answer', function (event, test) {
          assert.strictEqual(test, 'preload')
          done()
        })
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            sandbox: true,
            preload: preloadSpecialChars
          }
        })
        w.loadFile(path.join(fixtures, 'api', 'preload.html'))
      })

      it('exposes "exit" event to preload script', function (done) {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
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
          assert.strictEqual(url, expectedUrl)
          done()
        })
        w.loadURL(pageUrl)
      })

      it('should open windows in same domain with cross-scripting enabled', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
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
          assert.strictEqual(url, expectedUrl)
          assert.strictEqual(frameName, 'popup!')
          assert.strictEqual(options.width, 500)
          assert.strictEqual(options.height, 600)
          ipcMain.once('answer', function (event, html) {
            assert.strictEqual(html, '<h1>scripting from opener</h1>')
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
          assert.strictEqual(args.includes('--enable-sandbox'), true)
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
          assert.strictEqual(webPreferences.foo, 'bar')
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
          assert.notStrictEqual(w.webContents, childWc)
        })
        ipcMain.once('parent-ready', function (event) {
          assert.strictEqual(w.webContents, event.sender)
          event.sender.send('verified')
        })
        ipcMain.once('child-ready', function (event) {
          assert(childWc)
          assert.strictEqual(childWc, event.sender)
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
            assert.deepStrictEqual(currentWebContents, initialWebContents)
            done()
          }, 100)
        })
        w.loadFile(path.join(fixtures, 'pages', 'window-open.html'))
      })

      // TODO(alexeykuzmin): `GetProcessMemoryInfo()` is not available starting Ch67.
      xit('releases memory after popup is closed', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            preload,
            sandbox: true
          }
        })
        w.loadFile(path.join(fixtures, 'api', 'sandbox.html'), { search: 'allocate-memory' })
        ipcMain.once('answer', function (event, { bytesBeforeOpen, bytesAfterOpen, bytesAfterClose }) {
          const memoryIncreaseByOpen = bytesAfterOpen - bytesBeforeOpen
          const memoryDecreaseByClose = bytesAfterOpen - bytesAfterClose
          // decreased memory should be less than increased due to factors we
          // can't control, but given the amount of memory allocated in the
          // fixture, we can reasonably expect decrease to be at least 70% of
          // increase
          assert(memoryDecreaseByClose > memoryIncreaseByOpen * 0.7)
          done()
        })
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
          assert.strictEqual(arg, 'hi')
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
          assert.strictEqual(arg, 'hi child window')
          done()
        })
      })

      it('validates process APIs access in sandboxed renderer', (done) => {
        ipcMain.once('answer', function (event, test) {
          assert.strictEqual(test.pid, w.webContents.getOSProcessId())
          assert.strictEqual(test.arch, remote.process.arch)
          assert.strictEqual(test.platform, remote.process.platform)
          assert.deepStrictEqual(...resolveGetters(test.env, remote.process.env))
          assert.strictEqual(test.execPath, remote.process.helperExecPath)
          assert.strictEqual(test.sandboxed, true)
          assert.strictEqual(test.type, 'renderer')
          assert.strictEqual(test.version, remote.process.version)
          assert.deepStrictEqual(test.versions, remote.process.versions)
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
      beforeEach(() => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            nativeWindowOpen: true
          }
        })
      })

      it('opens window of about:blank with cross-scripting enabled', (done) => {
        ipcMain.once('answer', (event, content) => {
          assert.strictEqual(content, 'Hello')
          done()
        })
        w.loadFile(path.join(fixtures, 'api', 'native-window-open-blank.html'))
      })
      it('opens window of same domain with cross-scripting enabled', (done) => {
        ipcMain.once('answer', (event, content) => {
          assert.strictEqual(content, 'Hello')
          done()
        })
        w.loadFile(path.join(fixtures, 'api', 'native-window-open-file.html'))
      })
      it('blocks accessing cross-origin frames', (done) => {
        ipcMain.once('answer', (event, content) => {
          assert.strictEqual(content, 'Blocked a frame with origin "file://" from accessing a cross-origin frame.')
          done()
        })
        w.loadFile(path.join(fixtures, 'api', 'native-window-open-cross-origin.html'))
      })
      it('opens window from <iframe> tags', (done) => {
        ipcMain.once('answer', (event, content) => {
          assert.strictEqual(content, 'Hello')
          done()
        })
        w.loadFile(path.join(fixtures, 'api', 'native-window-open-iframe.html'))
      });
      (nativeModulesEnabled ? it : it.skip)('loads native addons correctly after reload', (done) => {
        ipcMain.once('answer', (event, content) => {
          assert.strictEqual(content, 'function')
          ipcMain.once('answer', (event, content) => {
            assert.strictEqual(content, 'function')
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
            nativeWindowOpen: true
          }
        })

        const preloadPath = path.join(fixtures, 'api', 'new-window-preload.js')
        ipcRenderer.send('set-web-preferences-on-next-new-window', w.webContents.id, 'preload', preloadPath)
        ipcMain.once('answer', (event, args) => {
          assert.strictEqual(args.includes('--native-window-open'), true)
          done()
        })
        w.loadFile(path.join(fixtures, 'api', 'new-window.html'))
      })
      it('should open windows with the options configured via new-window event listeners', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            nativeWindowOpen: true
          }
        })

        const preloadPath = path.join(fixtures, 'api', 'new-window-preload.js')
        ipcRenderer.send('set-web-preferences-on-next-new-window', w.webContents.id, 'preload', preloadPath)
        ipcRenderer.send('set-web-preferences-on-next-new-window', w.webContents.id, 'foo', 'bar')
        ipcMain.once('answer', (event, args, webPreferences) => {
          assert.strictEqual(webPreferences.foo, 'bar')
          done()
        })
        w.loadFile(path.join(fixtures, 'api', 'new-window.html'))
      })
      it('retains the original web preferences when window.location is changed to a new origin', async () => {
        await serveFileFromProtocol('foo', path.join(fixtures, 'api', 'window-open-location-change.html'))
        await serveFileFromProtocol('bar', path.join(fixtures, 'api', 'window-open-location-final.html'))

        w.destroy()
        w = new BrowserWindow({
          show: true,
          webPreferences: {
            nativeWindowOpen: true
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

      it('should have nodeIntegration disabled in child windows', async () => {
        const p = emittedOnce(ipcMain, 'answer')
        w.loadFile(path.join(fixtures, 'api', 'native-window-open-argv.html'))
        const [, typeofProcess] = await p
        expect(typeofProcess).to.eql('undefined')
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
        assert.strictEqual(content, 'Hello')
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
          assert.fail('Reload was not prevented')
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
          assert.fail('Navigation was not prevented')
        })
        w.loadURL('about:blank')
      })
      w.loadFile(path.join(fixtures, 'api', 'beforeunload-false-prevent3.html'))
    })
  })

  describe('document.visibilityState/hidden', () => {
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
        assert.strictEqual(readyToShow, false)
        assert.strictEqual(visibilityState, 'visible')
        assert.strictEqual(hidden, false)

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
        assert.strictEqual(visibilityState, 'visible')
        assert.strictEqual(hidden, false)

        onNextVisibilityChange((visibilityState, hidden) => {
          assert.strictEqual(visibilityState, 'hidden')
          assert.strictEqual(hidden, true)
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
            assert.strictEqual(visibilityState, 'visible')
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
            assert.strictEqual(visibilityState, 'visible')
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
        assert.strictEqual(visibilityState, 'visible')
        assert.strictEqual(hidden, false)

        onNextVisibilityChange((visibilityState, hidden) => {
          assert.strictEqual(visibilityState, 'hidden')
          assert.strictEqual(hidden, true)
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
        assert.strictEqual(visibilityState, 'visible')
        assert.strictEqual(hidden, false)

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
        assert.strictEqual(url, 'http://host/')
        assert.strictEqual(frameName, 'host')
        assert.strictEqual(additionalFeatures[0], 'this-is-not-a-standard-feature')
        done()
      })
      w.loadFile(path.join(fixtures, 'pages', 'window-open.html'))
    })
    it('emits when window.open is called with no webPreferences', (done) => {
      w.destroy()
      w = new BrowserWindow({ show: false })
      w.webContents.once('new-window', function (e, url, frameName, disposition, options, additionalFeatures) {
        e.preventDefault()
        assert.strictEqual(url, 'http://host/')
        assert.strictEqual(frameName, 'host')
        assert.strictEqual(additionalFeatures[0], 'this-is-not-a-standard-feature')
        done()
      })
      w.loadFile(path.join(fixtures, 'pages', 'window-open.html'))
    })

    it('emits when link with target is called', (done) => {
      w.webContents.once('new-window', (e, url, frameName) => {
        e.preventDefault()
        assert.strictEqual(url, 'http://host/')
        assert.strictEqual(frameName, 'target')
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
    before(function () {
      // This test is too slow, only test it on CI.
      if (!isCI) {
        this.skip()
      }

      // FIXME These specs crash on Linux when run in a docker container
      if (isCI && process.platform === 'linux') {
        this.skip()
      }
    })

    it('subscribes to frame updates', (done) => {
      let called = false
      w.loadFile(path.join(fixtures, 'api', 'frame-subscriber.html'))
      w.webContents.on('dom-ready', () => {
        w.webContents.beginFrameSubscription(function (data) {
          // This callback might be called twice.
          if (called) return
          called = true

          assert.notStrictEqual(data.length, 0)
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
        w.webContents.beginFrameSubscription(true, (data, rect) => {
          if (data.length === 0) {
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

          expect(data.length).to.equal(rect.width * rect.height * 4)
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
      const error = await new Promise(resolve => {
        w.webContents.savePage(savePageHtmlPath, 'HTMLComplete', function (error) {
          resolve(error)
        })
      })
      expect(error).to.be.null()
      assert(fs.existsSync(savePageHtmlPath))
      assert(fs.existsSync(savePageJsPath))
      assert(fs.existsSync(savePageCssPath))
    })
  })

  describe('BrowserWindow options argument is optional', () => {
    it('should create a window with default size (800x600)', () => {
      w.destroy()
      w = new BrowserWindow()
      const size = w.getSize()
      assert.strictEqual(size[0], 800)
      assert.strictEqual(size[1], 600)
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

      w.setMinimizable(false)
      w.setMinimizable(true)
      assert.deepStrictEqual(w.getSize(), [300, 200])

      w.setResizable(false)
      w.setResizable(true)
      assert.deepStrictEqual(w.getSize(), [300, 200])

      w.setMaximizable(false)
      w.setMaximizable(true)
      assert.deepStrictEqual(w.getSize(), [300, 200])

      w.setFullScreenable(false)
      w.setFullScreenable(true)
      assert.deepStrictEqual(w.getSize(), [300, 200])

      w.setClosable(false)
      w.setClosable(true)
      assert.deepStrictEqual(w.getSize(), [300, 200])
    })

    describe('resizable state', () => {
      it('can be changed with resizable option', () => {
        w.destroy()
        w = new BrowserWindow({ show: false, resizable: false })
        assert.strictEqual(w.isResizable(), false)

        if (process.platform === 'darwin') {
          assert.strictEqual(w.isMaximizable(), true)
        }
      })

      it('can be changed with setResizable method', () => {
        assert.strictEqual(w.isResizable(), true)
        w.setResizable(false)
        assert.strictEqual(w.isResizable(), false)
        w.setResizable(true)
        assert.strictEqual(w.isResizable(), true)
      })

      it('works for a frameless window', () => {
        w.destroy()
        w = new BrowserWindow({ show: false, frame: false })
        assert.strictEqual(w.isResizable(), true)

        if (process.platform === 'win32') {
          w.destroy()
          w = new BrowserWindow({ show: false, thickFrame: false })
          assert.strictEqual(w.isResizable(), false)
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
          assertBoundsEqual(w.getContentSize(), [60, 60])
          w.setContentSize(30, 30)
          assertBoundsEqual(w.getContentSize(), [30, 30])
          w.setContentSize(10, 10)
          assertBoundsEqual(w.getContentSize(), [10, 10])
        })
      }
    })

    describe('loading main frame state', () => {
      it('is true when the main frame is loading', (done) => {
        w.webContents.on('did-start-loading', () => {
          assert.strictEqual(w.webContents.isLoadingMainFrame(), true)
          done()
        })
        w.webContents.loadURL(server.url)
      })
      it('is false when only a subframe is loading', (done) => {
        w.webContents.once('did-finish-load', () => {
          assert.strictEqual(w.webContents.isLoadingMainFrame(), false)
          w.webContents.on('did-start-loading', () => {
            assert.strictEqual(w.webContents.isLoadingMainFrame(), false)
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
          assert.strictEqual(w.webContents.isLoadingMainFrame(), false)
          w.webContents.on('did-start-loading', () => {
            assert.strictEqual(w.webContents.isLoadingMainFrame(), true)
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

    describe('movable state', () => {
      it('can be changed with movable option', () => {
        w.destroy()
        w = new BrowserWindow({ show: false, movable: false })
        assert.strictEqual(w.isMovable(), false)
      })
      it('can be changed with setMovable method', () => {
        assert.strictEqual(w.isMovable(), true)
        w.setMovable(false)
        assert.strictEqual(w.isMovable(), false)
        w.setMovable(true)
        assert.strictEqual(w.isMovable(), true)
      })
    })

    describe('minimizable state', () => {
      it('can be changed with minimizable option', () => {
        w.destroy()
        w = new BrowserWindow({ show: false, minimizable: false })
        assert.strictEqual(w.isMinimizable(), false)
      })

      it('can be changed with setMinimizable method', () => {
        assert.strictEqual(w.isMinimizable(), true)
        w.setMinimizable(false)
        assert.strictEqual(w.isMinimizable(), false)
        w.setMinimizable(true)
        assert.strictEqual(w.isMinimizable(), true)
      })
    })

    describe('maximizable state', () => {
      it('can be changed with maximizable option', () => {
        w.destroy()
        w = new BrowserWindow({ show: false, maximizable: false })
        assert.strictEqual(w.isMaximizable(), false)
      })

      it('can be changed with setMaximizable method', () => {
        assert.strictEqual(w.isMaximizable(), true)
        w.setMaximizable(false)
        assert.strictEqual(w.isMaximizable(), false)
        w.setMaximizable(true)
        assert.strictEqual(w.isMaximizable(), true)
      })

      it('is not affected when changing other states', () => {
        w.setMaximizable(false)
        assert.strictEqual(w.isMaximizable(), false)
        w.setMinimizable(false)
        assert.strictEqual(w.isMaximizable(), false)
        w.setClosable(false)
        assert.strictEqual(w.isMaximizable(), false)

        w.setMaximizable(true)
        assert.strictEqual(w.isMaximizable(), true)
        w.setClosable(true)
        assert.strictEqual(w.isMaximizable(), true)
        w.setFullScreenable(false)
        assert.strictEqual(w.isMaximizable(), true)
      })
    })

    describe('maximizable state (Windows only)', () => {
      // Only implemented on windows.
      if (process.platform !== 'win32') return

      it('is reset to its former state', () => {
        w.setMaximizable(false)
        w.setResizable(false)
        w.setResizable(true)
        assert.strictEqual(w.isMaximizable(), false)
        w.setMaximizable(true)
        w.setResizable(false)
        w.setResizable(true)
        assert.strictEqual(w.isMaximizable(), true)
      })
    })

    describe('fullscreenable state', () => {
      before(function () {
        // Only implemented on macOS.
        if (process.platform !== 'darwin') {
          this.skip()
        }
      })

      it('can be changed with fullscreenable option', () => {
        w.destroy()
        w = new BrowserWindow({ show: false, fullscreenable: false })
        assert.strictEqual(w.isFullScreenable(), false)
      })

      it('can be changed with setFullScreenable method', () => {
        assert.strictEqual(w.isFullScreenable(), true)
        w.setFullScreenable(false)
        assert.strictEqual(w.isFullScreenable(), false)
        w.setFullScreenable(true)
        assert.strictEqual(w.isFullScreenable(), true)
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
        assert.strictEqual(w.isKiosk(), true)

        w.once('enter-full-screen', () => {
          w.setKiosk(false)
          assert.strictEqual(w.isKiosk(), false)
        })
        w.once('leave-full-screen', () => {
          done()
        })
      })
    })

    describe('fullscreen state with resizable set', () => {
      before(function () {
        // Only implemented on macOS.
        if (process.platform !== 'darwin') {
          this.skip()
        }
      })

      it('resizable flag should be set to true and restored', (done) => {
        w.destroy()
        w = new BrowserWindow({ resizable: false })
        w.once('enter-full-screen', () => {
          assert.strictEqual(w.isResizable(), true)
          w.setFullScreen(false)
        })
        w.once('leave-full-screen', () => {
          assert.strictEqual(w.isResizable(), false)
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
          assert.strictEqual(w.isFullScreen(), true)
          w.setFullScreen(false)
        })
        w.once('leave-full-screen', () => {
          assert.strictEqual(w.isFullScreen(), false)
          done()
        })
        w.setFullScreen(true)
      })

      it('should not be changed by setKiosk method', (done) => {
        w.destroy()
        w = new BrowserWindow()
        w.once('enter-full-screen', () => {
          assert.strictEqual(w.isFullScreen(), true)
          w.setKiosk(true)
          w.setKiosk(false)
          assert.strictEqual(w.isFullScreen(), true)
          w.setFullScreen(false)
        })
        w.once('leave-full-screen', () => {
          assert.strictEqual(w.isFullScreen(), false)
          done()
        })
        w.setFullScreen(true)
      })
    })

    describe('closable state', () => {
      it('can be changed with closable option', () => {
        w.destroy()
        w = new BrowserWindow({ show: false, closable: false })
        assert.strictEqual(w.isClosable(), false)
      })

      it('can be changed with setClosable method', () => {
        assert.strictEqual(w.isClosable(), true)
        w.setClosable(false)
        assert.strictEqual(w.isClosable(), false)
        w.setClosable(true)
        assert.strictEqual(w.isClosable(), true)
      })
    })

    describe('hasShadow state', () => {
      // On Window there is no shadow by default and it can not be changed
      // dynamically.
      it('can be changed with hasShadow option', () => {
        w.destroy()
        const hasShadow = process.platform !== 'darwin'
        w = new BrowserWindow({ show: false, hasShadow: hasShadow })
        assert.strictEqual(w.hasShadow(), hasShadow)
      })

      it('can be changed with setHasShadow method', () => {
        if (process.platform !== 'darwin') return

        assert.strictEqual(w.hasShadow(), true)
        w.setHasShadow(false)
        assert.strictEqual(w.hasShadow(), false)
        w.setHasShadow(true)
        assert.strictEqual(w.hasShadow(), true)
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
      assertBoundsEqual(w.getSize(), initialSize)
    })
  })

  describe('BrowserWindow.unmaximize()', () => {
    it('should restore the previous window position', () => {
      if (w != null) w.destroy()
      w = new BrowserWindow()

      const initialPosition = w.getPosition()
      w.maximize()
      w.unmaximize()
      assertBoundsEqual(w.getPosition(), initialPosition)
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
          assert.strictEqual(w.isVisible(), true)
          assert.strictEqual(w.isFullScreen(), false)
          done()
        })
        w.show()
      })
      w.loadURL('about:blank')
    })
    it('should keep window hidden if already in hidden state', (done) => {
      w.webContents.once('did-finish-load', () => {
        w.once('leave-full-screen', () => {
          assert.strictEqual(w.isVisible(), false)
          assert.strictEqual(w.isFullScreen(), false)
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
        w.once('enter-full-screen', () => {
          w.once('leave-html-full-screen', () => {
            done()
          })
          w.setFullScreen(false)
        })
        w.webContents.executeJavaScript('document.body.webkitRequestFullscreen()', true)
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
        assert.strictEqual(c.getParentWindow(), w)
      })
      it('adds window to child windows of parent', () => {
        assert.deepStrictEqual(w.getChildWindows(), [c])
      })
      it('removes from child windows of parent when window is closed', (done) => {
        c.once('closed', () => {
          assert.deepStrictEqual(w.getChildWindows(), [])
          done()
        })
        c.close()
      })

      it('should not affect the show option', () => {
        assert.strictEqual(c.isVisible(), false)
        assert.strictEqual(c.getParentWindow().isVisible(), false)
      })
    })

    describe('win.setParentWindow(parent)', () => {
      beforeEach(() => {
        if (c != null) c.destroy()
        c = new BrowserWindow({ show: false })
      })

      it('sets parent window', () => {
        assert.strictEqual(w.getParentWindow(), null)
        assert.strictEqual(c.getParentWindow(), null)
        c.setParentWindow(w)
        assert.strictEqual(c.getParentWindow(), w)
        c.setParentWindow(null)
        assert.strictEqual(c.getParentWindow(), null)
      })
      it('adds window to child windows of parent', () => {
        assert.deepStrictEqual(w.getChildWindows(), [])
        c.setParentWindow(w)
        assert.deepStrictEqual(w.getChildWindows(), [c])
        c.setParentWindow(null)
        assert.deepStrictEqual(w.getChildWindows(), [])
      })
      it('removes from child windows of parent when window is closed', (done) => {
        c.once('closed', () => {
          assert.deepStrictEqual(w.getChildWindows(), [])
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
        assert.strictEqual(w.isEnabled(), true)
        c.show()
        assert.strictEqual(w.isEnabled(), false)
      })
      it('re-enables an enabled parent window when closed', (done) => {
        c.once('closed', () => {
          assert.strictEqual(w.isEnabled(), true)
          done()
        })
        c.show()
        c.close()
      })
      it('does not re-enable a disabled parent window when closed', (done) => {
        c.once('closed', () => {
          assert.strictEqual(w.isEnabled(), false)
          done()
        })
        w.setEnabled(false)
        c.show()
        c.close()
      })
      it('disables parent window recursively', () => {
        const c2 = new BrowserWindow({ show: false, parent: w, modal: true })
        c.show()
        assert.strictEqual(w.isEnabled(), false)
        c2.show()
        assert.strictEqual(w.isEnabled(), false)
        c.destroy()
        assert.strictEqual(w.isEnabled(), false)
        c2.destroy()
        assert.strictEqual(w.isEnabled(), true)
      })
    })
  })

  describe('window.webContents.send(channel, args...)', () => {
    it('throws an error when the channel is missing', () => {
      assert.throws(() => {
        w.webContents.send()
      }, 'Missing required channel argument')

      assert.throws(() => {
        w.webContents.send(null)
      }, 'Missing required channel argument')
    })
  })

  describe('window.getNativeWindowHandle()', () => {
    if (!nativeModulesEnabled) {
      this.skip()
    }

    it('returns valid handle', () => {
      // The module's source code is hosted at
      // https://github.com/electron/node-is-valid-window
      const isValidWindow = remote.require('is-valid-window')
      assert.ok(isValidWindow(w.getNativeWindowHandle()))
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
          devToolsWebContents.executeJavaScript(`(${showLastPanel})()`, false, () => {
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
          expect(BrowserWindow.getDevToolsExtensions().hasOwnProperty(extensionName)).to.equal(false)
        }

        const addExtension = () => {
          const extensionPath = path.join(__dirname, 'fixtures', 'devtools-extensions', 'foo')
          BrowserWindow.addDevToolsExtension(extensionPath)
          expect(BrowserWindow.getDevToolsExtensions().hasOwnProperty(extensionName)).to.equal(true)

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
        assert.strictEqual(message.runtimeId, 'foo')
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
      assert.deepStrictEqual(JSON.parse(fs.readFileSync(serializedPath)), [extensionPath])

      BrowserWindow.removeDevToolsExtension(extensionName)
      app.emit('will-quit')
      assert.strictEqual(fs.existsSync(serializedPath), false)
    })

    describe('BrowserWindow.addExtension', () => {
      beforeEach(() => {
        BrowserWindow.removeExtension('foo')
        assert.strictEqual(BrowserWindow.getExtensions().hasOwnProperty('foo'), false)

        const extensionPath = path.join(__dirname, 'fixtures', 'devtools-extensions', 'foo')
        BrowserWindow.addExtension(extensionPath)
        assert.strictEqual(BrowserWindow.getExtensions().hasOwnProperty('foo'), true)

        showLastDevToolsPanel()

        w.loadURL('about:blank')
      })

      it('throws errors for missing manifest.json files', () => {
        assert.throws(() => {
          BrowserWindow.addExtension(path.join(__dirname, 'does-not-exist'))
        }, /ENOENT: no such file or directory/)
      })

      it('throws errors for invalid manifest.json files', () => {
        assert.throws(() => {
          BrowserWindow.addExtension(path.join(__dirname, 'fixtures', 'devtools-extensions', 'bad-manifest'))
        }, /Unexpected token }/)
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

    it('doesnt throw when no calback is provided', () => {
      const result = ipcRenderer.sendSync('executeJavaScript', code, false)
      assert.strictEqual(result, 'success')
    })
    it('returns result when calback is provided', (done) => {
      ipcRenderer.send('executeJavaScript', code, true)
      ipcRenderer.once('executeJavaScript-response', function (event, result) {
        assert.strictEqual(result, expected)
        done()
      })
    })
    it('returns result if the code returns an asyncronous promise', (done) => {
      ipcRenderer.send('executeJavaScript', asyncCode, true)
      ipcRenderer.once('executeJavaScript-response', (event, result) => {
        assert.strictEqual(result, expected)
        done()
      })
    })
    it('resolves the returned promise with the result when a callback is specified', (done) => {
      ipcRenderer.send('executeJavaScript', code, true)
      ipcRenderer.once('executeJavaScript-promise-response', (event, result) => {
        assert.strictEqual(result, expected)
        done()
      })
    })
    it('resolves the returned promise with the result when no callback is specified', (done) => {
      ipcRenderer.send('executeJavaScript', code, false)
      ipcRenderer.once('executeJavaScript-promise-response', (event, result) => {
        assert.strictEqual(result, expected)
        done()
      })
    })
    it('resolves the returned promise with the result if the code returns an asyncronous promise', (done) => {
      ipcRenderer.send('executeJavaScript', asyncCode, true)
      ipcRenderer.once('executeJavaScript-promise-response', (event, result) => {
        assert.strictEqual(result, expected)
        done()
      })
    })
    it('rejects the returned promise if an async error is thrown', (done) => {
      ipcRenderer.send('executeJavaScript', badAsyncCode, true)
      ipcRenderer.once('executeJavaScript-promise-error', (event, error) => {
        assert.strictEqual(error, expectedErrorMsg)
        done()
      })
    })
    it('rejects the returned promise with an error if an Error.prototype is thrown', async () => {
      for (const error in errorTypes) {
        await new Promise((resolve) => {
          ipcRenderer.send('executeJavaScript', `Promise.reject(new ${error.name}("Wamp-wamp")`, true)
          ipcRenderer.once('executeJavaScript-promise-error-name', (event, name) => {
            assert.strictEqual(name, error.name)
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
        `, () => {
          w.webContents.executeJavaScript('console.log(\'hello\')', () => {
            done()
          })
        })
      })
      w.loadURL(server.url)
    })
    it('executes after page load', (done) => {
      w.webContents.executeJavaScript(code, (result) => {
        assert.strictEqual(result, expected)
        done()
      })
      w.loadURL(server.url)
    })
    it('works with result objects that have DOM class prototypes', (done) => {
      w.webContents.executeJavaScript('document.location', (result) => {
        assert.strictEqual(result.origin, server.url)
        assert.strictEqual(result.protocol, 'http:')
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
      assert.doesNotThrow(() => {
        w.previewFile(__filename)
        w.closeFilePreview()
      })
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
      assert.deepStrictEqual(data, expectedContextData)
    })
    it('recreates the contexts on reload', async () => {
      await iw.loadFile(path.join(fixtures, 'api', 'isolated.html'))
      const isolatedWorld = emittedOnce(ipcMain, 'isolated-world')
      iw.webContents.reload()
      const [, data] = await isolatedWorld
      assert.deepStrictEqual(data, expectedContextData)
    })
    it('enables context isolation on child windows', async () => {
      const browserWindowCreated = emittedOnce(app, 'browser-window-created')
      iw.loadFile(path.join(fixtures, 'pages', 'window-open.html'))
      const [, window] = await browserWindowCreated
      assert.ok(window.webContents.getLastWebPreferences().contextIsolation)
    })
    it('separates the page context from the Electron/preload context with sandbox on', async () => {
      const p = emittedOnce(ipcMain, 'isolated-world')
      ws.loadFile(path.join(fixtures, 'api', 'isolated.html'))
      const [, data] = await p
      assert.deepStrictEqual(data, expectedContextData)
    })
    it('recreates the contexts on reload with sandbox on', async () => {
      await ws.loadFile(path.join(fixtures, 'api', 'isolated.html'))
      const isolatedWorld = emittedOnce(ipcMain, 'isolated-world')
      ws.webContents.reload()
      const [, data] = await isolatedWorld
      assert.deepStrictEqual(data, expectedContextData)
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
      assert.strictEqual(error, 'Failed to fetch')
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
      assert.strictEqual(data.pageContext.openedLocation, '')
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
        assert.notStrictEqual(data.length, 0)
        const size = data.getSize()
        assertWithinDelta(size.width, 100, 2, 'width')
        assertWithinDelta(size.height, 100, 2, 'height')
        done()
      })
      w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'))
    })

    describe('window.webContents.isOffscreen()', () => {
      it('is true for offscreen type', () => {
        w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'))
        assert.strictEqual(w.webContents.isOffscreen(), true)
      })

      it('is false for regular window', () => {
        const c = new BrowserWindow({ show: false })
        assert.strictEqual(c.webContents.isOffscreen(), false)
        c.destroy()
      })
    })

    describe('window.webContents.isPainting()', () => {
      it('returns whether is currently painting', (done) => {
        w.webContents.once('paint', function (event, rect, data) {
          assert.strictEqual(w.webContents.isPainting(), true)
          done()
        })
        w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'))
      })
    })

    describe('window.webContents.stopPainting()', () => {
      it('stops painting', (done) => {
        w.webContents.on('dom-ready', () => {
          w.webContents.stopPainting()
          assert.strictEqual(w.webContents.isPainting(), false)
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
            assert.strictEqual(w.webContents.isPainting(), true)
            done()
          })
        })
        w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'))
      })
    })

    describe('window.webContents.getFrameRate()', () => {
      it('has default frame rate', (done) => {
        w.webContents.once('paint', function (event, rect, data) {
          assert.strictEqual(w.webContents.getFrameRate(), 60)
          done()
        })
        w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'))
      })
    })

    describe('window.webContents.setFrameRate(frameRate)', () => {
      it('sets custom frame rate', (done) => {
        w.webContents.on('dom-ready', () => {
          w.webContents.setFrameRate(30)
          w.webContents.once('paint', function (event, rect, data) {
            assert.strictEqual(w.webContents.getFrameRate(), 30)
            done()
          })
        })
        w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'))
      })
    })
  })
})

const assertBoundsEqual = (actual, expect) => {
  if (!isScaleFactorRounding()) {
    assert.deepStrictEqual(expect, actual)
  } else if (Array.isArray(actual)) {
    assertWithinDelta(actual[0], expect[0], 1, 'x')
    assertWithinDelta(actual[1], expect[1], 1, 'y')
  } else {
    assertWithinDelta(actual.x, expect.x, 1, 'x')
    assertWithinDelta(actual.y, expect.y, 1, 'y')
    assertWithinDelta(actual.width, expect.width, 1, 'width')
    assertWithinDelta(actual.height, expect.height, 1, 'height')
  }
}

const assertWithinDelta = (actual, expect, delta, label) => {
  const result = Math.abs(actual - expect)
  assert.ok(result <= delta, `${label} value of ${actual} was not within ${delta} of ${expect}`)
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

function serveFileFromProtocol (protocolName, filePath) {
  return new Promise((resolve, reject) => {
    protocol.registerBufferProtocol(protocolName, (request, callback) => {
      // Disabled due to false positive in StandardJS
      // eslint-disable-next-line standard/no-callback-literal
      callback({
        mimeType: 'text/html',
        data: fs.readFileSync(filePath)
      })
    }, (error) => {
      if (error != null) {
        reject(error)
      } else {
        resolve()
      }
    })
  })
}

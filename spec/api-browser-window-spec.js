'use strict'

const assert = require('assert')
const fs = require('fs')
const path = require('path')
const os = require('os')
const qs = require('querystring')
const http = require('http')
const {closeWindow} = require('./window-helpers')

const {ipcRenderer, remote, screen} = require('electron')
const {app, ipcMain, BrowserWindow, BrowserView, protocol, session, webContents} = remote

const isCI = remote.getGlobal('isCi')
const nativeModulesEnabled = remote.getGlobal('nativeModulesEnabled')

describe('BrowserWindow module', () => {
  const fixtures = path.resolve(__dirname, 'fixtures')
  let w = null
  let ws = null
  let server
  let postData

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
            let parsedData = qs.parse(body)
            fs.readFile(filePath, (err, data) => {
              if (err) return
              if (parsedData.username === 'test' &&
                  parsedData.file === data.toString()) {
                res.end()
              }
            })
          })
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

  beforeEach(() => {
    w = new BrowserWindow({
      show: false,
      width: 400,
      height: 400,
      webPreferences: {
        backgroundThrottling: false
      }
    })
  })

  afterEach(() => {
    return closeWindow(w).then(() => { w = null })
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
      w.webContents.on('did-finish-load', () => { w.close() })
      w.once('closed', () => {
        const test = path.join(fixtures, 'api', 'unload')
        const content = fs.readFileSync(test)
        fs.unlinkSync(test)
        assert.equal(String(content), 'unload')
        done()
      })
      w.loadURL('file://' + path.join(fixtures, 'api', 'unload.html'))
    })
    it('should emit beforeunload handler', (done) => {
      w.once('onbeforeunload', () => { done() })
      w.webContents.on('did-finish-load', () => { w.close() })
      w.loadURL(`file://${path.join(fixtures, 'api', 'beforeunload-false.html')}`)
    })
    it('should not crash when invoked synchronously inside navigation observer', (done) => {
      const events = [
        { name: 'did-start-loading', url: `${server.url}/200` },
        { name: 'did-get-redirect-request', url: `${server.url}/301` },
        { name: 'did-get-response-details', url: `${server.url}/200` },
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
          let w = new BrowserWindow({show: false})
          eventOptions.id = w.id
          eventOptions.responseEvent = responseEvent
          ipcRenderer.send('test-webcontents-navigation-observer', eventOptions)
          yield 1
        }
      }

      let gen = genNavigationEvent()
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
        assert.equal(String(content), 'close')
        done()
      })
      w.loadURL(`file://${path.join(fixtures, 'api', 'close.html')}`)
    })
    it('should emit beforeunload handler', (done) => {
      w.once('onbeforeunload', () => { done() })
      w.loadURL(`file://${path.join(fixtures, 'api', 'close-beforeunload-false.html')}`)
    })
  })

  describe('BrowserWindow.destroy()', () => {
    it('prevents users to access methods of webContents', () => {
      const contents = w.webContents
      w.destroy()
      assert.throws(() => {
        contents.getId()
      }, /Object has been destroyed/)
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
    it('should emit did-get-response-details event', (done) => {
      // expected {fileName: resourceType} pairs
      const expectedResources = {
        'did-get-response-details.html': 'mainFrame',
        'logo.png': 'image'
      }
      let responses = 0
      w.webContents.on('did-get-response-details', (event, status, newUrl, oldUrl, responseCode, method, referrer, headers, resourceType) => {
        responses += 1
        const fileName = newUrl.slice(newUrl.lastIndexOf('/') + 1)
        const expectedType = expectedResources[fileName]
        assert(!!expectedType, `Unexpected response details for ${newUrl}`)
        assert(typeof status === 'boolean', 'status should be boolean')
        assert.equal(responseCode, 200)
        assert.equal(method, 'GET')
        assert(typeof referrer === 'string', 'referrer should be string')
        assert(!!headers, 'headers should be present')
        assert(typeof headers === 'object', 'headers should be object')
        assert.equal(resourceType, expectedType, 'Incorrect resourceType')
        if (responses === Object.keys(expectedResources).length) done()
      })
      w.loadURL(`file://${path.join(fixtures, 'pages', 'did-get-response-details.html')}`)
    })
    it('should emit did-fail-load event for files that do not exist', (done) => {
      w.webContents.on('did-fail-load', (event, code, desc, url, isMainFrame) => {
        assert.equal(code, -6)
        assert.equal(desc, 'ERR_FILE_NOT_FOUND')
        assert.equal(isMainFrame, true)
        done()
      })
      w.loadURL('file://a.txt')
    })
    it('should emit did-fail-load event for invalid URL', (done) => {
      w.webContents.on('did-fail-load', (event, code, desc, url, isMainFrame) => {
        assert.equal(desc, 'ERR_INVALID_URL')
        assert.equal(code, -300)
        assert.equal(isMainFrame, true)
        done()
      })
      w.loadURL('http://example:port')
    })
    it('should set `mainFrame = false` on did-fail-load events in iframes', (done) => {
      w.webContents.on('did-fail-load', (event, code, desc, url, isMainFrame) => {
        assert.equal(isMainFrame, false)
        done()
      })
      w.loadURL(`file://${path.join(fixtures, 'api', 'did-fail-load-iframe.html')}`)
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
        assert.equal(desc, 'ERR_INVALID_URL')
        assert.equal(code, -300)
        assert.equal(isMainFrame, true)
        done()
      })
      const data = Buffer.alloc(2 * 1024 * 1024).toString('base64')
      w.loadURL(`data:image/png;base64,${data}`)
    })

    describe('POST navigations', () => {
      afterEach(() => { w.webContents.session.webRequest.onBeforeSendHeaders(null) })

      it('supports specifying POST data', (done) => {
        w.webContents.on('did-finish-load', () => done())
        w.loadURL(server.url, {postData: postData})
      })
      it('sets the content type header on URL encoded forms', (done) => {
        w.webContents.on('did-finish-load', () => {
          w.webContents.session.webRequest.onBeforeSendHeaders((details, callback) => {
            assert.equal(details.requestHeaders['content-type'], 'application/x-www-form-urlencoded')
            done()
          })
          w.webContents.executeJavaScript(`
            form = document.createElement('form')
            document.body.appendChild(form)
            form.method = 'POST'
            form.target = '_blank'
            form.submit()
          `)
        })
        w.loadURL(server.url)
      })
      it('sets the content type header on multi part forms', (done) => {
        w.webContents.on('did-finish-load', () => {
          w.webContents.session.webRequest.onBeforeSendHeaders((details, callback) => {
            assert(details.requestHeaders['content-type'].startsWith('multipart/form-data; boundary=----WebKitFormBoundary'))
            done()
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
        })
        w.loadURL(server.url)
      })
    })

    it('should support support base url for data urls', (done) => {
      ipcMain.once('answer', (event, test) => {
        assert.equal(test, 'test')
        done()
      })
      w.loadURL('data:text/html,<script src="loaded-from-dataurl.js"></script>', {baseURLForDataURL: `file://${path.join(fixtures, 'api')}${path.sep}`})
    })
  })

  describe('will-navigate event', () => {
    it('allows the window to be closed from the event listener', (done) => {
      ipcRenderer.send('close-on-will-navigate', w.id)
      ipcRenderer.once('closed-on-will-navigate', () => { done() })
      w.loadURL(`file://${fixtures}/pages/will-navigate.html`)
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
        assert.equal(w.isVisible(), true)
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
        assert.equal(w.isVisible(), false)
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
      assert.equal(w.isVisible(), false)
      w.focus()
      assert.equal(w.isVisible(), false)
    })
  })

  describe('BrowserWindow.blur()', () => {
    it('removes focus from window', () => {
      w.blur()
      assert(!w.isFocused())
    })
  })

  describe('BrowserWindow.capturePage(rect, callback)', () => {
    it('calls the callback with a Buffer', (done) => {
      w.capturePage({
        x: 0,
        y: 0,
        width: 100,
        height: 100
      }, (image) => {
        assert.equal(image.isEmpty(), true)
        done()
      })
    })
  })

  describe('BrowserWindow.setSize(width, height)', () => {
    it('sets the window size', (done) => {
      const size = [300, 400]
      w.once('resize', () => {
        assertBoundsEqual(w.getSize(), size)
        done()
      })
      w.setSize(size[0], size[1])
    })
  })

  describe('BrowserWindow.setMinimum/MaximumSize(width, height)', () => {
    it('sets the maximum and minimum size of the window', () => {
      assert.deepEqual(w.getMinimumSize(), [0, 0])
      assert.deepEqual(w.getMaximumSize(), [0, 0])

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
        assert.equal(newPos[0], pos[0])
        assert.equal(newPos[1], pos[1])
        done()
      })
      w.setPosition(pos[0], pos[1])
    })
  })

  describe('BrowserWindow.setContentSize(width, height)', () => {
    it('sets the content size', () => {
      const size = [400, 400]
      w.setContentSize(size[0], size[1])
      var after = w.getContentSize()
      assert.equal(after[0], size[0])
      assert.equal(after[1], size[1])
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
      assert.equal(after[0], size[0])
      assert.equal(after[1], size[1])
    })
  })

  describe('BrowserWindow.setContentBounds(bounds)', () => {
    it('sets the content size and position', (done) => {
      const bounds = {x: 10, y: 10, width: 250, height: 250}
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
      const bounds = {x: 10, y: 10, width: 250, height: 250}
      w.once('resize', () => {
        assert.deepEqual(w.getContentBounds(), bounds)
        done()
      })
      w.setContentBounds(bounds)
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
        w.setProgressBar(0.5, {mode: 'paused'})
      })
    })
    it('sets the progress using "error" mode', () => {
      assert.doesNotThrow(() => {
        w.setProgressBar(0.5, {mode: 'error'})
      })
    })
    it('sets the progress using "normal" mode', () => {
      assert.doesNotThrow(() => {
        w.setProgressBar(0.5, {mode: 'normal'})
      })
    })
  })

  describe('BrowserWindow.setAlwaysOnTop(flag, level)', () => {
    it('sets the window as always on top', () => {
      assert.equal(w.isAlwaysOnTop(), false)
      w.setAlwaysOnTop(true, 'screen-saver')
      assert.equal(w.isAlwaysOnTop(), true)
      w.setAlwaysOnTop(false)
      assert.equal(w.isAlwaysOnTop(), false)
      w.setAlwaysOnTop(true)
      assert.equal(w.isAlwaysOnTop(), true)
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
      assert.equal(w.isAlwaysOnTop(), false)
      w.setAlwaysOnTop(true, 'screen-saver')
      assert.equal(w.isAlwaysOnTop(), true)
      w.minimize()
      assert.equal(w.isAlwaysOnTop(), false)
      w.restore()
      assert.equal(w.isAlwaysOnTop(), true)
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

  // FIXME(alexeykuzmin): Fails on Mac.
  xdescribe('BrowserWindow.addTabbedWindow()', () => {
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
      closeWindow(tabbedWindow).then(done)
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
        w.setAppDetails({appId: 'my.app.id'})
        w.setAppDetails({appIconPath: iconPath, appIconIndex: 0})
        w.setAppDetails({appIconPath: iconPath})
        w.setAppDetails({relaunchCommand: 'my-app.exe arg1 arg2', relaunchDisplayName: 'My app name'})
        w.setAppDetails({relaunchCommand: 'my-app.exe arg1 arg2'})
        w.setAppDetails({relaunchDisplayName: 'My app name'})
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
      assert.equal(w.id, BrowserWindow.fromId(w.id).id)
    })
  })

  describe('BrowserWindow.fromWebContents(webContents)', () => {
    let contents = null

    beforeEach(() => { contents = webContents.create({}) })

    afterEach(() => { contents.destroy() })

    it('returns the window with the webContents', () => {
      assert.equal(BrowserWindow.fromWebContents(w.webContents).id, w.id)
      assert.equal(BrowserWindow.fromWebContents(contents), undefined)
    })
  })

  describe('BrowserWindow.fromDevToolsWebContents(webContents)', () => {
    let contents = null

    beforeEach(() => { contents = webContents.create({}) })

    afterEach(() => { contents.destroy() })

    it('returns the window with the webContents', (done) => {
      w.webContents.once('devtools-opened', () => {
        assert.equal(BrowserWindow.fromDevToolsWebContents(w.devToolsWebContents).id, w.id)
        assert.equal(BrowserWindow.fromDevToolsWebContents(w.webContents), undefined)
        assert.equal(BrowserWindow.fromDevToolsWebContents(contents), undefined)
        done()
      })
      w.webContents.openDevTools()
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
      assert.equal(BrowserWindow.fromBrowserView(bv).id, w.id)
    })

    it('returns undefined if not attached', () => {
      w.setBrowserView(null)
      assert.equal(BrowserWindow.fromBrowserView(bv), undefined)
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
      assert.equal(w.getOpacity(), 0.5)
    })
    it('allows setting the opacity', () => {
      assert.doesNotThrow(() => {
        w.setOpacity(0.0)
        assert.equal(w.getOpacity(), 0.0)
        w.setOpacity(0.5)
        assert.equal(w.getOpacity(), 0.5)
        w.setOpacity(1.0)
        assert.equal(w.getOpacity(), 1.0)
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
      assert.equal(contentSize[0], 400)
      assert.equal(contentSize[1], 400)
    })
    it('make window created with window size when not used', () => {
      const size = w.getSize()
      assert.equal(size[0], 400)
      assert.equal(size[1], 400)
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
      assert.equal(contentSize[0], 400)
      assert.equal(contentSize[1], 400)
      const size = w.getSize()
      assert.equal(size[0], 400)
      assert.equal(size[1], 400)
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
      assert.equal(contentSize[1], 400)
    })
    it('creates browser window with hidden inset title bar', () => {
      w.destroy()
      w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        titleBarStyle: 'hidden-inset'
      })
      const contentSize = w.getContentSize()
      assert.equal(contentSize[1], 400)
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
      assert.equal(after[0], -10)
      assert.equal(after[1], -10)
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
      assert.equal(w.getSize()[0], 500)
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
      it('loads the script before other scripts in window', (done) => {
        const preload = path.join(fixtures, 'module', 'set-global.js')
        ipcMain.once('answer', (event, test) => {
          assert.equal(test, 'preload')
          done()
        })
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            preload: preload
          }
        })
        w.loadURL(`file://${path.join(fixtures, 'api', 'preload.html')}`)
      })
      it('can successfully delete the Buffer global', (done) => {
        const preload = path.join(fixtures, 'module', 'delete-buffer.js')
        ipcMain.once('answer', (event, test) => {
          assert.equal(test.toString(), 'buffer')
          done()
        })
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            preload: preload
          }
        })
        w.loadURL(`file://${path.join(fixtures, 'api', 'preload.html')}`)
      })
    })

    describe('session preload scripts', function () {
      const preloads = [
        path.join(fixtures, 'module', 'set-global-preload-1.js'),
        path.join(fixtures, 'module', 'set-global-preload-2.js')
      ]
      const defaultSession = session.defaultSession

      beforeEach(() => {
        assert.deepEqual(defaultSession.getPreloads(), [])
        defaultSession.setPreloads(preloads)
      })
      afterEach(() => {
        defaultSession.setPreloads([])
      })

      it('can set multiple session preload script', function () {
        assert.deepEqual(defaultSession.getPreloads(), preloads)
      })

      it('loads the script before other scripts in window including normal preloads', function (done) {
        ipcMain.once('vars', function (event, preload1, preload2, preload3) {
          assert.equal(preload1, 'preload-1')
          assert.equal(preload2, 'preload-1-2')
          assert.equal(preload3, 'preload-1-2-3')
          done()
        })
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            preload: path.join(fixtures, 'module', 'set-global-preload-3.js')
          }
        })
        w.loadURL('file://' + path.join(fixtures, 'api', 'preloads.html'))
      })
    })

    describe('"node-integration" option', () => {
      it('disables node integration when specified to false', (done) => {
        const preload = path.join(fixtures, 'module', 'send-later.js')
        ipcMain.once('answer', (event, typeofProcess, typeofBuffer) => {
          assert.equal(typeofProcess, 'undefined')
          assert.equal(typeofBuffer, 'undefined')
          done()
        })
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            preload: preload,
            nodeIntegration: false
          }
        })
        w.loadURL(`file://${path.join(fixtures, 'api', 'blank.html')}`)
      })
    })

    describe('"sandbox" option', () => {
      function waitForEvents (emitter, events, callback) {
        let count = events.length
        for (let event of events) {
          emitter.once(event, () => {
            if (!--count) callback()
          })
        }
      }

      const preload = path.join(fixtures, 'module', 'preload-sandbox.js')

      // http protocol to simulate accessing another domain. This is required
      // because the code paths for cross domain popups is different.
      function crossDomainHandler (request, callback) {
        // Disabled due to false positive in StandardJS
        // eslint-disable-next-line standard/no-callback-literal
        callback({
          mimeType: 'text/html',
          data: `<html><body><h1>${request.url}</h1></body></html>`
        })
      }

      before((done) => {
        protocol.interceptStringProtocol('http', crossDomainHandler, () => {
          done()
        })
      })

      after((done) => {
        protocol.uninterceptProtocol('http', () => {
          done()
        })
      })

      it('exposes ipcRenderer to preload script', (done) => {
        ipcMain.once('answer', function (event, test) {
          assert.equal(test, 'preload')
          done()
        })
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload: preload
          }
        })
        w.loadURL('file://' + path.join(fixtures, 'api', 'preload.html'))
      })

      it('exposes "exit" event to preload script', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload: preload
          }
        })
        let htmlPath = path.join(fixtures, 'api', 'sandbox.html?exit-event')
        const pageUrl = 'file://' + htmlPath
        w.loadURL(pageUrl)
        ipcMain.once('answer', function (event, url) {
          let expectedUrl = pageUrl
          if (process.platform === 'win32') {
            expectedUrl = 'file:///' + htmlPath.replace(/\\/g, '/')
          }
          assert.equal(url, expectedUrl)
          done()
        })
      })

      it('should open windows in same domain with cross-scripting enabled', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload: preload
          }
        })
        ipcRenderer.send('set-web-preferences-on-next-new-window', w.webContents.id, 'preload', preload)
        let htmlPath = path.join(fixtures, 'api', 'sandbox.html?window-open')
        const pageUrl = 'file://' + htmlPath
        w.loadURL(pageUrl)
        w.webContents.once('new-window', (e, url, frameName, disposition, options) => {
          let expectedUrl = pageUrl
          if (process.platform === 'win32') {
            expectedUrl = 'file:///' + htmlPath.replace(/\\/g, '/')
          }
          assert.equal(url, expectedUrl)
          assert.equal(frameName, 'popup!')
          assert.equal(options.width, 500)
          assert.equal(options.height, 600)
          ipcMain.once('answer', function (event, html) {
            assert.equal(html, '<h1>scripting from opener</h1>')
            done()
          })
        })
      })

      it('should open windows in another domain with cross-scripting disabled', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload: preload
          }
        })
        ipcRenderer.send('set-web-preferences-on-next-new-window', w.webContents.id, 'preload', preload)
        let htmlPath = path.join(fixtures, 'api', 'sandbox.html?window-open-external')
        const pageUrl = 'file://' + htmlPath
        let popupWindow
        w.loadURL(pageUrl)
        w.webContents.once('new-window', (e, url, frameName, disposition, options) => {
          assert.equal(url, 'http://www.google.com/#q=electron')
          assert.equal(options.width, 505)
          assert.equal(options.height, 605)
          ipcMain.once('child-loaded', function (event, openerIsNull, html) {
            assert(openerIsNull)
            assert.equal(html, '<h1>http://www.google.com/#q=electron</h1>')
            ipcMain.once('answer', function (event, exceptionMessage) {
              assert(/Blocked a frame with origin/.test(exceptionMessage))

              // FIXME this popup window should be closed in sandbox.html
              closeWindow(popupWindow, {assertSingleWindow: false}).then(() => {
                popupWindow = null
                done()
              })
            })
            w.webContents.send('child-loaded')
          })
        })

        app.once('browser-window-created', function (event, window) {
          popupWindow = window
        })
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
          assert.equal(args.includes('--enable-sandbox'), true)
          done()
        })
        w.loadURL(`file://${path.join(fixtures, 'api', 'new-window.html')}`)
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
          assert.equal(webPreferences.foo, 'bar')
          done()
        })
        w.loadURL(`file://${path.join(fixtures, 'api', 'new-window.html')}`)
      })

      it('should set ipc event sender correctly', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload: preload
          }
        })
        let htmlPath = path.join(fixtures, 'api', 'sandbox.html?verify-ipc-sender')
        const pageUrl = 'file://' + htmlPath
        w.loadURL(pageUrl)
        w.webContents.once('new-window', (e, url, frameName, disposition, options) => {
          let parentWc = w.webContents
          let childWc = options.webContents
          assert.notEqual(parentWc, childWc)
          ipcMain.once('parent-ready', function (event) {
            assert.equal(parentWc, event.sender)
            parentWc.send('verified')
          })
          ipcMain.once('child-ready', function (event) {
            assert.equal(childWc, event.sender)
            childWc.send('verified')
          })
          waitForEvents(ipcMain, [
            'parent-answer',
            'child-answer'
          ], done)
        })
      })

      describe('event handling', () => {
        it('works for window events', (done) => {
          waitForEvents(w, [
            'page-title-updated'
          ], done)
          w.loadURL('file://' + path.join(fixtures, 'api', 'sandbox.html?window-events'))
        })

        it('works for stop events', (done) => {
          waitForEvents(w.webContents, [
            'did-navigate',
            'did-fail-load',
            'did-stop-loading'
          ], done)
          w.loadURL('file://' + path.join(fixtures, 'api', 'sandbox.html?webcontents-stop'))
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
          w.loadURL('file://' + path.join(fixtures, 'api', 'sandbox.html?webcontents-events'))
        })
      })

      it('can get printer list', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload: preload
          }
        })
        w.loadURL('data:text/html,%3Ch1%3EHello%2C%20World!%3C%2Fh1%3E')
        w.webContents.once('did-finish-load', () => {
          const printers = w.webContents.getPrinters()
          assert.equal(Array.isArray(printers), true)
          done()
        })
      })

      it('can print to PDF', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload: preload
          }
        })
        w.loadURL('data:text/html,%3Ch1%3EHello%2C%20World!%3C%2Fh1%3E')
        w.webContents.once('did-finish-load', () => {
          w.webContents.printToPDF({}, function (error, data) {
            assert.equal(error, null)
            assert.equal(data instanceof Buffer, true)
            assert.notEqual(data.length, 0)
            done()
          })
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
            assert.deepEqual(currentWebContents, initialWebContents)
            done()
          }, 100)
        })
        w.loadURL('file://' + path.join(fixtures, 'pages', 'window-open.html'))
      })

      it('releases memory after popup is closed', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            preload: preload,
            sandbox: true
          }
        })
        w.loadURL('file://' + path.join(fixtures, 'api', 'sandbox.html?allocate-memory'))
        ipcMain.once('answer', function (event, {bytesBeforeOpen, bytesAfterOpen, bytesAfterClose}) {
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
            preload: preload,
            sandbox: true
          }
        })
        w.loadURL('file://' + path.join(fixtures, 'api', 'sandbox.html?reload-remote'))

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
          assert.equal(arg, 'hi')
          done()
        })
      })

      it('properly manages remote object references after page reload in child window', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            preload: preload,
            sandbox: true
          }
        })
        w.loadURL('file://' + path.join(fixtures, 'api', 'sandbox.html?reload-remote-child'))

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
          assert.equal(arg, 'hi child window')
          done()
        })
      })
    })

    describe('nativeWindowOpen option', () => {
      beforeEach(() => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            nativeWindowOpen: true
          }
        })
      })

      it('opens window of about:blank with cross-scripting enabled', (done) => {
        ipcMain.once('answer', (event, content) => {
          assert.equal(content, 'Hello')
          done()
        })
        w.loadURL(`file://${path.join(fixtures, 'api', 'native-window-open-blank.html')}`)
      })
      it('opens window of same domain with cross-scripting enabled', (done) => {
        ipcMain.once('answer', (event, content) => {
          assert.equal(content, 'Hello')
          done()
        })
        w.loadURL(`file://${path.join(fixtures, 'api', 'native-window-open-file.html')}`)
      })
      it('blocks accessing cross-origin frames', (done) => {
        ipcMain.once('answer', (event, content) => {
          assert.equal(content, 'Blocked a frame with origin "file://" from accessing a cross-origin frame.')
          done()
        })
        w.loadURL(`file://${path.join(fixtures, 'api', 'native-window-open-cross-origin.html')}`)
      })
      it('opens window from <iframe> tags', (done) => {
        ipcMain.once('answer', (event, content) => {
          assert.equal(content, 'Hello')
          done()
        })
        w.loadURL(`file://${path.join(fixtures, 'api', 'native-window-open-iframe.html')}`)
      })
      it('loads native addons correctly after reload', (done) => {
        if (!nativeModulesEnabled) return done()

        ipcMain.once('answer', (event, content) => {
          assert.equal(content, 'function')
          ipcMain.once('answer', (event, content) => {
            assert.equal(content, 'function')
            done()
          })
          w.reload()
        })
        w.loadURL(`file://${path.join(fixtures, 'api', 'native-window-open-native-addon.html')}`)
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
          assert.equal(args.includes('--native-window-open'), true)
          done()
        })
        w.loadURL(`file://${path.join(fixtures, 'api', 'new-window.html')}`)
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
          assert.equal(webPreferences.foo, 'bar')
          done()
        })
        w.loadURL(`file://${path.join(fixtures, 'api', 'new-window.html')}`)
      })
      it('retains the original web preferences when window.location is changed to a new origin', async () => {
        await serveFileFromProtocol('foo', path.join(fixtures, 'api', 'window-open-location-change.html'))
        await serveFileFromProtocol('bar', path.join(fixtures, 'api', 'window-open-location-final.html'))

        w.destroy()
        w = new BrowserWindow({
          show: true,
          webPreferences: {
            nodeIntegration: false,
            nativeWindowOpen: true
          }
        })

        return new Promise((resolve, reject) => {
          ipcRenderer.send('set-web-preferences-on-next-new-window', w.webContents.id, 'preload', path.join(fixtures, 'api', 'window-open-preload.js'))
          ipcMain.once('answer', (event, args, typeofProcess) => {
            assert.equal(args.includes('--node-integration=false'), true)
            assert.equal(args.includes('--native-window-open'), true)
            assert.equal(typeofProcess, 'undefined')
            resolve()
          })
          w.loadURL(`file://${path.join(fixtures, 'api', 'window-open-location-open.html')}`)
        })
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
        assert.equal(content, 'Hello')
        done()
      })
      w.loadURL(`file://${path.join(fixtures, 'api', 'native-window-open-isolated.html')}`)
    })
  })

  describe('beforeunload handler', () => {
    it('returning undefined would not prevent close', (done) => {
      w.once('closed', () => { done() })
      w.loadURL('file://' + path.join(fixtures, 'api', 'close-beforeunload-undefined.html'))
    })
    it('returning false would prevent close', (done) => {
      w.once('onbeforeunload', () => { done() })
      w.loadURL(`file://${path.join(fixtures, 'api', 'close-beforeunload-false.html')}`)
    })
    it('returning empty string would prevent close', (done) => {
      w.once('onbeforeunload', () => { done() })
      w.loadURL(`file://${path.join(fixtures, 'api', 'close-beforeunload-empty-string.html')}`)
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
      w.loadURL(`file://${path.join(fixtures, 'api', 'beforeunload-false-prevent3.html')}`)
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
      w.loadURL(`file://${path.join(fixtures, 'api', 'beforeunload-false-prevent3.html')}`)
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
      w.loadURL(`file://${path.join(fixtures, 'api', 'beforeunload-false-prevent3.html')}`)
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
      w = new BrowserWindow({ show: false, width: 100, height: 100 })

      let readyToShow = false
      w.once('ready-to-show', () => {
        readyToShow = true
      })

      onNextVisibilityChange((visibilityState, hidden) => {
        assert.equal(readyToShow, false)
        assert.equal(visibilityState, 'visible')
        assert.equal(hidden, false)

        done()
      })

      w.loadURL(`file://${path.join(fixtures, 'pages', 'visibilitychange.html')}`)
    })
    it('visibilityState changes when window is hidden', (done) => {
      w = new BrowserWindow({width: 100, height: 100})

      onNextVisibilityChange((visibilityState, hidden) => {
        assert.equal(visibilityState, 'visible')
        assert.equal(hidden, false)

        onNextVisibilityChange((visibilityState, hidden) => {
          assert.equal(visibilityState, 'hidden')
          assert.equal(hidden, true)
          done()
        })

        w.hide()
      })

      w.loadURL(`file://${path.join(fixtures, 'pages', 'visibilitychange.html')}`)
    })
    it('visibilityState changes when window is shown', (done) => {
      w = new BrowserWindow({width: 100, height: 100})

      onNextVisibilityChange((visibilityState, hidden) => {
        onVisibilityChange((visibilityState, hidden) => {
          if (!hidden) {
            assert.equal(visibilityState, 'visible')
            done()
          }
        })

        w.hide()
        w.show()
      })

      w.loadURL(`file://${path.join(fixtures, 'pages', 'visibilitychange.html')}`)
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

      w = new BrowserWindow({width: 100, height: 100})

      onNextVisibilityChange((visibilityState, hidden) => {
        onVisibilityChange((visibilityState, hidden) => {
          if (!hidden) {
            assert.equal(visibilityState, 'visible')
            done()
          }
        })

        w.hide()
        w.showInactive()
      })

      w.loadURL(`file://${path.join(fixtures, 'pages', 'visibilitychange.html')}`)
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

      w = new BrowserWindow({width: 100, height: 100})

      onNextVisibilityChange((visibilityState, hidden) => {
        assert.equal(visibilityState, 'visible')
        assert.equal(hidden, false)

        onNextVisibilityChange((visibilityState, hidden) => {
          assert.equal(visibilityState, 'hidden')
          assert.equal(hidden, true)
          done()
        })

        w.minimize()
      })

      w.loadURL(`file://${path.join(fixtures, 'pages', 'visibilitychange.html')}`)
    })
    it('visibilityState remains visible if backgroundThrottling is disabled', (done) => {
      w = new BrowserWindow({
        show: false,
        width: 100,
        height: 100,
        webPreferences: {
          backgroundThrottling: false
        }
      })

      onNextVisibilityChange((visibilityState, hidden) => {
        assert.equal(visibilityState, 'visible')
        assert.equal(hidden, false)

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

      w.loadURL(`file://${path.join(fixtures, 'pages', 'visibilitychange.html')}`)
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
        assert.equal(url, 'http://host/')
        assert.equal(frameName, 'host')
        assert.equal(additionalFeatures[0], 'this-is-not-a-standard-feature')
        done()
      })
      w.loadURL(`file://${fixtures}/pages/window-open.html`)
    })
    it('emits when window.open is called with no webPreferences', (done) => {
      w.destroy()
      w = new BrowserWindow({ show: false })
      w.webContents.once('new-window', function (e, url, frameName, disposition, options, additionalFeatures) {
        e.preventDefault()
        assert.equal(url, 'http://host/')
        assert.equal(frameName, 'host')
        assert.equal(additionalFeatures[0], 'this-is-not-a-standard-feature')
        done()
      })
      w.loadURL(`file://${fixtures}/pages/window-open.html`)
    })
    it('emits when link with target is called', (done) => {
      w.webContents.once('new-window', (e, url, frameName) => {
        e.preventDefault()
        assert.equal(url, 'http://host/')
        assert.equal(frameName, 'target')
        done()
      })
      w.loadURL(`file://${fixtures}/pages/target-name.html`)
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
      return closeWindow(sheet, {assertSingleWindow: false}).then(() => { sheet = null })
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
      return closeWindow(sheet, {assertSingleWindow: false}).then(() => { sheet = null })
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
      w.loadURL(`file://${fixtures}/api/frame-subscriber.html`)
      w.webContents.on('dom-ready', () => {
        w.webContents.beginFrameSubscription(function (data) {
          // This callback might be called twice.
          if (called) return
          called = true

          assert.notEqual(data.length, 0)
          w.webContents.endFrameSubscription()
          done()
        })
      })
    })
    it('subscribes to frame updates (only dirty rectangle)', (done) => {
      let called = false
      w.loadURL(`file://${fixtures}/api/frame-subscriber.html`)
      w.webContents.on('dom-ready', () => {
        w.webContents.beginFrameSubscription(true, (data) => {
          // This callback might be called twice.
          if (called) return
          called = true

          assert.notEqual(data.length, 0)
          w.webContents.endFrameSubscription()
          done()
        })
      })
    })
    it('throws error when subscriber is not well defined', (done) => {
      w.loadURL(`file://${fixtures}'/api/frame-subscriber.html`)
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

    it('should save page to disk', (done) => {
      w.webContents.on('did-finish-load', () => {
        w.webContents.savePage(savePageHtmlPath, 'HTMLComplete', function (error) {
          assert.equal(error, null)
          assert(fs.existsSync(savePageHtmlPath))
          assert(fs.existsSync(savePageJsPath))
          assert(fs.existsSync(savePageCssPath))
          done()
        })
      })
      w.loadURL('file://' + fixtures + '/pages/save_page/index.html')
    })
  })

  describe('BrowserWindow options argument is optional', () => {
    it('should create a window with default size (800x600)', () => {
      w.destroy()
      w = new BrowserWindow()
      const size = w.getSize()
      assert.equal(size[0], 800)
      assert.equal(size[1], 600)
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
      assert.deepEqual(w.getSize(), [300, 200])

      w.setResizable(false)
      w.setResizable(true)
      assert.deepEqual(w.getSize(), [300, 200])

      w.setMaximizable(false)
      w.setMaximizable(true)
      assert.deepEqual(w.getSize(), [300, 200])

      w.setFullScreenable(false)
      w.setFullScreenable(true)
      assert.deepEqual(w.getSize(), [300, 200])

      w.setClosable(false)
      w.setClosable(true)
      assert.deepEqual(w.getSize(), [300, 200])
    })

    describe('resizable state', () => {
      it('can be changed with resizable option', () => {
        w.destroy()
        w = new BrowserWindow({show: false, resizable: false})
        assert.equal(w.isResizable(), false)

        if (process.platform === 'darwin') {
          assert.equal(w.isMaximizable(), true)
        }
      })

      it('can be changed with setResizable method', () => {
        assert.equal(w.isResizable(), true)
        w.setResizable(false)
        assert.equal(w.isResizable(), false)
        w.setResizable(true)
        assert.equal(w.isResizable(), true)
      })

      it('works for a frameless window', () => {
        w.destroy()
        w = new BrowserWindow({show: false, frame: false})
        assert.equal(w.isResizable(), true)

        if (process.platform === 'win32') {
          w.destroy()
          w = new BrowserWindow({show: false, thickFrame: false})
          assert.equal(w.isResizable(), false)
        }
      })
    })

    describe('loading main frame state', () => {
      it('is true when the main frame is loading', (done) => {
        w.webContents.on('did-start-loading', () => {
          assert.equal(w.webContents.isLoadingMainFrame(), true)
          done()
        })
        w.webContents.loadURL(server.url)
      })
      it('is false when only a subframe is loading', (done) => {
        w.webContents.once('did-finish-load', () => {
          assert.equal(w.webContents.isLoadingMainFrame(), false)
          w.webContents.on('did-start-loading', () => {
            assert.equal(w.webContents.isLoadingMainFrame(), false)
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
          assert.equal(w.webContents.isLoadingMainFrame(), false)
          w.webContents.on('did-start-loading', () => {
            assert.equal(w.webContents.isLoadingMainFrame(), true)
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
        w = new BrowserWindow({show: false, movable: false})
        assert.equal(w.isMovable(), false)
      })
      it('can be changed with setMovable method', () => {
        assert.equal(w.isMovable(), true)
        w.setMovable(false)
        assert.equal(w.isMovable(), false)
        w.setMovable(true)
        assert.equal(w.isMovable(), true)
      })
    })

    describe('minimizable state', () => {
      it('can be changed with minimizable option', () => {
        w.destroy()
        w = new BrowserWindow({show: false, minimizable: false})
        assert.equal(w.isMinimizable(), false)
      })

      it('can be changed with setMinimizable method', () => {
        assert.equal(w.isMinimizable(), true)
        w.setMinimizable(false)
        assert.equal(w.isMinimizable(), false)
        w.setMinimizable(true)
        assert.equal(w.isMinimizable(), true)
      })
    })

    describe('maximizable state', () => {
      it('can be changed with maximizable option', () => {
        w.destroy()
        w = new BrowserWindow({show: false, maximizable: false})
        assert.equal(w.isMaximizable(), false)
      })

      it('can be changed with setMaximizable method', () => {
        assert.equal(w.isMaximizable(), true)
        w.setMaximizable(false)
        assert.equal(w.isMaximizable(), false)
        w.setMaximizable(true)
        assert.equal(w.isMaximizable(), true)
      })

      it('is not affected when changing other states', () => {
        w.setMaximizable(false)
        assert.equal(w.isMaximizable(), false)
        w.setMinimizable(false)
        assert.equal(w.isMaximizable(), false)
        w.setClosable(false)
        assert.equal(w.isMaximizable(), false)

        w.setMaximizable(true)
        assert.equal(w.isMaximizable(), true)
        w.setClosable(true)
        assert.equal(w.isMaximizable(), true)
        w.setFullScreenable(false)
        assert.equal(w.isMaximizable(), true)
      })
    })

    describe('maximizable state (Windows only)', () => {
      // Only implemented on windows.
      if (process.platform !== 'win32') return

      it('is set to false when resizable state is set to false', () => {
        w.setResizable(false)
        assert.equal(w.isMaximizable(), false)
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
        w = new BrowserWindow({show: false, fullscreenable: false})
        assert.equal(w.isFullScreenable(), false)
      })

      it('can be changed with setFullScreenable method', () => {
        assert.equal(w.isFullScreenable(), true)
        w.setFullScreenable(false)
        assert.equal(w.isFullScreenable(), false)
        w.setFullScreenable(true)
        assert.equal(w.isFullScreenable(), true)
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
        assert.equal(w.isKiosk(), true)

        w.once('enter-full-screen', () => {
          w.setKiosk(false)
          assert.equal(w.isKiosk(), false)
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
          assert.equal(w.isResizable(), true)
          w.setFullScreen(false)
        })
        w.once('leave-full-screen', () => {
          assert.equal(w.isResizable(), false)
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
          assert.equal(w.isFullScreen(), true)
          w.setFullScreen(false)
        })
        w.once('leave-full-screen', () => {
          assert.equal(w.isFullScreen(), false)
          done()
        })
        w.setFullScreen(true)
      })

      it('should not be changed by setKiosk method', (done) => {
        w.destroy()
        w = new BrowserWindow()
        w.once('enter-full-screen', () => {
          assert.equal(w.isFullScreen(), true)
          w.setKiosk(true)
          w.setKiosk(false)
          assert.equal(w.isFullScreen(), true)
          w.setFullScreen(false)
        })
        w.once('leave-full-screen', () => {
          assert.equal(w.isFullScreen(), false)
          done()
        })
        w.setFullScreen(true)
      })
    })

    describe('closable state', () => {
      it('can be changed with closable option', () => {
        w.destroy()
        w = new BrowserWindow({show: false, closable: false})
        assert.equal(w.isClosable(), false)
      })

      it('can be changed with setClosable method', () => {
        assert.equal(w.isClosable(), true)
        w.setClosable(false)
        assert.equal(w.isClosable(), false)
        w.setClosable(true)
        assert.equal(w.isClosable(), true)
      })
    })

    describe('hasShadow state', () => {
      // On Window there is no shadow by default and it can not be changed
      // dynamically.
      it('can be changed with hasShadow option', () => {
        w.destroy()
        let hasShadow = process.platform !== 'darwin'
        w = new BrowserWindow({show: false, hasShadow: hasShadow})
        assert.equal(w.hasShadow(), hasShadow)
      })

      it('can be changed with setHasShadow method', () => {
        if (process.platform !== 'darwin') return

        assert.equal(w.hasShadow(), true)
        w.setHasShadow(false)
        assert.equal(w.hasShadow(), false)
        w.setHasShadow(true)
        assert.equal(w.hasShadow(), true)
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
          assert.equal(w.isVisible(), true)
          assert.equal(w.isFullScreen(), false)
          done()
        })
        w.show()
      })
      w.loadURL('about:blank')
    })
    it('should keep window hidden if already in hidden state', (done) => {
      w.webContents.once('did-finish-load', () => {
        w.once('leave-full-screen', () => {
          assert.equal(w.isVisible(), false)
          assert.equal(w.isFullScreen(), false)
          done()
        })
        w.setFullScreen(false)
      })
      w.loadURL('about:blank')
    })
  })

  describe('parent window', () => {
    let c = null

    beforeEach(() => {
      if (c != null) c.destroy()
      c = new BrowserWindow({show: false, parent: w})
    })

    afterEach(() => {
      if (c != null) c.destroy()
      c = null
    })

    describe('parent option', () => {
      it('sets parent window', () => {
        assert.equal(c.getParentWindow(), w)
      })
      it('adds window to child windows of parent', () => {
        assert.deepEqual(w.getChildWindows(), [c])
      })
      it('removes from child windows of parent when window is closed', (done) => {
        c.once('closed', () => {
          assert.deepEqual(w.getChildWindows(), [])
          done()
        })
        c.close()
      })

      it('should not affect the show option', () => {
        assert.equal(c.isVisible(), false)
        assert.equal(c.getParentWindow().isVisible(), false)
      })
    })

    describe('win.setParentWindow(parent)', () => {
      before(function () {
        if (process.platform === 'win32') {
          this.skip()
        }
      })

      beforeEach(() => {
        if (c != null) c.destroy()
        c = new BrowserWindow({show: false})
      })

      it('sets parent window', () => {
        assert.equal(w.getParentWindow(), null)
        assert.equal(c.getParentWindow(), null)
        c.setParentWindow(w)
        assert.equal(c.getParentWindow(), w)
        c.setParentWindow(null)
        assert.equal(c.getParentWindow(), null)
      })
      it('adds window to child windows of parent', () => {
        assert.deepEqual(w.getChildWindows(), [])
        c.setParentWindow(w)
        assert.deepEqual(w.getChildWindows(), [c])
        c.setParentWindow(null)
        assert.deepEqual(w.getChildWindows(), [])
      })
      it('removes from child windows of parent when window is closed', (done) => {
        c.once('closed', () => {
          assert.deepEqual(w.getChildWindows(), [])
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
        c = new BrowserWindow({show: false, parent: w, modal: true})
      })

      it('disables parent window', () => {
        assert.equal(w.isEnabled(), true)
        c.show()
        assert.equal(w.isEnabled(), false)
      })
      it('enables parent window when closed', (done) => {
        c.once('closed', () => {
          assert.equal(w.isEnabled(), true)
          done()
        })
        c.show()
        c.close()
      })
      it('disables parent window recursively', () => {
        let c2 = new BrowserWindow({show: false, parent: w, modal: true})
        c.show()
        assert.equal(w.isEnabled(), false)
        c2.show()
        assert.equal(w.isEnabled(), false)
        c.destroy()
        assert.equal(w.isEnabled(), false)
        c2.destroy()
        assert.equal(w.isEnabled(), true)
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

  describe('extensions and dev tools extensions', () => {
    let showPanelTimeoutId

    const showLastDevToolsPanel = () => {
      w.webContents.once('devtools-opened', () => {
        const show = () => {
          if (w == null || w.isDestroyed()) return
          const {devToolsWebContents} = w
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
      beforeEach(() => {
        BrowserWindow.removeDevToolsExtension('foo')
        assert.equal(BrowserWindow.getDevToolsExtensions().hasOwnProperty('foo'), false)

        var extensionPath = path.join(__dirname, 'fixtures', 'devtools-extensions', 'foo')
        BrowserWindow.addDevToolsExtension(extensionPath)
        assert.equal(BrowserWindow.getDevToolsExtensions().hasOwnProperty('foo'), true)

        showLastDevToolsPanel()

        w.loadURL('about:blank')
      })

      it('throws errors for missing manifest.json files', () => {
        assert.throws(() => {
          BrowserWindow.addDevToolsExtension(path.join(__dirname, 'does-not-exist'))
        }, /ENOENT: no such file or directory/)
      })

      it('throws errors for invalid manifest.json files', () => {
        assert.throws(() => {
          BrowserWindow.addDevToolsExtension(path.join(__dirname, 'fixtures', 'devtools-extensions', 'bad-manifest'))
        }, /Unexpected token }/)
      })

      describe('when the devtools is docked', () => {
        it('creates the extension', (done) => {
          w.webContents.openDevTools({mode: 'bottom'})

          ipcMain.once('answer', function (event, message) {
            assert.equal(message.runtimeId, 'foo')
            assert.equal(message.tabId, w.webContents.id)
            assert.equal(message.i18nString, 'foo - bar (baz)')
            assert.deepEqual(message.storageItems, {
              local: {
                set: {hello: 'world', world: 'hello'},
                remove: {world: 'hello'},
                clear: {}
              },
              sync: {
                set: {foo: 'bar', bar: 'foo'},
                remove: {foo: 'bar'},
                clear: {}
              }
            })
            done()
          })
        })
      })

      describe('when the devtools is undocked', () => {
        it('creates the extension', (done) => {
          w.webContents.openDevTools({mode: 'undocked'})

          ipcMain.once('answer', function (event, message, extensionId) {
            assert.equal(message.runtimeId, 'foo')
            assert.equal(message.tabId, w.webContents.id)
            done()
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
          partition: 'temp'
        }
      })

      var extensionPath = path.join(__dirname, 'fixtures', 'devtools-extensions', 'foo')
      BrowserWindow.removeDevToolsExtension('foo')
      BrowserWindow.addDevToolsExtension(extensionPath)

      showLastDevToolsPanel()

      w.loadURL('about:blank')
      w.webContents.openDevTools({mode: 'bottom'})

      ipcMain.once('answer', function (event, message) {
        assert.equal(message.runtimeId, 'foo')
        done()
      })
    })

    it('serializes the registered extensions on quit', () => {
      var extensionName = 'foo'
      var extensionPath = path.join(__dirname, 'fixtures', 'devtools-extensions', extensionName)
      var serializedPath = path.join(app.getPath('userData'), 'DevTools Extensions')

      BrowserWindow.addDevToolsExtension(extensionPath)
      app.emit('will-quit')
      assert.deepEqual(JSON.parse(fs.readFileSync(serializedPath)), [extensionPath])

      BrowserWindow.removeDevToolsExtension(extensionName)
      app.emit('will-quit')
      assert.equal(fs.existsSync(serializedPath), false)
    })

    describe('BrowserWindow.addExtension', () => {
      beforeEach(() => {
        BrowserWindow.removeExtension('foo')
        assert.equal(BrowserWindow.getExtensions().hasOwnProperty('foo'), false)

        var extensionPath = path.join(__dirname, 'fixtures', 'devtools-extensions', 'foo')
        BrowserWindow.addExtension(extensionPath)
        assert.equal(BrowserWindow.getExtensions().hasOwnProperty('foo'), true)

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
      assert.equal(result, 'success')
    })
    it('returns result when calback is provided', (done) => {
      ipcRenderer.send('executeJavaScript', code, true)
      ipcRenderer.once('executeJavaScript-response', function (event, result) {
        assert.equal(result, expected)
        done()
      })
    })
    it('returns result if the code returns an asyncronous promise', (done) => {
      ipcRenderer.send('executeJavaScript', asyncCode, true)
      ipcRenderer.once('executeJavaScript-response', (event, result) => {
        assert.equal(result, expected)
        done()
      })
    })
    it('resolves the returned promise with the result when a callback is specified', (done) => {
      ipcRenderer.send('executeJavaScript', code, true)
      ipcRenderer.once('executeJavaScript-promise-response', (event, result) => {
        assert.equal(result, expected)
        done()
      })
    })
    it('resolves the returned promise with the result when no callback is specified', (done) => {
      ipcRenderer.send('executeJavaScript', code, false)
      ipcRenderer.once('executeJavaScript-promise-response', (event, result) => {
        assert.equal(result, expected)
        done()
      })
    })
    it('resolves the returned promise with the result if the code returns an asyncronous promise', (done) => {
      ipcRenderer.send('executeJavaScript', asyncCode, true)
      ipcRenderer.once('executeJavaScript-promise-response', (event, result) => {
        assert.equal(result, expected)
        done()
      })
    })
    it('rejects the returned promise if an async error is thrown', (done) => {
      ipcRenderer.send('executeJavaScript', badAsyncCode, true)
      ipcRenderer.once('executeJavaScript-promise-error', (event, error) => {
        assert.equal(error, expectedErrorMsg)
        done()
      })
    })
    it('rejects the returned promise with an error if an Error.prototype is thrown', async () => {
      for (const error in errorTypes) {
        await new Promise((resolve) => {
          ipcRenderer.send('executeJavaScript', `Promise.reject(new ${error.name}("Wamp-wamp")`, true)
          ipcRenderer.once('executeJavaScript-promise-error-name', (event, name) => {
            assert.equal(name, error.name)
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
        assert.equal(result, expected)
        done()
      })
      w.loadURL(server.url)
    })
    it('works with result objects that have DOM class prototypes', (done) => {
      w.webContents.executeJavaScript('document.location', (result) => {
        assert.equal(result.origin, server.url)
        assert.equal(result.protocol, 'http:')
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
        typeofFunctionApply: 'function'
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
      if (w != null) w.destroy()
      w = new BrowserWindow({
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
      if (ws != null) ws.destroy()
    })

    it('separates the page context from the Electron/preload context', (done) => {
      ipcMain.once('isolated-world', (event, data) => {
        assert.deepEqual(data, expectedContextData)
        done()
      })
      w.loadURL(`file://${fixtures}/api/isolated.html`)
    })
    it('recreates the contexts on reload', (done) => {
      w.webContents.once('did-finish-load', () => {
        ipcMain.once('isolated-world', (event, data) => {
          assert.deepEqual(data, expectedContextData)
          done()
        })
        w.webContents.reload()
      })
      w.loadURL(`file://${fixtures}/api/isolated.html`)
    })
    it('enables context isolation on child windows', (done) => {
      app.once('browser-window-created', (event, window) => {
        assert.equal(window.webContents.getWebPreferences().contextIsolation, true)
        done()
      })
      w.loadURL(`file://${fixtures}/pages/window-open.html`)
    })
    it('separates the page context from the Electron/preload context with sandbox on', (done) => {
      ipcMain.once('isolated-sandbox-world', (event, data) => {
        assert.deepEqual(data, expectedContextData)
        done()
      })
      w.loadURL(`file://${fixtures}/api/isolated.html`)
    })
    it('recreates the contexts on reload with sandbox on', (done) => {
      w.webContents.once('did-finish-load', () => {
        ipcMain.once('isolated-sandbox-world', (event, data) => {
          assert.deepEqual(data, expectedContextData)
          done()
        })
        w.webContents.reload()
      })
      w.loadURL(`file://${fixtures}/api/isolated.html`)
    })
  })

  describe('offscreen rendering', () => {
    const isOffscreenRenderingDisabled = () => {
      const contents = webContents.create({})
      const disabled = typeof contents.isOffscreen !== 'function'
      contents.destroy()
      return disabled
    }

    // Offscreen rendering can be disabled in the build
    if (isOffscreenRenderingDisabled()) return

    beforeEach(() => {
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
        assert.notEqual(data.length, 0)
        let size = data.getSize()
        assertWithinDelta(size.width, 100, 2, 'width')
        assertWithinDelta(size.height, 100, 2, 'height')
        done()
      })
      w.loadURL('file://' + fixtures + '/api/offscreen-rendering.html')
    })

    describe('window.webContents.isOffscreen()', () => {
      it('is true for offscreen type', () => {
        w.loadURL('file://' + fixtures + '/api/offscreen-rendering.html')
        assert.equal(w.webContents.isOffscreen(), true)
      })

      it('is false for regular window', () => {
        let c = new BrowserWindow({show: false})
        assert.equal(c.webContents.isOffscreen(), false)
        c.destroy()
      })
    })

    describe('window.webContents.isPainting()', () => {
      it('returns whether is currently painting', (done) => {
        w.webContents.once('paint', function (event, rect, data) {
          assert.equal(w.webContents.isPainting(), true)
          done()
        })
        w.loadURL('file://' + fixtures + '/api/offscreen-rendering.html')
      })
    })

    describe('window.webContents.stopPainting()', () => {
      it('stops painting', (done) => {
        w.webContents.on('dom-ready', () => {
          w.webContents.stopPainting()
          assert.equal(w.webContents.isPainting(), false)
          done()
        })
        w.loadURL('file://' + fixtures + '/api/offscreen-rendering.html')
      })
    })

    describe('window.webContents.startPainting()', () => {
      it('starts painting', (done) => {
        w.webContents.on('dom-ready', () => {
          w.webContents.stopPainting()
          w.webContents.startPainting()
          w.webContents.once('paint', function (event, rect, data) {
            assert.equal(w.webContents.isPainting(), true)
            done()
          })
        })
        w.loadURL('file://' + fixtures + '/api/offscreen-rendering.html')
      })
    })

    describe('window.webContents.getFrameRate()', () => {
      it('has default frame rate', (done) => {
        w.webContents.once('paint', function (event, rect, data) {
          assert.equal(w.webContents.getFrameRate(), 60)
          done()
        })
        w.loadURL('file://' + fixtures + '/api/offscreen-rendering.html')
      })
    })

    describe('window.webContents.setFrameRate(frameRate)', () => {
      it('sets custom frame rate', (done) => {
        w.webContents.on('dom-ready', () => {
          w.webContents.setFrameRate(30)
          w.webContents.once('paint', function (event, rect, data) {
            assert.equal(w.webContents.getFrameRate(), 30)
            done()
          })
        })
        w.loadURL('file://' + fixtures + '/api/offscreen-rendering.html')
      })
    })
  })
})

const assertBoundsEqual = (actual, expect) => {
  if (!isScaleFactorRounding()) {
    assert.deepEqual(expect, actual)
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
  const {scaleFactor} = screen.getPrimaryDisplay()
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

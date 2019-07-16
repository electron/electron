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

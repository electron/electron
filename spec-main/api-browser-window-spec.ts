import * as chai from 'chai'
import * as chaiAsPromised from 'chai-as-promised'
import * as path from 'path'
import * as fs from 'fs'
import * as os from 'os'
import * as qs from 'querystring'
import * as http from 'http'
import { AddressInfo } from 'net'
import { app, BrowserWindow, BrowserView, ipcMain, OnBeforeSendHeadersListenerDetails, protocol, screen, webContents, session, WebContents } from 'electron'
import { emittedOnce } from './events-helpers';
import { closeWindow } from './window-helpers';
const { expect } = chai

const ifit = (condition: boolean) => (condition ? it : it.skip)
const ifdescribe = (condition: boolean) => (condition ? describe : describe.skip)

chai.use(chaiAsPromised)

const fixtures = path.resolve(__dirname, '..', 'spec', 'fixtures')

// Is the display's scale factor possibly causing rounding of pixel coordinate
// values?
const isScaleFactorRounding = () => {
  const { scaleFactor } = screen.getPrimaryDisplay()
  // Return true if scale factor is non-integer value
  if (Math.round(scaleFactor) !== scaleFactor) return true
  // Return true if scale factor is odd number above 2
  return scaleFactor > 2 && scaleFactor % 2 === 1
}

const expectBoundsEqual = (actual: any, expected: any) => {
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

const closeAllWindows = async () => {
  for (const w of BrowserWindow.getAllWindows()) {
    await closeWindow(w, {assertNotWindows: false})
  }
}

describe('BrowserWindow module', () => {
  describe('BrowserWindow constructor', () => {
    it('allows passing void 0 as the webContents', async () => {
      expect(() => {
        const w = new BrowserWindow({
          show: false,
          // apparently void 0 had different behaviour from undefined in the
          // issue that this test is supposed to catch.
          webContents: void 0
        } as any)
        w.destroy()
      }).not.to.throw()
    })
  })

  describe('BrowserWindow.close()', () => {
    let w = null as unknown as BrowserWindow
    beforeEach(() => {
      w = new BrowserWindow({show: false, webPreferences: {nodeIntegration: true}})
    })
    afterEach(async () => {
      await closeWindow(w)
      w = null as unknown as BrowserWindow
    })

    it('should emit unload handler', async () => {
      await w.loadFile(path.join(fixtures, 'api', 'unload.html'))
      const closed = emittedOnce(w, 'closed')
      w.close()
      await closed
      const test = path.join(fixtures, 'api', 'unload')
      const content = fs.readFileSync(test)
      fs.unlinkSync(test)
      expect(String(content)).to.equal('unload')
    })
    it('should emit beforeunload handler', async () => {
      await w.loadFile(path.join(fixtures, 'api', 'beforeunload-false.html'))
      const beforeunload = emittedOnce(w, 'onbeforeunload')
      w.close()
      await beforeunload
    })

    describe('when invoked synchronously inside navigation observer', () => {
      let server: http.Server = null as unknown as http.Server
      let url: string = null as unknown as string

      before((done) => {
        server = http.createServer((request, response) => {
          switch (request.url) {
            case '/net-error':
              response.destroy()
              break
            case '/301':
              response.statusCode = 301
              response.setHeader('Location', '/200')
              response.end()
              break
            case '/200':
              response.statusCode = 200
              response.end('hello')
              break
            case '/title':
              response.statusCode = 200
              response.end('<title>Hello</title>')
              break
            default:
              throw new Error(`unsupported endpoint: ${request.url}`)
          }
        }).listen(0, '127.0.0.1', () => {
          url = 'http://127.0.0.1:' + (server.address() as AddressInfo).port
          done()
        })
      })

      after(() => {
        server.close()
      })

      const events = [
        { name: 'did-start-loading', path: '/200' },
        { name: 'dom-ready', path: '/200' },
        { name: 'page-title-updated', path: '/title' },
        { name: 'did-stop-loading', path: '/200' },
        { name: 'did-finish-load', path: '/200' },
        { name: 'did-frame-finish-load', path: '/200' },
        { name: 'did-fail-load', path: '/net-error' }
      ]

      for (const {name, path} of events) {
        it(`should not crash when closed during ${name}`, async () => {
          const w = new BrowserWindow({ show: false })
          w.webContents.once((name as any), () => {
            w.close()
          })
          const destroyed = emittedOnce(w.webContents, 'destroyed')
          w.webContents.loadURL(url + path)
          await destroyed
        })
      }
    })
  })

  describe('window.close()', () => {
    let w = null as unknown as BrowserWindow
    beforeEach(() => {
      w = new BrowserWindow({show: false, webPreferences: {nodeIntegration: true}})
    })
    afterEach(async () => {
      await closeWindow(w)
      w = null as unknown as BrowserWindow
    })

    it('should emit unload event', async () => {
      w.loadFile(path.join(fixtures, 'api', 'close.html'))
      await emittedOnce(w, 'closed')
      const test = path.join(fixtures, 'api', 'close')
      const content = fs.readFileSync(test).toString()
      fs.unlinkSync(test)
      expect(content).to.equal('close')
    })
    it('should emit beforeunload event', async () => {
      w.loadFile(path.join(fixtures, 'api', 'close-beforeunload-false.html'))
      await emittedOnce(w, 'onbeforeunload')
    })
  })

  describe('BrowserWindow.destroy()', () => {
    let w = null as unknown as BrowserWindow
    beforeEach(() => {
      w = new BrowserWindow({show: false, webPreferences: {nodeIntegration: true}})
    })
    afterEach(async () => {
      await closeWindow(w)
      w = null as unknown as BrowserWindow
    })

    it('prevents users to access methods of webContents', async () => {
      const contents = w.webContents
      w.destroy()
      await new Promise(setImmediate)
      expect(() => {
        contents.getProcessId()
      }).to.throw('Object has been destroyed')
    })
    it('should not crash when destroying windows with pending events', () => {
      const focusListener = () => {}
      app.on('browser-window-focus', focusListener)
      const windowCount = 3
      const windowOptions = {
        show: false,
        width: 400,
        height: 400,
        webPreferences: {
          backgroundThrottling: false
        }
      }
      const windows = Array.from(Array(windowCount)).map(x => new BrowserWindow(windowOptions))
      windows.forEach(win => win.show())
      windows.forEach(win => win.focus())
      windows.forEach(win => win.destroy())
      app.removeListener('browser-window-focus', focusListener)
    })
  })

  describe('BrowserWindow.loadURL(url)', () => {
    let w = null as unknown as BrowserWindow
    const scheme = 'other'
    const srcPath = path.join(fixtures, 'api', 'loaded-from-dataurl.js')
    before((done) => {
      protocol.registerFileProtocol(scheme, (request, callback) => {
        callback(srcPath)
      }, (error) => done(error))
    })

    after(() => {
      protocol.unregisterProtocol(scheme)
    })

    beforeEach(() => {
      w = new BrowserWindow({show: false, webPreferences: {nodeIntegration: true}})
    })
    afterEach(async () => {
      await closeWindow(w)
      w = null as unknown as BrowserWindow
    })
    let server = null as unknown as http.Server
    let url = null as unknown as string
    let postData = null as any
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
          } else {
            res.end()
          }
        }
        setTimeout(respond, req.url && req.url.includes('slow') ? 200 : 0)
      })
      server.listen(0, '127.0.0.1', () => {
        url = `http://127.0.0.1:${(server.address() as AddressInfo).port}`
        done()
      })
    })

    after(() => {
      server.close()
    })

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
        expect(code).to.equal(-6)
        expect(desc).to.equal('ERR_FILE_NOT_FOUND')
        expect(isMainFrame).to.equal(true)
        done()
      })
      w.loadURL('file://a.txt')
    })
    it('should emit did-fail-load event for invalid URL', (done) => {
      w.webContents.on('did-fail-load', (event, code, desc, url, isMainFrame) => {
        expect(desc).to.equal('ERR_INVALID_URL')
        expect(code).to.equal(-300)
        expect(isMainFrame).to.equal(true)
        done()
      })
      w.loadURL('http://example:port')
    })
    it('should set `mainFrame = false` on did-fail-load events in iframes', (done) => {
      w.webContents.on('did-fail-load', (event, code, desc, url, isMainFrame) => {
        expect(isMainFrame).to.equal(false)
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
        expect(desc).to.equal('ERR_INVALID_URL')
        expect(code).to.equal(-300)
        expect(isMainFrame).to.equal(true)
        done()
      })
      const data = Buffer.alloc(2 * 1024 * 1024).toString('base64')
      w.loadURL(`data:image/png;base64,${data}`)
    })

    it('should return a promise', () => {
      const p = w.loadURL('about:blank')
      expect(p).to.have.property('then')
    })

    it('should return a promise that resolves', async () => {
      expect(w.loadURL('about:blank')).to.eventually.be.fulfilled
    })

    it('should return a promise that rejects on a load failure', async () => {
      const data = Buffer.alloc(2 * 1024 * 1024).toString('base64')
      const p = w.loadURL(`data:image/png;base64,${data}`)
      await expect(p).to.eventually.be.rejected
    })

    it('should return a promise that resolves even if pushState occurs during navigation', async () => {
      const p = w.loadURL('data:text/html,<script>window.history.pushState({}, "/foo")</script>')
      await expect(p).to.eventually.be.fulfilled
    })

    describe('POST navigations', () => {
      afterEach(() => { w.webContents.session.webRequest.onBeforeSendHeaders(null) })

      it('supports specifying POST data', async () => {
        await w.loadURL(url, { postData })
      })
      it('sets the content type header on URL encoded forms', async () => {
        await w.loadURL(url)
        const requestDetails: Promise<OnBeforeSendHeadersListenerDetails> = new Promise(resolve => {
          w.webContents.session.webRequest.onBeforeSendHeaders((details, callback) => {
            resolve(details)
          })
        })
        w.webContents.executeJavaScript(`
          form = document.createElement('form')
          document.body.appendChild(form)
          form.method = 'POST'
          form.submit()
        `)
        const details = await requestDetails
        expect(details.requestHeaders['Content-Type']).to.equal('application/x-www-form-urlencoded')
      })
      it('sets the content type header on multi part forms', async () => {
        await w.loadURL(url)
        const requestDetails: Promise<OnBeforeSendHeadersListenerDetails> = new Promise(resolve => {
          w.webContents.session.webRequest.onBeforeSendHeaders((details, callback) => {
            resolve(details)
          })
        })
        w.webContents.executeJavaScript(`
          form = document.createElement('form')
          document.body.appendChild(form)
          form.method = 'POST'
          form.enctype = 'multipart/form-data'
          file = document.createElement('input')
          file.type = 'file'
          file.name = 'file'
          form.appendChild(file)
          form.submit()
        `)
        const details = await requestDetails
        expect(details.requestHeaders['Content-Type'].startsWith('multipart/form-data; boundary=----WebKitFormBoundary')).to.equal(true)
      })
    })

    it('should support support base url for data urls', (done) => {
      ipcMain.once('answer', (event, test) => {
        expect(test).to.equal('test')
        done()
      })
      w.loadURL('data:text/html,<script src="loaded-from-dataurl.js"></script>', { baseURLForDataURL: `other://${path.join(fixtures, 'api')}${path.sep}` })
    })
  })

  describe('navigation events', () => {
    let w = null as unknown as BrowserWindow
    beforeEach(() => {
      w = new BrowserWindow({show: false, webPreferences: {nodeIntegration: true}})
    })
    afterEach(async () => {
      await closeWindow(w)
      w = null as unknown as BrowserWindow
    })

    describe('will-navigate event', () => {
      it('allows the window to be closed from the event listener', (done) => {
        w.webContents.once('will-navigate', () => {
          w.close()
          done()
        })
        w.loadFile(path.join(fixtures, 'pages', 'will-navigate.html'))
      })
    })

    describe('will-redirect event', () => {
      let server = null as unknown as http.Server
      let url = null as unknown as string
      before((done) => {
        server = http.createServer((req, res) => {
          if (req.url === '/302') {
            res.setHeader('Location', '/200')
            res.statusCode = 302
            res.end()
          } else if (req.url === '/navigate-302') {
            res.end(`<html><body><script>window.location='${url}/302'</script></body></html>`)
          } else {
            res.end()
          }
        })
        server.listen(0, '127.0.0.1', () => {
          url = `http://127.0.0.1:${(server.address() as AddressInfo).port}`
          done()
        })
      })

      after(() => {
        server.close()
      })
      it('is emitted on redirects', (done) => {
        w.webContents.on('will-redirect', (event, url) => {
          done()
        })
        w.loadURL(`${url}/302`)
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
        w.loadURL(`${url}/navigate-302`)
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
        w.loadURL(`${url}/302`)
      })

      it('allows the window to be closed from the event listener', (done) => {
        w.webContents.once('will-redirect', (event, input) => {
          w.close()
          done()
        })
        w.loadURL(`${url}/302`)
      })

      it('can be prevented', (done) => {
        w.webContents.once('will-redirect', (event) => {
          event.preventDefault()
        })
        w.webContents.on('will-navigate', (e, u) => {
          expect(u).to.equal(`${url}/302`)
        })
        w.webContents.on('did-stop-loading', () => {
          expect(w.webContents.getURL()).to.equal(
            `${url}/navigate-302`,
            'url should not have changed after navigation event'
          )
          done()
        })
        w.webContents.on('will-redirect', (e, u) => {
          expect(u).to.equal(`${url}/200`)
        })
        w.loadURL(`${url}/navigate-302`)
      })
    })
  })

  describe('focus and visibility', () => {
    let w = null as unknown as BrowserWindow
    beforeEach(() => {
      w = new BrowserWindow({show: false})
    })
    afterEach(async () => {
      await closeWindow(w)
      w = null as unknown as BrowserWindow
    })

    describe('BrowserWindow.show()', () => {
      it('should focus on window', () => {
        w.show()
        if (process.platform === 'darwin' && !isCI) {
          // on CI, the Electron window will be the only one open, so it'll get
          // focus. on not-CI, some other window will have focus, and we don't
          // steal focus any more, so we expect isFocused to be false.
          expect(w.isFocused()).to.equal(false)
        } else {
          expect(w.isFocused()).to.equal(true)
        }
      })
      it('should make the window visible', () => {
        w.show()
        expect(w.isVisible()).to.equal(true)
      })
      it('emits when window is shown', (done) => {
        w.once('show', () => {
          expect(w.isVisible()).to.equal(true)
          done()
        })
        w.show()
      })
    })

    describe('BrowserWindow.hide()', () => {
      it('should defocus on window', () => {
        w.hide()
        expect(w.isFocused()).to.equal(false)
      })
      it('should make the window not visible', () => {
        w.show()
        w.hide()
        expect(w.isVisible()).to.equal(false)
      })
      it('emits when window is hidden', async () => {
        const shown = emittedOnce(w, 'show')
        w.show()
        await shown
        const hidden = emittedOnce(w, 'hide')
        w.hide()
        await hidden
        expect(w.isVisible()).to.equal(false)
      })
    })

    describe('BrowserWindow.showInactive()', () => {
      it('should not focus on window', () => {
        w.showInactive()
        expect(w.isFocused()).to.equal(false)
      })
    })

    describe('BrowserWindow.focus()', () => {
      it('does not make the window become visible', () => {
        expect(w.isVisible()).to.equal(false)
        w.focus()
        expect(w.isVisible()).to.equal(false)
      })
    })

    describe('BrowserWindow.blur()', () => {
      it('removes focus from window', () => {
        w.blur()
        expect(w.isFocused()).to.equal(false)
      })
    })

    describe('BrowserWindow.getFocusedWindow()', () => {
      it('returns the opener window when dev tools window is focused', (done) => {
        w.show()
        w.webContents.once('devtools-focused', () => {
          expect(BrowserWindow.getFocusedWindow()).to.equal(w)
          done()
        })
        w.webContents.openDevTools({ mode: 'undocked' })
      })
    })

    describe('BrowserWindow.moveTop()', () => {
      it('should not steal focus', async () => {
        const posDelta = 50
        const wShownInactive = emittedOnce(w, 'show')
        w.showInactive()
        await wShownInactive
        expect(w.isFocused()).to.equal(false)

        const otherWindow = new BrowserWindow({ show: false, title: 'otherWindow' })
        const otherWindowShown = emittedOnce(otherWindow, 'show')
        const otherWindowFocused = emittedOnce(otherWindow, 'focus')
        otherWindow.show()
        await otherWindowShown
        await otherWindowFocused
        expect(otherWindow.isFocused()).to.equal(true)

        w.moveTop()
        const wPos = w.getPosition()
        const wMoving = emittedOnce(w, 'move')
        w.setPosition(wPos[0] + posDelta, wPos[1] + posDelta)
        await wMoving
        expect(w.isFocused()).to.equal(false)
        expect(otherWindow.isFocused()).to.equal(true)

        const wFocused = emittedOnce(w, 'focus')
        w.focus()
        await wFocused
        expect(w.isFocused()).to.equal(true)

        otherWindow.moveTop()
        const otherWindowPos = otherWindow.getPosition()
        const otherWindowMoving = emittedOnce(otherWindow, 'move')
        otherWindow.setPosition(otherWindowPos[0] + posDelta, otherWindowPos[1] + posDelta)
        await otherWindowMoving
        expect(otherWindow.isFocused()).to.equal(false)
        expect(w.isFocused()).to.equal(true)

        await closeWindow(otherWindow, { assertNotWindows: false })
        expect(BrowserWindow.getAllWindows()).to.have.lengthOf(1)
      })
    })

  })

  describe('sizing', () => {
    let w = null as unknown as BrowserWindow
    beforeEach(() => {
      w = new BrowserWindow({show: false, width: 400, height: 400})
    })
    afterEach(async () => {
      await closeWindow(w)
      w = null as unknown as BrowserWindow
    })

    describe('BrowserWindow.setBounds(bounds[, animate])', () => {
      it('sets the window bounds with full bounds', () => {
        const fullBounds = { x: 440, y: 225, width: 500, height: 400 }
        w.setBounds(fullBounds)

        expectBoundsEqual(w.getBounds(), fullBounds)
      })

      it('sets the window bounds with partial bounds', () => {
        const fullBounds = { x: 440, y: 225, width: 500, height: 400 }
        w.setBounds(fullBounds)

        const boundsUpdate = { width: 200 }
        w.setBounds(boundsUpdate as any)

        const expectedBounds = Object.assign(fullBounds, boundsUpdate)
        expectBoundsEqual(w.getBounds(), expectedBounds)
      })
    })

    describe('BrowserWindow.setSize(width, height)', () => {
      it('sets the window size', async () => {
        const size = [300, 400]

        const resized = emittedOnce(w, 'resize')
        w.setSize(size[0], size[1])
        await resized

        expectBoundsEqual(w.getSize(), size)
      })
    })

    describe('BrowserWindow.setMinimum/MaximumSize(width, height)', () => {
      it('sets the maximum and minimum size of the window', () => {
        expect(w.getMinimumSize()).to.deep.equal([0, 0])
        expect(w.getMaximumSize()).to.deep.equal([0, 0])

        w.setMinimumSize(100, 100)
        expectBoundsEqual(w.getMinimumSize(), [100, 100])
        expectBoundsEqual(w.getMaximumSize(), [0, 0])

        w.setMaximumSize(900, 600)
        expectBoundsEqual(w.getMinimumSize(), [100, 100])
        expectBoundsEqual(w.getMaximumSize(), [900, 600])
      })
    })

    describe('BrowserWindow.setAspectRatio(ratio)', () => {
      it('resets the behaviour when passing in 0', (done) => {
        const size = [300, 400]
        w.setAspectRatio(1 / 2)
        w.setAspectRatio(0)
        w.once('resize', () => {
          expectBoundsEqual(w.getSize(), size)
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
          expect(newPos).to.deep.equal(pos)
          done()
        })
        w.setPosition(pos[0], pos[1])
      })
    })

    describe('BrowserWindow.setContentSize(width, height)', () => {
      it('sets the content size', (done) => {
        // NB. The CI server has a very small screen. Attempting to size the window
        // larger than the screen will limit the window's size to the screen and
        // cause the test to fail.
        const size = [456, 567]
        w.setContentSize(size[0], size[1])
        setImmediate(() => {
          const after = w.getContentSize()
          expect(after).to.deep.equal(size)
          done()
        })
      })
      it('works for a frameless window', (done) => {
        w.destroy()
        w = new BrowserWindow({
          show: false,
          frame: false,
          width: 400,
          height: 400
        })
        const size = [456, 567]
        w.setContentSize(size[0], size[1])
        setImmediate(() => {
          const after = w.getContentSize()
          expect(after).to.deep.equal(size)
          done()
        })
      })
    })

    describe('BrowserWindow.setContentBounds(bounds)', () => {
      it('sets the content size and position', (done) => {
        const bounds = { x: 10, y: 10, width: 250, height: 250 }
        w.once('resize', () => {
          setTimeout(() => {
            expectBoundsEqual(w.getContentBounds(), bounds)
            done()
          })
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
          setTimeout(() => {
            expect(w.getContentBounds()).to.deep.equal(bounds)
            done()
          })
        })
        w.setContentBounds(bounds)
      })
    })

    describe(`BrowserWindow.getNormalBounds()`, () => {
      describe(`Normal state`, () => {
        it(`checks normal bounds after resize`, (done) => {
          const size = [300, 400]
          w.once('resize', () => {
            expectBoundsEqual(w.getNormalBounds(), w.getBounds())
            done()
          })
          w.setSize(size[0], size[1])
        })
        it(`checks normal bounds after move`, (done) => {
          const pos = [10, 10]
          w.once('move', () => {
            expectBoundsEqual(w.getNormalBounds(), w.getBounds())
            done()
          })
          w.setPosition(pos[0], pos[1])
        })
      })
      ifdescribe(process.platform !== 'linux')(`Maximized state`, () => {
        it(`checks normal bounds when maximized`, (done) => {
          const bounds = w.getBounds()
          w.once('maximize', () => {
            expectBoundsEqual(w.getNormalBounds(), bounds)
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
            expectBoundsEqual(w.getNormalBounds(), bounds)
            done()
          })
          w.show()
          w.maximize()
        })
      })
      ifdescribe(process.platform !== 'linux')(`Minimized state`, () => {
        it(`checks normal bounds when minimized`, (done) => {
          const bounds = w.getBounds()
          w.once('minimize', () => {
            expectBoundsEqual(w.getNormalBounds(), bounds)
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
            expectBoundsEqual(w.getNormalBounds(), bounds)
            done()
          })
          w.show()
          w.minimize()
        })
      })
      ifdescribe(process.platform === 'win32')(`Fullscreen state`, () => {
        it(`checks normal bounds when fullscreen'ed`, (done) => {
          const bounds = w.getBounds()
          w.once('enter-full-screen', () => {
            expectBoundsEqual(w.getNormalBounds(), bounds)
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
            expectBoundsEqual(w.getNormalBounds(), bounds)
            done()
          })
          w.show()
          w.setFullScreen(true)
        })
      })
    })
  })

  ifdescribe(process.platform === 'darwin')('tabbed windows', () => {
    let w = null as unknown as BrowserWindow
    beforeEach(() => {
      w = new BrowserWindow({show: false})
    })
    afterEach(async () => {
      await closeWindow(w)
      w = null as unknown as BrowserWindow
    })

    describe('BrowserWindow.selectPreviousTab()', () => {
      it('does not throw', () => {
        expect(() => {
          w.selectPreviousTab()
        }).to.not.throw()
      })
    })

    describe('BrowserWindow.selectNextTab()', () => {
      it('does not throw', () => {
        expect(() => {
          w.selectNextTab()
        }).to.not.throw()
      })
    })

    describe('BrowserWindow.mergeAllWindows()', () => {
      it('does not throw', () => {
        expect(() => {
          w.mergeAllWindows()
        }).to.not.throw()
      })
    })

    describe('BrowserWindow.moveTabToNewWindow()', () => {
      it('does not throw', () => {
        expect(() => {
          w.moveTabToNewWindow()
        }).to.not.throw()
      })
    })

    describe('BrowserWindow.toggleTabBar()', () => {
      it('does not throw', () => {
        expect(() => {
          w.toggleTabBar()
        }).to.not.throw()
      })
    })

    describe('BrowserWindow.addTabbedWindow()', () => {
      it('does not throw', async () => {
        const tabbedWindow = new BrowserWindow({})
        expect(() => {
          w.addTabbedWindow(tabbedWindow)
        }).to.not.throw()

        expect(BrowserWindow.getAllWindows()).to.have.lengthOf(2) // w + tabbedWindow

        await closeWindow(tabbedWindow, { assertNotWindows: false })
        expect(BrowserWindow.getAllWindows()).to.have.lengthOf(1) // w
      })

      it('throws when called on itself', () => {
        expect(() => {
          w.addTabbedWindow(w)
        }).to.throw('AddTabbedWindow cannot be called by a window on itself.')
      })
    })
  })

  describe('autoHideMenuBar property', () => {
    afterEach(closeAllWindows)
    it('exists', () => {
      const w = new BrowserWindow({show: false})
      expect(w).to.have.property('autoHideMenuBar')

      // TODO(codebytere): remove when propertyification is complete
      expect(w.setAutoHideMenuBar).to.be.a('function')
      expect(w.isMenuBarAutoHide).to.be.a('function')
    })
  })

  describe('BrowserWindow.capturePage(rect)', () => {
    afterEach(closeAllWindows)

    it('returns a Promise with a Buffer', async () => {
      const w = new BrowserWindow({show: false})
      const image = await w.capturePage({
        x: 0,
        y: 0,
        width: 100,
        height: 100
      })

      expect(image.isEmpty()).to.equal(true)
    })

    it('preserves transparency', async () => {
      const w = new BrowserWindow({show: false, transparent: true})
      w.loadURL('about:blank')
      await emittedOnce(w, 'ready-to-show')
      w.show()

      const image = await w.capturePage()
      const imgBuffer = image.toPNG()

      // Check the 25th byte in the PNG.
      // Values can be 0,2,3,4, or 6. We want 6, which is RGB + Alpha
      expect(imgBuffer[25]).to.equal(6)
    })
  })

  describe('BrowserWindow.setProgressBar(progress)', () => {
    let w = null as unknown as BrowserWindow
    before(() => {
      w = new BrowserWindow({show: false})
    })
    after(async () => {
      await closeWindow(w)
      w = null as unknown as BrowserWindow
    })
    it('sets the progress', () => {
      expect(() => {
        if (process.platform === 'darwin') {
          app.dock.setIcon(path.join(fixtures, 'assets', 'logo.png'))
        }
        w.setProgressBar(0.5)

        if (process.platform === 'darwin') {
          app.dock.setIcon(null as any)
        }
        w.setProgressBar(-1)
      }).to.not.throw()
    })
    it('sets the progress using "paused" mode', () => {
      expect(() => {
        w.setProgressBar(0.5, { mode: 'paused' })
      }).to.not.throw()
    })
    it('sets the progress using "error" mode', () => {
      expect(() => {
        w.setProgressBar(0.5, { mode: 'error' })
      }).to.not.throw()
    })
    it('sets the progress using "normal" mode', () => {
      expect(() => {
        w.setProgressBar(0.5, { mode: 'normal' })
      }).to.not.throw()
    })
  })

  describe('BrowserWindow.setAlwaysOnTop(flag, level)', () => {
    let w = null as unknown as BrowserWindow
    beforeEach(() => {
      w = new BrowserWindow({show: false})
    })
    afterEach(async () => {
      await closeWindow(w)
      w = null as unknown as BrowserWindow
    })

    it('sets the window as always on top', () => {
      expect(w.isAlwaysOnTop()).to.equal(false)
      w.setAlwaysOnTop(true, 'screen-saver')
      expect(w.isAlwaysOnTop()).to.equal(true)
      w.setAlwaysOnTop(false)
      expect(w.isAlwaysOnTop()).to.equal(false)
      w.setAlwaysOnTop(true)
      expect(w.isAlwaysOnTop()).to.equal(true)
    })

    ifit(process.platform === 'darwin')('raises an error when relativeLevel is out of bounds', () => {
      expect(() => {
        w.setAlwaysOnTop(true, 'normal', -2147483644)
      }).to.throw()

      expect(() => {
        w.setAlwaysOnTop(true, 'normal', 2147483632)
      }).to.throw()
    })

    ifit(process.platform === 'darwin')('resets the windows level on minimize', () => {
      expect(w.isAlwaysOnTop()).to.equal(false)
      w.setAlwaysOnTop(true, 'screen-saver')
      expect(w.isAlwaysOnTop()).to.equal(true)
      w.minimize()
      expect(w.isAlwaysOnTop()).to.equal(false)
      w.restore()
      expect(w.isAlwaysOnTop()).to.equal(true)
    })
  })

  describe('BrowserWindow.setAutoHideCursor(autoHide)', () => {
    let w = null as unknown as BrowserWindow
    beforeEach(() => {
      w = new BrowserWindow({show: false})
    })
    afterEach(async () => {
      await closeWindow(w)
      w = null as unknown as BrowserWindow
    })

    ifit(process.platform === 'darwin')('on macOS', () => {
      it('allows changing cursor auto-hiding', () => {
        expect(() => {
          w.setAutoHideCursor(false)
          w.setAutoHideCursor(true)
        }).to.not.throw()
      })
    })

    ifit(process.platform !== 'darwin')('on non-macOS platforms', () => {
      it('is not available', () => {
        expect(w.setAutoHideCursor).to.be.undefined('setAutoHideCursor function')
      })
    })
  })

  ifdescribe(process.platform === 'darwin')('BrowserWindow.setWindowButtonVisibility()', () => {
    afterEach(closeAllWindows)

    it('does not throw', () => {
      const w = new BrowserWindow({show: false})
      expect(() => {
        w.setWindowButtonVisibility(true)
        w.setWindowButtonVisibility(false)
      }).to.not.throw()
    })

    it('throws with custom title bar buttons', () => {
      expect(() => {
        const w = new BrowserWindow({
          show: false,
          titleBarStyle: 'customButtonsOnHover',
          frame: false
        })
        w.setWindowButtonVisibility(true)
      }).to.throw('Not supported for this window')
    })
  })

  ifdescribe(process.platform === 'darwin')('BrowserWindow.setVibrancy(type)', () => {
    afterEach(closeAllWindows)

    it('allows setting, changing, and removing the vibrancy', () => {
      const w = new BrowserWindow({show: false})
      expect(() => {
        w.setVibrancy('light')
        w.setVibrancy('dark')
        w.setVibrancy(null)
        w.setVibrancy('ultra-dark')
        w.setVibrancy('' as any)
      }).to.not.throw()
    })
  })

  ifdescribe(process.platform === 'win32')('BrowserWindow.setAppDetails(options)', () => {
    afterEach(closeAllWindows)

    it('supports setting the app details', () => {
      const w = new BrowserWindow({show: false})
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
        (w.setAppDetails as any)()
      }).to.throw('Insufficient number of arguments.')
    })
  })

  describe('BrowserWindow.fromId(id)', () => {
    afterEach(closeAllWindows)
    it('returns the window with id', () => {
      const w = new BrowserWindow({show: false})
      expect(BrowserWindow.fromId(w.id).id).to.equal(w.id)
    })
  })

  describe('BrowserWindow.fromWebContents(webContents)', () => {
    afterEach(closeAllWindows)

    it('returns the window with the webContents', () => {
      const w = new BrowserWindow({show: false})
      const found = BrowserWindow.fromWebContents(w.webContents)
      expect(found.id).to.equal(w.id)
    })

    it('returns undefined for webContents without a BrowserWindow', () => {
      const contents = (webContents as any).create({})
      try {
        expect(BrowserWindow.fromWebContents(contents)).to.be.undefined('BrowserWindow.fromWebContents(contents)')
      } finally {
        contents.destroy()
      }
    })
  })

  describe('BrowserWindow.openDevTools()', () => {
    afterEach(closeAllWindows)
    it('does not crash for frameless window', () => {
      const w = new BrowserWindow({ show: false, frame: false })
      w.webContents.openDevTools()
    })
  })

  describe('BrowserWindow.fromBrowserView(browserView)', () => {
    afterEach(closeAllWindows)

    it('returns the window with the browserView', () => {
      const w = new BrowserWindow({ show: false })
      const bv = new BrowserView
      w.setBrowserView(bv)
      expect(BrowserWindow.fromBrowserView(bv)!.id).to.equal(w.id)
    })

    it('returns undefined if not attached', () => {
      const bv = new BrowserView
      expect(BrowserWindow.fromBrowserView(bv)).to.be.null('BrowserWindow associated with bv')
    })
  })

  describe('BrowserWindow.setOpacity(opacity)', () => {
    afterEach(closeAllWindows)
    it('make window with initial opacity', () => {
      const w = new BrowserWindow({ show: false, opacity: 0.5 })
      expect(w.getOpacity()).to.equal(0.5)
    })
    it('allows setting the opacity', () => {
      const w = new BrowserWindow({ show: false })
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
    afterEach(closeAllWindows)
    it('allows setting shape', () => {
      const w = new BrowserWindow({ show: false })
      expect(() => {
        w.setShape([])
        w.setShape([{ x: 0, y: 0, width: 100, height: 100 }])
        w.setShape([{ x: 0, y: 0, width: 100, height: 100 }, { x: 0, y: 200, width: 1000, height: 100 }])
        w.setShape([])
      }).to.not.throw()
    })
  })

  describe('"useContentSize" option', () => {
    afterEach(closeAllWindows)
    it('make window created with content size when used', () => {
      const w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        useContentSize: true
      })
      const contentSize = w.getContentSize()
      expect(contentSize).to.deep.equal([400, 400])
    })
    it('make window created with window size when not used', () => {
      const w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
      })
      const size = w.getSize()
      expect(size).to.deep.equal([400, 400])
    })
    it('works for a frameless window', () => {
      const w = new BrowserWindow({
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

  ifdescribe(process.platform === 'darwin' && parseInt(os.release().split('.')[0]) >= 14)('"titleBarStyle" option', () => {
    afterEach(closeAllWindows)
    it('creates browser window with hidden title bar', () => {
      const w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        titleBarStyle: 'hidden'
      })
      const contentSize = w.getContentSize()
      expect(contentSize).to.deep.equal([400, 400])
    })
    it('creates browser window with hidden inset title bar', () => {
      const w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        titleBarStyle: 'hiddenInset'
      })
      const contentSize = w.getContentSize()
      expect(contentSize).to.deep.equal([400, 400])
    })
  })

  ifdescribe(process.platform === 'darwin')('"enableLargerThanScreen" option', () => {
    afterEach(closeAllWindows)
    it('can move the window out of screen', () => {
      const w = new BrowserWindow({ show: true, enableLargerThanScreen: true })
      w.setPosition(-10, -10)
      const after = w.getPosition()
      expect(after).to.deep.equal([-10, -10])
    })
    it('without it, cannot move the window out of screen', () => {
      const w = new BrowserWindow({ show: true, enableLargerThanScreen: false })
      w.setPosition(-10, -10)
      const after = w.getPosition()
      expect(after[1]).to.be.at.least(0)
    })
    it('can set the window larger than screen', () => {
      const w = new BrowserWindow({ show: true, enableLargerThanScreen: true })
      const size = screen.getPrimaryDisplay().size
      size.width += 100
      size.height += 100
      w.setSize(size.width, size.height)
      expectBoundsEqual(w.getSize(), [size.width, size.height])
    })
    it('without it, cannot set the window larger than screen', () => {
      const w = new BrowserWindow({ show: true, enableLargerThanScreen: false })
      const size = screen.getPrimaryDisplay().size
      size.width += 100
      size.height += 100
      w.setSize(size.width, size.height)
      expect(w.getSize()[1]).to.at.most(screen.getPrimaryDisplay().size.height)
    })
  })

  ifdescribe(process.platform === 'darwin')('"zoomToPageWidth" option', () => {
    afterEach(closeAllWindows)
    it('sets the window width to the page width when used', () => {
      const w = new BrowserWindow({
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
    afterEach(closeAllWindows)
    it('can be set on a window', () => {
      expect(() => {
        new BrowserWindow({
          tabbingIdentifier: 'group1'
        })
        new BrowserWindow({
          tabbingIdentifier: 'group2',
          frame: false
        })
      }).not.to.throw()
    })
  })

  describe('"webPreferences" option', () => {
    afterEach(() => { ipcMain.removeAllListeners('answer') })
    afterEach(closeAllWindows)

    describe('"preload" option', () => {
      const doesNotLeakSpec = (name: string, webPrefs: {nodeIntegration: boolean, sandbox: boolean, contextIsolation: boolean}) => {
        it(name, async () => {
          const w = new BrowserWindow({
            webPreferences: {
              ...webPrefs,
              preload: path.resolve(fixtures, 'module', 'empty.js')
            },
            show: false
          })
          w.loadFile(path.join(fixtures, 'api', 'no-leak.html'))
          const [, result] = await emittedOnce(ipcMain, 'leak-result')
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
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            preload
          }
        })
        w.loadFile(path.join(fixtures, 'api', 'preload.html'))
        const [, test] = await emittedOnce(ipcMain, 'answer')
        expect(test).to.eql('preload')
      })
      it('can successfully delete the Buffer global', async () => {
        const preload = path.join(fixtures, 'module', 'delete-buffer.js')
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            preload
          }
        })
        w.loadFile(path.join(fixtures, 'api', 'preload.html'))
        const [, test] = await emittedOnce(ipcMain, 'answer')
        expect(test.toString()).to.eql('buffer')
      })
      it('has synchronous access to all eventual window APIs', async () => {
        const preload = path.join(fixtures, 'module', 'access-blink-apis.js')
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            preload
          }
        })
        w.loadFile(path.join(fixtures, 'api', 'preload.html'))
        const [, test] = await emittedOnce(ipcMain, 'answer')
        expect(test).to.be.an('object')
        expect(test.atPreload).to.be.an('array')
        expect(test.atLoad).to.be.an('array')
        expect(test.atPreload).to.deep.equal(test.atLoad, 'should have access to the same window APIs')
      })
    })

    describe('session preload scripts', function () {
      const preloads = [
        path.join(fixtures, 'module', 'set-global-preload-1.js'),
        path.join(fixtures, 'module', 'set-global-preload-2.js'),
        path.relative(process.cwd(), path.join(fixtures, 'module', 'set-global-preload-3.js'))
      ]
      const defaultSession = session.defaultSession

      beforeEach(() => {
        expect(defaultSession.getPreloads()).to.deep.equal([])
        defaultSession.setPreloads(preloads)
      })
      afterEach(() => {
        defaultSession.setPreloads([])
      })

      it('can set multiple session preload script', () => {
        expect(defaultSession.getPreloads()).to.deep.equal(preloads)
      })

      const generateSpecs = (description: string, sandbox: boolean) => {
        describe(description, () => {
          it('loads the script before other scripts in window including normal preloads', function (done) {
            ipcMain.once('vars', function (event, preload1, preload2, preload3) {
              expect(preload1).to.equal('preload-1')
              expect(preload2).to.equal('preload-1-2')
              expect(preload3).to.be.null('preload 3')
              done()
            })
            const w = new BrowserWindow({
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
      it('adds extra args to process.argv in the renderer process', async () => {
        const preload = path.join(fixtures, 'module', 'check-arguments.js')
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            preload,
            additionalArguments: ['--my-magic-arg']
          }
        })
        w.loadFile(path.join(fixtures, 'api', 'blank.html'))
        const [, argv] = await emittedOnce(ipcMain, 'answer')
        expect(argv).to.include('--my-magic-arg')
      })

      it('adds extra value args to process.argv in the renderer process', async () => {
        const preload = path.join(fixtures, 'module', 'check-arguments.js')
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            preload,
            additionalArguments: ['--my-magic-arg=foo']
          }
        })
        w.loadFile(path.join(fixtures, 'api', 'blank.html'))
        const [, argv] = await emittedOnce(ipcMain, 'answer')
        expect(argv).to.include('--my-magic-arg=foo')
      })
    })

    describe('"node-integration" option', () => {
      it('disables node integration by default', async () => {
        const preload = path.join(fixtures, 'module', 'send-later.js')
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            preload
          }
        })
        w.loadFile(path.join(fixtures, 'api', 'blank.html'))
        const [, typeofProcess, typeofBuffer] = await emittedOnce(ipcMain, 'answer')
        expect(typeofProcess).to.equal('undefined')
        expect(typeofBuffer).to.equal('undefined')
      })
    })

    describe('"enableRemoteModule" option', () => {
      const generateSpecs = (description: string, sandbox: boolean) => {
        describe(description, () => {
          const preload = path.join(fixtures, 'module', 'preload-remote.js')

          it('enables the remote module by default', async () => {
            const w = new BrowserWindow({
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
            const w = new BrowserWindow({
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
      function waitForEvents<T>(emitter: {once: Function}, events: string[], callback: () => void) {
        let count = events.length
        for (const event of events) {
          emitter.once(event, () => {
            if (!--count) callback()
          })
        }
      }

      const preload = path.join(fixtures, 'module', 'preload-sandbox.js')

      let server: http.Server = null as unknown as http.Server
      let serverUrl: string = null as unknown as string

      before((done) => {
        server = http.createServer((request, response) => {
          switch (request.url) {
            case '/cross-site':
              response.end(`<html><body><h1>${request.url}</h1></body></html>`)
              break
            default:
              throw new Error(`unsupported endpoint: ${request.url}`)
          }
        }).listen(0, '127.0.0.1', () => {
          serverUrl = 'http://127.0.0.1:' + (server.address() as AddressInfo).port
          done()
        })
      })

      after(() => {
        server.close()
      })

      it('exposes ipcRenderer to preload script', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload
          }
        })
        w.loadFile(path.join(fixtures, 'api', 'preload.html'))
        const [, test] = await emittedOnce(ipcMain, 'answer')
        expect(test).to.equal('preload')
      })

      it('exposes ipcRenderer to preload script (path has special chars)', async () => {
        const preloadSpecialChars = path.join(fixtures, 'module', 'preload-sandboxæø åü.js')
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload: preloadSpecialChars
          }
        })
        w.loadFile(path.join(fixtures, 'api', 'preload.html'))
        const [, test] = await emittedOnce(ipcMain, 'answer')
        expect(test).to.equal('preload')
      })

      it('exposes "loaded" event to preload script', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload
          }
        })
        w.loadURL('about:blank')
        await emittedOnce(ipcMain, 'process-loaded')
      })

      it('exposes "exit" event to preload script', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload
          }
        })
        const htmlPath = path.join(fixtures, 'api', 'sandbox.html?exit-event')
        const pageUrl = 'file://' + htmlPath
        w.loadURL(pageUrl)
        const [, url] = await emittedOnce(ipcMain, 'answer')
        const expectedUrl = process.platform === 'win32'
          ? 'file:///' + htmlPath.replace(/\\/g, '/')
          : pageUrl
        expect(url).to.equal(expectedUrl)
      })

      it('should open windows in same domain with cross-scripting enabled', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload
          }
        })
        w.webContents.once('new-window', (event, url, frameName, disposition, options) => {
          options.webPreferences.preload = preload
        })
        const htmlPath = path.join(fixtures, 'api', 'sandbox.html?window-open')
        const pageUrl = 'file://' + htmlPath
        const answer = emittedOnce(ipcMain, 'answer')
        w.loadURL(pageUrl)
        const [, url, frameName, , options] = await emittedOnce(w.webContents, 'new-window')
        const expectedUrl = process.platform === 'win32'
          ? 'file:///' + htmlPath.replace(/\\/g, '/')
          : pageUrl
        expect(url).to.equal(expectedUrl)
        expect(frameName).to.equal('popup!')
        expect(options.width).to.equal(500)
        expect(options.height).to.equal(600)
        const [, html] = await answer
        expect(html).to.equal('<h1>scripting from opener</h1>')
      })

      it('should open windows in another domain with cross-scripting disabled', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload
          }
        })

        w.webContents.once('new-window', (event, url, frameName, disposition, options) => {
          options.webPreferences.preload = preload
        })
        w.loadFile(
          path.join(fixtures, 'api', 'sandbox.html'),
          { search: 'window-open-external' }
        )

        // Wait for a message from the main window saying that it's ready.
        await emittedOnce(ipcMain, 'opener-loaded')

        // Ask the opener to open a popup with window.opener.
        const expectedPopupUrl = `${serverUrl}/cross-site` // Set in "sandbox.html".

        w.webContents.send('open-the-popup', expectedPopupUrl)

        // The page is going to open a popup that it won't be able to close.
        // We have to close it from here later.
        const [, popupWindow] = await emittedOnce(app, 'browser-window-created')

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
        await closeWindow(popupWindow, { assertNotWindows: false })

        expect(popupAccessMessage).to.be.a('string',
          `child's .document is accessible from its parent window`)
        expect(popupAccessMessage).to.match(/^Blocked a frame with origin/)
        expect(openerAccessMessage).to.be.a('string',
          `opener .document is accessible from a popup window`)
        expect(openerAccessMessage).to.match(/^Blocked a frame with origin/)
      })

      it('should inherit the sandbox setting in opened windows', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true
          }
        })

        const preloadPath = path.join(fixtures, 'api', 'new-window-preload.js')
        w.webContents.once('new-window', (event, url, frameName, disposition, options) => {
          options.webPreferences.preload = preloadPath
        })
        w.loadFile(path.join(fixtures, 'api', 'new-window.html'))
        const [, args] = await emittedOnce(ipcMain, 'answer')
        expect(args).to.include('--enable-sandbox')
      })

      it('should open windows with the options configured via new-window event listeners', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true
          }
        })

        const preloadPath = path.join(fixtures, 'api', 'new-window-preload.js')
        w.webContents.once('new-window', (event, url, frameName, disposition, options) => {
          options.webPreferences.preload = preloadPath
          options.webPreferences.foo = 'bar'
        })
        w.loadFile(path.join(fixtures, 'api', 'new-window.html'))
        const [, , webPreferences] = await emittedOnce(ipcMain, 'answer')
        expect(webPreferences.foo).to.equal('bar')
      })

      it('should set ipc event sender correctly', (done) => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload
          }
        })
        let childWc: WebContents | null = null
        w.webContents.on('new-window', (event, url, frameName, disposition, options) => {
          options.webPreferences.preload = preload
          childWc = options.webContents
          expect(w.webContents).to.not.equal(childWc)
        })
        ipcMain.once('parent-ready', function (event) {
          expect(event.sender).to.equal(w.webContents, 'sender should be the parent')
          event.sender.send('verified')
        })
        ipcMain.once('child-ready', function (event) {
          expect(childWc).to.not.be.null('child webcontents should be available')
          expect(event.sender).to.equal(childWc, 'sender should be the child')
          event.sender.send('verified')
        })
        waitForEvents(ipcMain, [
          'parent-answer',
          'child-answer'
        ], done)
        w.loadFile(path.join(fixtures, 'api', 'sandbox.html'), { search: 'verify-ipc-sender' })
      })

      describe('event handling', () => {
        let w: BrowserWindow = null as unknown as BrowserWindow
        beforeEach(() => {
          w = new BrowserWindow({show: false, webPreferences: {nodeIntegration: true}})
        })
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
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true
          }
        })
        const initialWebContents = webContents.getAllWebContents().map((i) => i.id)
        w.webContents.once('new-window', (e) => {
          e.preventDefault()
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
        const w = new BrowserWindow({
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
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            preload,
            sandbox: true
          }
        })
        w.webContents.once('new-window', (event, url, frameName, disposition, options) => {
          options.webPreferences.preload = preload
        })

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

      it('validates process APIs access in sandboxed renderer', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload
          }
        })
        w.webContents.once('preload-error', (event, preloadPath, error) => {
          throw error
        })
        process.env.sandboxmain = 'foo'
        w.loadFile(path.join(fixtures, 'api', 'preload.html'))
        const [, test] = await emittedOnce(ipcMain, 'answer')
        expect(test.hasCrash).to.be.true('has crash')
        expect(test.hasHang).to.be.true('has hang')
        expect(test.heapStatistics).to.be.an('object')
        expect(test.blinkMemoryInfo).to.be.an('object')
        expect(test.processMemoryInfo).to.be.an('object')
        expect(test.systemVersion).to.be.a('string')
        expect(test.cpuUsage).to.be.an('object')
        expect(test.ioCounters).to.be.an('object')
        expect(test.arch).to.equal(process.arch)
        expect(test.platform).to.equal(process.platform)
        expect(test.env).to.deep.equal(process.env)
        expect(test.execPath).to.equal(process.helperExecPath)
        expect(test.sandboxed).to.be.true('sandboxed')
        expect(test.type).to.equal('renderer')
        expect(test.version).to.equal(process.version)
        expect(test.versions).to.deep.equal(process.versions)

        if (process.platform === 'linux' && test.osSandbox) {
          expect(test.creationTime).to.be.null('creation time')
          expect(test.systemMemoryInfo).to.be.null('system memory info')
        } else {
          expect(test.creationTime).to.be.a('number')
          expect(test.systemMemoryInfo).to.be.an('object')
        }
      })

      it('webview in sandbox renderer', async () => {
        const w = new BrowserWindow({
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
      let w: BrowserWindow = null as unknown as BrowserWindow

      beforeEach(() => {
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
      ifit(!process.env.ELECTRON_SKIP_NATIVE_MODULE_TESTS)('loads native addons correctly after reload', async () => {
        w.loadFile(path.join(fixtures, 'api', 'native-window-open-native-addon.html'))
        {
          const [, content] = await emittedOnce(ipcMain, 'answer')
          expect(content).to.equal('function')
        }
        w.reload()
        {
          const [, content] = await emittedOnce(ipcMain, 'answer')
          expect(content).to.equal('function')
        }
      })
      it('should inherit the nativeWindowOpen setting in opened windows', async () => {
        const preloadPath = path.join(fixtures, 'api', 'new-window-preload.js')
        w.webContents.once('new-window', (event, url, frameName, disposition, options) => {
          options.webPreferences.preload = preloadPath
        })
        w.loadFile(path.join(fixtures, 'api', 'new-window.html'))
        const [, args] = await emittedOnce(ipcMain, 'answer')
        expect(args).to.include('--native-window-open')
      })
      it('should open windows with the options configured via new-window event listeners', async () => {
        const preloadPath = path.join(fixtures, 'api', 'new-window-preload.js')
        w.webContents.once('new-window', (event, url, frameName, disposition, options) => {
          options.webPreferences.preload = preloadPath
          options.webPreferences.foo = 'bar'
        })
        w.loadFile(path.join(fixtures, 'api', 'new-window.html'))
        const [, , webPreferences] = await emittedOnce(ipcMain, 'answer')
        expect(webPreferences.foo).to.equal('bar')
      })
      it('should have nodeIntegration disabled in child windows', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            nativeWindowOpen: true,
          }
        })
        w.loadFile(path.join(fixtures, 'api', 'native-window-open-argv.html'))
        const [, typeofProcess] = await emittedOnce(ipcMain, 'answer')
        expect(typeofProcess).to.eql('undefined')
      })

      describe('window.location', () => {
        const protocols = [
          ['foo', path.join(fixtures, 'api', 'window-open-location-change.html')],
          ['bar', path.join(fixtures, 'api', 'window-open-location-final.html')]
        ]
        beforeEach(async () => {
          await Promise.all(protocols.map(([scheme, path]) => new Promise((resolve, reject) => {
            protocol.registerBufferProtocol(scheme, (request, callback) => {
              callback({
                mimeType: 'text/html',
                data: fs.readFileSync(path)
              })
            }, (error) => {
              if (error != null) {
                reject(error)
              } else {
                resolve()
              }
            })
          })))
        })
        afterEach(async () => {
          await Promise.all(protocols.map(([scheme,]) => {
            return new Promise(resolve => protocol.unregisterProtocol(scheme, () => resolve()))
          }))
        })
        it('retains the original web preferences when window.location is changed to a new origin', async () => {
          const w = new BrowserWindow({
            show: false,
            webPreferences: {
              nativeWindowOpen: true,
              // test relies on preloads in opened window
              nodeIntegrationInSubFrames: true
            }
          })

          w.webContents.once('new-window', (event, url, frameName, disposition, options) => {
            options.webPreferences.preload = path.join(fixtures, 'api', 'window-open-preload.js')
          })
          w.loadFile(path.join(fixtures, 'api', 'window-open-location-open.html'))
          const [, args, typeofProcess] = await emittedOnce(ipcMain, 'answer')
          expect(args).not.to.include('--node-integration')
          expect(args).to.include('--native-window-open')
          expect(typeofProcess).to.eql('undefined')
        })

        it('window.opener is not null when window.location is changed to a new origin', async () => {
          const w = new BrowserWindow({
            show: false,
            webPreferences: {
              nativeWindowOpen: true,
              // test relies on preloads in opened window
              nodeIntegrationInSubFrames: true
            }
          })

          w.webContents.once('new-window', (event, url, frameName, disposition, options) => {
            options.webPreferences.preload = path.join(fixtures, 'api', 'window-open-preload.js')
          })
          w.loadFile(path.join(fixtures, 'api', 'window-open-location-open.html'))
          const [, , , windowOpenerIsNull] = await emittedOnce(ipcMain, 'answer')
          expect(windowOpenerIsNull).to.be.false('window.opener is null')
        })
      })
    })

    describe('"disableHtmlFullscreenWindowResize" option', () => {
      it('prevents window from resizing when set', (done) => {
        const w = new BrowserWindow({
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
    afterEach(closeAllWindows)
    it('opens window with cross-scripting enabled from isolated context', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nativeWindowOpen: true,
          contextIsolation: true,
          preload: path.join(fixtures, 'api', 'native-window-open-isolated-preload.js')
        }
      })
      w.loadFile(path.join(fixtures, 'api', 'native-window-open-isolated.html'))
      const [, content] = await emittedOnce(ipcMain, 'answer')
      expect(content).to.equal('Hello')
    })
  })

  describe('beforeunload handler', () => {
    let w: BrowserWindow = null as unknown as BrowserWindow
    beforeEach(() => {
      w = new BrowserWindow({show: false, webPreferences: {nodeIntegration: true}})
    })
    afterEach(closeAllWindows)
    it('returning undefined would not prevent close', (done) => {
      w.once('closed', () => { done() })
      w.loadFile(path.join(fixtures, 'api', 'close-beforeunload-undefined.html'))
    })
    it('returning false would prevent close', (done) => {
      w.once('onbeforeunload' as any, () => { done() })
      w.loadFile(path.join(fixtures, 'api', 'close-beforeunload-false.html'))
    })
    it('returning empty string would prevent close', (done) => {
      w.once('onbeforeunload' as any, () => { done() })
      w.loadFile(path.join(fixtures, 'api', 'close-beforeunload-empty-string.html'))
    })
    it('emits for each close attempt', (done) => {
      let beforeUnloadCount = 0
      w.on('onbeforeunload' as any, () => {
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
      w.on('onbeforeunload' as any, () => {
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
      w.on('onbeforeunload' as any, () => {
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

  describe('document.visibilityState/hidden', () => {
    afterEach(closeAllWindows)

    it('visibilityState is initially visible despite window being hidden', async () => {
      const w = new BrowserWindow({
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

      w.loadFile(path.join(fixtures, 'pages', 'visibilitychange.html'))

      const [, visibilityState, hidden] = await emittedOnce(ipcMain, 'pong')

      expect(readyToShow).to.be.false('ready to show')
      expect(visibilityState).to.equal('visible')
      expect(hidden).to.be.false('hidden')
    })

    it('visibilityState changes when window is hidden', async () => {
      const w = new BrowserWindow({
        width: 100,
        height: 100,
        webPreferences: {
          nodeIntegration: true
        }
      })

      w.loadFile(path.join(fixtures, 'pages', 'visibilitychange.html'))

      {
        const [, visibilityState, hidden] = await emittedOnce(ipcMain, 'pong')
        expect(visibilityState).to.equal('visible')
        expect(hidden).to.be.false('hidden')
      }

      w.hide()

      {
        const [, visibilityState, hidden] = await emittedOnce(ipcMain, 'pong')
        expect(visibilityState).to.equal('hidden')
        expect(hidden).to.be.true('hidden')
      }
    })

    it('visibilityState changes when window is shown', async () => {
      const w = new BrowserWindow({
        width: 100,
        height: 100,
        webPreferences: {
          nodeIntegration: true
        }
      })

      w.loadFile(path.join(fixtures, 'pages', 'visibilitychange.html'))
      await emittedOnce(w, 'show')
      w.hide()
      w.show()
      const [, visibilityState] = await emittedOnce(ipcMain, 'pong')
      expect(visibilityState).to.equal('visible')
    })

    ifit(!(isCI && process.platform === 'win32'))('visibilityState changes when window is shown inactive', async () => {
      const w = new BrowserWindow({
        width: 100,
        height: 100,
        webPreferences: {
          nodeIntegration: true
        }
      })
      w.loadFile(path.join(fixtures, 'pages', 'visibilitychange.html'))
      await emittedOnce(w, 'show')
      w.hide()
      w.showInactive()
      const [, visibilityState] = await emittedOnce(ipcMain, 'pong')
      expect(visibilityState).to.equal('visible')
    })

    ifit(!(isCI && process.platform === 'linux'))('visibilityState changes when window is minimized', async () => {
      const w = new BrowserWindow({
        width: 100,
        height: 100,
        webPreferences: {
          nodeIntegration: true
        }
      })
      w.loadFile(path.join(fixtures, 'pages', 'visibilitychange.html'))

      {
        const [, visibilityState, hidden] = await emittedOnce(ipcMain, 'pong')
        expect(visibilityState).to.equal('visible')
        expect(hidden).to.be.false('hidden')
      }

      w.minimize()

      {
        const [, visibilityState, hidden] = await emittedOnce(ipcMain, 'pong')
        expect(visibilityState).to.equal('hidden')
        expect(hidden).to.be.true('hidden')
      }
    })

    it('visibilityState remains visible if backgroundThrottling is disabled', async () => {
      const w = new BrowserWindow({
        show: false,
        width: 100,
        height: 100,
        webPreferences: {
          backgroundThrottling: false,
          nodeIntegration: true
        }
      })
      w.loadFile(path.join(fixtures, 'pages', 'visibilitychange.html'))
      {
        const [, visibilityState, hidden] = await emittedOnce(ipcMain, 'pong')
        expect(visibilityState).to.equal('visible')
        expect(hidden).to.be.false('hidden')
      }

      ipcMain.once('pong', (event, visibilityState, hidden) => {
        throw new Error(`Unexpected visibility change event. visibilityState: ${visibilityState} hidden: ${hidden}`)
      })
      try {
        w.show()
        await emittedOnce(w, 'show')
        w.hide()
        await emittedOnce(w, 'hide')
        w.show()
        await emittedOnce(w, 'show')
      } finally {
        ipcMain.removeAllListeners('pong')
      }
    })
  })

  describe('new-window event', () => {
    afterEach(closeAllWindows)

    it('emits when window.open is called', (done) => {
      const w = new BrowserWindow({show: false, webPreferences: {nodeIntegration: true}})
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
      const w = new BrowserWindow({ show: false })
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
      const w = new BrowserWindow({show: false, webPreferences: {nodeIntegration: true}})
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
    afterEach(closeAllWindows)
    it('emits when window is maximized', (done) => {
      const w = new BrowserWindow({show: false})
      w.once('maximize', () => { done() })
      w.show()
      w.maximize()
    })
  })

  describe('unmaximize event', () => {
    afterEach(closeAllWindows)
    it('emits when window is unmaximized', (done) => {
      const w = new BrowserWindow({show: false})
      w.once('unmaximize', () => { done() })
      w.show()
      w.maximize()
      w.unmaximize()
    })
  })

  describe('minimize event', () => {
    afterEach(closeAllWindows)
    it('emits when window is minimized', (done) => {
      const w = new BrowserWindow({show: false})
      w.once('minimize', () => { done() })
      w.show()
      w.minimize()
    })
  })

  describe('focus event', () => {
    afterEach(closeAllWindows)
    it('should not emit if focusing on a main window with a modal open', (done) => {
      const w = new BrowserWindow({show: false})
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
      child.loadURL('about:blank')
      w.show()
    })
  })

  ifdescribe(process.platform === 'darwin')('sheet-begin event', () => {
    afterEach(closeAllWindows)

    it('emits when window opens a sheet', (done) => {
      const w = new BrowserWindow()
      w.once('sheet-begin', () => {
        done()
      })
      new BrowserWindow({
        modal: true,
        parent: w
      })
    })
  })

  ifdescribe(process.platform === 'darwin')('sheet-end event', () => {
    afterEach(closeAllWindows)

    it('emits when window has closed a sheet', (done) => {
      const w = new BrowserWindow()
      const sheet = new BrowserWindow({
        modal: true,
        parent: w
      })
      w.once('sheet-end', () => { done() })
      sheet.close()
    })
  })

  describe('beginFrameSubscription method', () => {
    it('does not crash when callback returns nothing', (done) => {
      const w = new BrowserWindow({show: false})
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
      const w = new BrowserWindow({show: false})
      let called = false
      w.loadFile(path.join(fixtures, 'api', 'frame-subscriber.html'))
      w.webContents.on('dom-ready', () => {
        w.webContents.beginFrameSubscription(function (data) {
          // This callback might be called twice.
          if (called) return
          called = true

          expect(data.constructor.name).to.equal('NativeImage')
          expect(data.isEmpty()).to.be.false('data is empty')

          w.webContents.endFrameSubscription()
          done()
        })
      })
    })

    it('subscribes to frame updates (only dirty rectangle)', (done) => {
      const w = new BrowserWindow({show: false})
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

    it('throws error when subscriber is not well defined', () => {
      const w = new BrowserWindow({show: false})
      expect(() => {
        w.webContents.beginFrameSubscription(true, true as any)
      }).to.throw('Error processing argument at index 1, conversion failure from true')
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
    afterEach(closeAllWindows)

    it('should save page to disk', async () => {
      const w = new BrowserWindow({show: false})
      await w.loadFile(path.join(fixtures, 'pages', 'save_page', 'index.html'))
      await w.webContents.savePage(savePageHtmlPath, 'HTMLComplete')

      expect(fs.existsSync(savePageHtmlPath)).to.be.true('html path')
      expect(fs.existsSync(savePageJsPath)).to.be.true('js path')
      expect(fs.existsSync(savePageCssPath)).to.be.true('css path')
    })
  })

  describe('BrowserWindow options argument is optional', () => {
    afterEach(closeAllWindows)
    it('should create a window with default size (800x600)', () => {
      const w = new BrowserWindow()
      expect(w.getSize()).to.deep.equal([800, 600])
    })
  })

  describe('BrowserWindow.restore()', () => {
    afterEach(closeAllWindows)
    it('should restore the previous window size', () => {
      const w = new BrowserWindow({
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
    afterEach(closeAllWindows)
    it('should restore the previous window position', () => {
      const w = new BrowserWindow()

      const initialPosition = w.getPosition()
      w.maximize()
      w.unmaximize()
      expectBoundsEqual(w.getPosition(), initialPosition)
    })
  })

  describe('setFullScreen(false)', () => {
    afterEach(closeAllWindows)

    // only applicable to windows: https://github.com/electron/electron/issues/6036
    ifdescribe(process.platform === 'win32')('on windows', () => {
      it('should restore a normal visible window from a fullscreen startup state', async () => {
        const w = new BrowserWindow({show: false})
        await w.loadURL('about:blank')
        // start fullscreen and hidden
        w.setFullScreen(true)
        w.show()
        await emittedOnce(w, 'show')
        w.setFullScreen(false)
        await emittedOnce(w, 'leave-full-screen')
        expect(w.isVisible()).to.be.true('visible')
        expect(w.isFullScreen()).to.be.false('fullscreen')
      })
      it('should keep window hidden if already in hidden state', async () => {
        const w = new BrowserWindow({show: false})
        await w.loadURL('about:blank')
        w.setFullScreen(false)
        await emittedOnce(w, 'leave-full-screen')
        expect(w.isVisible()).to.be.false('visible')
        expect(w.isFullScreen()).to.be.false('fullscreen')
      })
    })

    ifdescribe(process.platform === 'darwin')('BrowserWindow.setFullScreen(false) when HTML fullscreen', () => {
      it('exits HTML fullscreen when window leaves fullscreen', async () => {
        const w = new BrowserWindow()
        await w.loadURL('about:blank')
        await w.webContents.executeJavaScript('document.body.webkitRequestFullscreen()', true)
        await emittedOnce(w, 'enter-full-screen')
        // Wait a tick for the full-screen state to 'stick'
        await new Promise(resolve => setTimeout(resolve))
        w.setFullScreen(false)
        await emittedOnce(w, 'leave-html-full-screen')
      })
    })
  })
})

import * as chai from 'chai'
import * as chaiAsPromised from 'chai-as-promised'
import * as path from 'path'
import * as fs from 'fs'
import * as qs from 'querystring'
import * as http from 'http'
import { AddressInfo } from 'net'
import { app, BrowserWindow, ipcMain, OnBeforeSendHeadersListenerDetails, screen } from 'electron'
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
      w.loadURL('data:text/html,<script src="loaded-from-dataurl.js"></script>', { baseURLForDataURL: `file://${path.join(fixtures, 'api')}${path.sep}` })
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

})

'use strict'

const ChildProcess = require('child_process')
const fs = require('fs')
const http = require('http')
const path = require('path')
const { closeWindow } = require('./window-helpers')
const { emittedOnce } = require('./events-helpers')
const chai = require('chai')
const dirtyChai = require('dirty-chai')

const features = process.electronBinding('features')
const { ipcRenderer, remote, clipboard } = require('electron')
const { BrowserWindow, webContents, ipcMain, session } = remote
const { expect } = chai

const isCi = remote.getGlobal('isCi')

chai.use(dirtyChai)

/* The whole webContents API doesn't use standard callbacks */
/* eslint-disable standard/no-callback-literal */

describe('webContents module', () => {
  const fixtures = path.resolve(__dirname, 'fixtures')
  let w

  beforeEach(() => {
    w = new BrowserWindow({
      show: false,
      width: 400,
      height: 400,
      webPreferences: {
        backgroundThrottling: false,
        nodeIntegration: true,
        webviewTag: true
      }
    })
  })

  afterEach(() => closeWindow(w).then(() => { w = null }))

  describe('devtools window', () => {
    let testFn = it
    if (process.platform === 'darwin' && isCi) {
      testFn = it.skip
    }
    if (process.platform === 'win32' && isCi) {
      testFn = it.skip
    }
    try {
      // We have other tests that check if native modules work, if we fail to require
      // robotjs let's skip this test to avoid false negatives
      require('robotjs')
    } catch (err) {
      testFn = it.skip
    }

    testFn('can receive and handle menu events', async function () {
      this.timeout(5000)
      w.show()
      w.loadFile(path.join(fixtures, 'pages', 'key-events.html'))
      // Ensure the devtools are loaded
      w.webContents.closeDevTools()
      const opened = emittedOnce(w.webContents, 'devtools-opened')
      w.webContents.openDevTools()
      await opened
      await emittedOnce(w.webContents.devToolsWebContents, 'did-finish-load')
      w.webContents.devToolsWebContents.focus()

      // Focus an input field
      await w.webContents.devToolsWebContents.executeJavaScript(
        `const input = document.createElement('input');
        document.body.innerHTML = '';
        document.body.appendChild(input)
        input.focus();`
      )

      // Write something to the clipboard
      clipboard.writeText('test value')

      // Fake a paste request using robotjs to emulate a REAL keyboard paste event
      require('robotjs').keyTap('v', process.platform === 'darwin' ? ['command'] : ['control'])

      const start = Date.now()
      let val

      // Check every now and again for the pasted value (paste is async)
      while (val !== 'test value' && Date.now() - start <= 1000) {
        val = await w.webContents.devToolsWebContents.executeJavaScript(
          `document.querySelector('input').value`
        )
        await new Promise(resolve => setTimeout(resolve, 10))
      }

      // Once we're done expect the paste to have been successful
      expect(val).to.equal('test value', 'value should eventually become the pasted value')
    })
  })

  describe('webrtc ip policy api', () => {
    it('can set and get webrtc ip policies', () => {
      const policies = [
        'default',
        'default_public_interface_only',
        'default_public_and_private_interfaces',
        'disable_non_proxied_udp'
      ]
      policies.forEach((policy) => {
        w.webContents.setWebRTCIPHandlingPolicy(policy)
        expect(w.webContents.getWebRTCIPHandlingPolicy()).to.equal(policy)
      })
    })
  })

  describe('render view deleted events', () => {
    let server = null

    before((done) => {
      server = http.createServer((req, res) => {
        const respond = () => {
          if (req.url === '/redirect-cross-site') {
            res.setHeader('Location', `${server.cross_site_url}/redirected`)
            res.statusCode = 302
            res.end()
          } else if (req.url === '/redirected') {
            res.end('<html><script>window.localStorage</script></html>')
          } else {
            res.end()
          }
        }
        setTimeout(respond, 0)
      })
      server.listen(0, '127.0.0.1', () => {
        server.url = `http://127.0.0.1:${server.address().port}`
        server.cross_site_url = `http://localhost:${server.address().port}`
        done()
      })
    })

    after(() => {
      server.close()
      server = null
    })

    it('does not emit current-render-view-deleted when speculative RVHs are deleted', (done) => {
      let currentRenderViewDeletedEmitted = false
      w.webContents.once('destroyed', () => {
        expect(currentRenderViewDeletedEmitted).to.be.false('current-render-view-deleted was emitted')
        done()
      })
      const renderViewDeletedHandler = () => {
        currentRenderViewDeletedEmitted = true
      }
      w.webContents.on('current-render-view-deleted', renderViewDeletedHandler)
      w.webContents.on('did-finish-load', (e) => {
        w.webContents.removeListener('current-render-view-deleted', renderViewDeletedHandler)
        w.close()
      })
      w.loadURL(`${server.url}/redirect-cross-site`)
    })

    it('emits current-render-view-deleted if the current RVHs are deleted', (done) => {
      let currentRenderViewDeletedEmitted = false
      w.webContents.once('destroyed', () => {
        expect(currentRenderViewDeletedEmitted).to.be.true('current-render-view-deleted wasn\'t emitted')
        done()
      })
      w.webContents.on('current-render-view-deleted', () => {
        currentRenderViewDeletedEmitted = true
      })
      w.webContents.on('did-finish-load', (e) => {
        w.close()
      })
      w.loadURL(`${server.url}/redirect-cross-site`)
    })

    it('emits render-view-deleted if any RVHs are deleted', (done) => {
      let rvhDeletedCount = 0
      w.webContents.once('destroyed', () => {
        const expectedRenderViewDeletedEventCount = 3 // 1 speculative upon redirection + 2 upon window close.
        expect(rvhDeletedCount).to.equal(expectedRenderViewDeletedEventCount, 'render-view-deleted wasn\'t emitted the expected nr. of times')
        done()
      })
      w.webContents.on('render-view-deleted', () => {
        rvhDeletedCount++
      })
      w.webContents.on('did-finish-load', (e) => {
        w.close()
      })
      w.loadURL(`${server.url}/redirect-cross-site`)
    })
  })

  describe('setIgnoreMenuShortcuts(ignore)', () => {
    it('does not throw', () => {
      expect(() => {
        w.webContents.setIgnoreMenuShortcuts(true)
        w.webContents.setIgnoreMenuShortcuts(false)
      }).to.not.throw()
    })
  })

  describe('create()', () => {
    it('does not crash on exit', async () => {
      const appPath = path.join(__dirname, 'fixtures', 'api', 'leak-exit-webcontents.js')
      const electronPath = remote.getGlobal('process').execPath
      const appProcess = ChildProcess.spawn(electronPath, [appPath])
      const [code] = await emittedOnce(appProcess, 'close')
      expect(code).to.equal(0)
    })
  })

  // Destroying webContents in its event listener is going to crash when
  // Electron is built in Debug mode.
  xdescribe('destroy()', () => {
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

    it('should not crash when invoked synchronously inside navigation observer', (done) => {
      const events = [
        { name: 'did-start-loading', url: `${server.url}/200` },
        { name: 'dom-ready', url: `${server.url}/200` },
        { name: 'did-stop-loading', url: `${server.url}/200` },
        { name: 'did-finish-load', url: `${server.url}/200` },
        // FIXME: Multiple Emit calls inside an observer assume that object
        // will be alive till end of the observer. Synchronous `destroy` api
        // violates this contract and crashes.
        // { name: 'did-frame-finish-load', url: `${server.url}/200` },
        { name: 'did-fail-load', url: `${server.url}/404` }
      ]
      const responseEvent = 'webcontents-destroyed'

      function * genNavigationEvent () {
        let eventOptions = null
        while ((eventOptions = events.shift()) && events.length) {
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

  describe('did-change-theme-color event', () => {
    it('is triggered with correct theme color', (done) => {
      let count = 0
      w.webContents.on('did-change-theme-color', (e, color) => {
        if (count === 0) {
          count += 1
          expect(color).to.equal('#FFEEDD')
          w.loadFile(path.join(fixtures, 'pages', 'base-page.html'))
        } else if (count === 1) {
          expect(color).to.be.null()
          done()
        }
      })
      w.loadFile(path.join(fixtures, 'pages', 'theme-color.html'))
    })
  })

  describe('console-message event', () => {
    it('is triggered with correct log message', (done) => {
      w.webContents.on('console-message', (e, level, message) => {
        // Don't just assert as Chromium might emit other logs that we should ignore.
        if (message === 'a') {
          done()
        }
      })
      w.loadFile(path.join(fixtures, 'pages', 'a.html'))
    })
  })

  describe('ipc-message event', () => {
    it('emits when the renderer process sends an asynchronous message', async () => {
      const webContents = remote.getCurrentWebContents()
      const promise = emittedOnce(webContents, 'ipc-message')

      ipcRenderer.send('message', 'Hello World!')

      const [, channel, message] = await promise
      expect(channel).to.equal('message')
      expect(message).to.equal('Hello World!')
    })
  })

  describe('ipc-message-sync event', () => {
    it('emits when the renderer process sends a synchronous message', async () => {
      const webContents = remote.getCurrentWebContents()
      const promise = emittedOnce(webContents, 'ipc-message-sync')

      ipcRenderer.send('handle-next-ipc-message-sync', 'foobar')
      const result = ipcRenderer.sendSync('message', 'Hello World!')

      const [, channel, message] = await promise
      expect(channel).to.equal('message')
      expect(message).to.equal('Hello World!')
      expect(result).to.equal('foobar')
    })
  })

  describe('referrer', () => {
    it('propagates referrer information to new target=_blank windows', (done) => {
      const server = http.createServer((req, res) => {
        if (req.url === '/should_have_referrer') {
          expect(req.headers.referer).to.equal(`http://127.0.0.1:${server.address().port}/`)
          return done()
        }
        res.end('<a id="a" href="/should_have_referrer" target="_blank">link</a>')
      })
      server.listen(0, '127.0.0.1', () => {
        const url = 'http://127.0.0.1:' + server.address().port + '/'
        w.webContents.once('did-finish-load', () => {
          w.webContents.once('new-window', (event, newUrl, frameName, disposition, options, features, referrer) => {
            expect(referrer.url).to.equal(url)
            expect(referrer.policy).to.equal('no-referrer-when-downgrade')
          })
          w.webContents.executeJavaScript('a.click()')
        })
        w.loadURL(url)
      })
    })

    // TODO(jeremy): window.open() in a real browser passes the referrer, but
    // our hacked-up window.open() shim doesn't. It should.
    xit('propagates referrer information to windows opened with window.open', (done) => {
      const server = http.createServer((req, res) => {
        if (req.url === '/should_have_referrer') {
          expect(req.headers.referer).to.equal(`http://127.0.0.1:${server.address().port}/`)
          return done()
        }
        res.end('')
      })
      server.listen(0, '127.0.0.1', () => {
        const url = 'http://127.0.0.1:' + server.address().port + '/'
        w.webContents.once('did-finish-load', () => {
          w.webContents.once('new-window', (event, newUrl, frameName, disposition, options, features, referrer) => {
            expect(referrer.url).to.equal(url)
            expect(referrer.policy).to.equal('no-referrer-when-downgrade')
          })
          w.webContents.executeJavaScript('window.open(location.href + "should_have_referrer")')
        })
        w.loadURL(url)
      })
    })
  })

  describe('webframe messages in sandboxed contents', () => {
    it('responds to executeJavaScript', async () => {
      w.destroy()
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          sandbox: true
        }
      })
      await w.loadURL('about:blank')
      const result = await w.webContents.executeJavaScript('37 + 5')
      expect(result).to.equal(42)
    })
  })

  describe('preload-error event', () => {
    const generateSpecs = (description, sandbox) => {
      describe(description, () => {
        it('is triggered when unhandled exception is thrown', async () => {
          const preload = path.join(fixtures, 'module', 'preload-error-exception.js')

          w.destroy()
          w = new BrowserWindow({
            show: false,
            webPreferences: {
              sandbox,
              preload
            }
          })

          const promise = emittedOnce(w.webContents, 'preload-error')
          w.loadURL('about:blank')

          const [, preloadPath, error] = await promise
          expect(preloadPath).to.equal(preload)
          expect(error.message).to.equal('Hello World!')
        })

        it('is triggered on syntax errors', async () => {
          const preload = path.join(fixtures, 'module', 'preload-error-syntax.js')

          w.destroy()
          w = new BrowserWindow({
            show: false,
            webPreferences: {
              sandbox,
              preload
            }
          })

          const promise = emittedOnce(w.webContents, 'preload-error')
          w.loadURL('about:blank')

          const [, preloadPath, error] = await promise
          expect(preloadPath).to.equal(preload)
          expect(error.message).to.equal('foobar is not defined')
        })

        it('is triggered when preload script loading fails', async () => {
          const preload = path.join(fixtures, 'module', 'preload-invalid.js')

          w.destroy()
          w = new BrowserWindow({
            show: false,
            webPreferences: {
              sandbox,
              preload
            }
          })

          const promise = emittedOnce(w.webContents, 'preload-error')
          w.loadURL('about:blank')

          const [, preloadPath, error] = await promise
          expect(preloadPath).to.equal(preload)
          expect(error.message).to.contain('preload-invalid.js')
        })
      })
    }

    generateSpecs('without sandbox', false)
    generateSpecs('with sandbox', true)
  })

  describe('takeHeapSnapshot()', () => {
    it('works with sandboxed renderers', async () => {
      w.destroy()
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          sandbox: true
        }
      })

      await w.loadURL('about:blank')

      const filePath = path.join(remote.app.getPath('temp'), 'test.heapsnapshot')

      const cleanup = () => {
        try {
          fs.unlinkSync(filePath)
        } catch (e) {
          // ignore error
        }
      }

      try {
        await w.webContents.takeHeapSnapshot(filePath)
        const stats = fs.statSync(filePath)
        expect(stats.size).not.to.be.equal(0)
      } finally {
        cleanup()
      }
    })

    it('fails with invalid file path', async () => {
      w.destroy()
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          sandbox: true
        }
      })

      await w.loadURL('about:blank')

      const promise = w.webContents.takeHeapSnapshot('')
      return expect(promise).to.be.eventually.rejectedWith(Error, 'takeHeapSnapshot failed')
    })
  })

  describe('setBackgroundThrottling()', () => {
    it('does not crash when allowing', (done) => {
      w.webContents.setBackgroundThrottling(true)
      done()
    })

    it('does not crash when disallowing', (done) => {
      w.destroy()
      w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        webPreferences: {
          backgroundThrottling: true
        }
      })

      w.webContents.setBackgroundThrottling(false)
      done()
    })

    it('does not crash when called via BrowserWindow', (done) => {
      w.setBackgroundThrottling(true)
      done()
    })
  })

  describe('getPrinterList()', () => {
    before(function () {
      if (!features.isPrintingEnabled()) {
        return closeWindow(w).then(() => {
          w = null
          this.skip()
        })
      }
    })

    it('can get printer list', async () => {
      w.destroy()
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          sandbox: true
        }
      })
      await w.loadURL('data:text/html,%3Ch1%3EHello%2C%20World!%3C%2Fh1%3E')
      const printers = w.webContents.getPrinters()
      expect(printers).to.be.an('array')
    })
  })

  describe('printToPDF()', () => {
    before(function () {
      if (!features.isPrintingEnabled()) {
        return closeWindow(w).then(() => {
          w = null
          this.skip()
        })
      }
    })

    it('can print to PDF', async () => {
      w.destroy()
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          sandbox: true
        }
      })
      await w.loadURL('data:text/html,%3Ch1%3EHello%2C%20World!%3C%2Fh1%3E')
      const data = await w.webContents.printToPDF({})
      expect(data).to.be.an.instanceof(Buffer).that.is.not.empty()
    })
  })

  describe('PictureInPicture video', () => {
    it('works as expected', (done) => {
      w.destroy()
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          sandbox: true
        }
      })
      w.webContents.once('did-finish-load', async () => {
        const result = await w.webContents.executeJavaScript(
          `runTest(${features.isPictureInPictureEnabled()})`, true)
        expect(result).to.be.true()
        done()
      })
      w.loadFile(path.join(fixtures, 'api', 'picture-in-picture.html'))
    })
  })

  // FIXME: disable during chromium update due to crash in content::WorkerScriptFetchInitation::CreateScriptLoaderOnIO
  xdescribe('Shared Workers', () => {
    let worker1
    let worker2
    let vectorOfWorkers

    before(() => {
      worker1 = new SharedWorker('../fixtures/api/shared-worker/shared-worker1.js')
      worker2 = new SharedWorker('../fixtures/api/shared-worker/shared-worker2.js')
      worker1.port.start()
      worker2.port.start()
      vectorOfWorkers = []
    })

    after(() => {
      worker1.port.close()
      worker2.port.close()
    })

    it('can get multiple shared workers', () => {
      const contents = remote.getCurrentWebContents()
      vectorOfWorkers = contents.getAllSharedWorkers()

      expect(vectorOfWorkers.length).to.equal(2)
      expect(vectorOfWorkers[0].url).to.contain('shared-worker')
      expect(vectorOfWorkers[1].url).to.contain('shared-worker')
    })

    it('can inspect a specific shared worker', (done) => {
      const contents = webContents.getAllWebContents()
      contents[0].once('devtools-opened', () => {
        contents[0].closeDevTools()
      })
      contents[0].once('devtools-closed', () => {
        done()
      })
      contents[0].inspectSharedWorkerById(vectorOfWorkers[0].id)
    })
  })
})

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
})

import * as chai from 'chai'
import { AddressInfo } from 'net'
import * as chaiAsPromised from 'chai-as-promised'
import * as path from 'path'
import * as http from 'http'
import { BrowserWindow, ipcMain, webContents } from 'electron'
import { emittedOnce } from './events-helpers';
import { closeAllWindows } from './window-helpers';

const { expect } = chai

chai.use(chaiAsPromised)

const fixturesPath = path.resolve(__dirname, '..', 'spec', 'fixtures')

describe('webContents module', () => {
  describe('getAllWebContents() API', () => {
    afterEach(closeAllWindows)
    it('returns an array of web contents', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: { webviewTag: true }
      })
      w.loadFile(path.join(fixturesPath, 'pages', 'webview-zoom-factor.html'))

      await emittedOnce(w.webContents, 'did-attach-webview')

      w.webContents.openDevTools()

      await emittedOnce(w.webContents, 'devtools-opened')

      const all = webContents.getAllWebContents().sort((a, b) => {
        return a.id - b.id
      })

      expect(all).to.have.length(3)
      expect(all[0].getType()).to.equal('window')
      expect(all[all.length - 2].getType()).to.equal('webview')
      expect(all[all.length - 1].getType()).to.equal('remote')
    })
  })

  describe('will-prevent-unload event', () => {
    afterEach(closeAllWindows)
    it('does not emit if beforeunload returns undefined', (done) => {
      const w = new BrowserWindow({show: false})
      w.once('closed', () => done())
      w.webContents.once('will-prevent-unload', (e) => {
        expect.fail('should not have fired')
      })
      w.loadFile(path.join(fixturesPath, 'api', 'close-beforeunload-undefined.html'))
    })

    it('emits if beforeunload returns false', (done) => {
      const w = new BrowserWindow({show: false})
      w.webContents.once('will-prevent-unload', () => done())
      w.loadFile(path.join(fixturesPath, 'api', 'close-beforeunload-false.html'))
    })

    it('supports calling preventDefault on will-prevent-unload events', (done) => {
      const w = new BrowserWindow({show: false})
      w.webContents.once('will-prevent-unload', event => event.preventDefault())
      w.once('closed', () => done())
      w.loadFile(path.join(fixturesPath, 'api', 'close-beforeunload-false.html'))
    })
  })

  describe('webContents.send(channel, args...)', () => {
    afterEach(closeAllWindows)
    it('throws an error when the channel is missing', () => {
      const w = new BrowserWindow({show: false})
      expect(() => {
        (w.webContents.send as any)()
      }).to.throw('Missing required channel argument')

      expect(() => {
        w.webContents.send(null as any)
      }).to.throw('Missing required channel argument')
    })

    it('does not block node async APIs when sent before document is ready', (done) => {
      // Please reference https://github.com/electron/electron/issues/19368 if
      // this test fails.
      ipcMain.once('async-node-api-done', () => {
        done()
      })
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          sandbox: false,
          contextIsolation: false
        }
      })
      w.loadFile(path.join(fixturesPath, 'pages', 'send-after-node.html'))
      setTimeout(() => {
        w.webContents.send("test")
      }, 50)
    })
  })

  describe('webContents.print()', () => {
    it('throws when invalid settings are passed', () => {
      const w = new BrowserWindow({ show: false })
      expect(() => {
        // @ts-ignore this line is intentionally incorrect
        w.webContents.print(true)
      }).to.throw('webContents.print(): Invalid print settings specified.')

      expect(() => {
        // @ts-ignore this line is intentionally incorrect
        w.webContents.print({}, true)
      }).to.throw('webContents.print(): Invalid optional callback provided.')
    })

    it('does not crash', () => {
      const w = new BrowserWindow({ show: false })
      expect(() => {
        w.webContents.print({ silent: true })
      }).to.not.throw()
    })
  })

  describe('webContents.executeJavaScript', () => {
    describe('in about:blank', () => {
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
      let w: BrowserWindow

      before(async () => {
        w = new BrowserWindow({show: false})
        await w.loadURL('about:blank')
      })
      after(closeAllWindows)

      it('resolves the returned promise with the result', async () => {
        const result = await w.webContents.executeJavaScript(code)
        expect(result).to.equal(expected)
      })
      it('resolves the returned promise with the result if the code returns an asyncronous promise', async () => {
        const result = await w.webContents.executeJavaScript(asyncCode)
        expect(result).to.equal(expected)
      })
      it('rejects the returned promise if an async error is thrown', async () => {
        await expect(w.webContents.executeJavaScript(badAsyncCode)).to.eventually.be.rejectedWith(expectedErrorMsg)
      })
      it('rejects the returned promise with an error if an Error.prototype is thrown', async () => {
        for (const error of errorTypes) {
          await expect(w.webContents.executeJavaScript(`Promise.reject(new ${error.name}("Wamp-wamp"))`))
            .to.eventually.be.rejectedWith(error)
        }
      })
    })

    describe("on a real page", () => {
      let w: BrowserWindow
      beforeEach(() => {
        w = new BrowserWindow({show: false})
      })
      afterEach(closeAllWindows)

      let server: http.Server = null as unknown as http.Server
      let serverUrl: string = null as unknown as string

      before((done) => {
        server = http.createServer((request, response) => {
          response.end()
        }).listen(0, '127.0.0.1', () => {
          serverUrl = 'http://127.0.0.1:' + (server.address() as AddressInfo).port
          done()
        })
      })

      after(() => {
        server.close()
      })

      it('works after page load and during subframe load', (done) => {
        w.webContents.once('did-finish-load', () => {
          // initiate a sub-frame load, then try and execute script during it
          w.webContents.executeJavaScript(`
            var iframe = document.createElement('iframe')
            iframe.src = '${serverUrl}/slow'
            document.body.appendChild(iframe)
          `).then(() => {
            w.webContents.executeJavaScript('console.log(\'hello\')').then(() => {
              done()
            })
          })
        })
        w.loadURL(serverUrl)
      })

      it('executes after page load', (done) => {
        w.webContents.executeJavaScript(`(() => "test")()`).then(result => {
          expect(result).to.equal("test")
          done()
        })
        w.loadURL(serverUrl)
      })

      it('works with result objects that have DOM class prototypes', (done) => {
        w.webContents.executeJavaScript('document.location').then(result => {
          expect(result.origin).to.equal(serverUrl)
          expect(result.protocol).to.equal('http:')
          done()
        })
        w.loadURL(serverUrl)
      })
    })
  })

  describe('loadURL() promise API', () => {
    let w: BrowserWindow
    beforeEach(async () => {
      w = new BrowserWindow({show: false})
    })
    afterEach(closeAllWindows)

    it('resolves when done loading', async () => {
      await expect(w.loadURL('about:blank')).to.eventually.be.fulfilled()
    })

    it('resolves when done loading a file URL', async () => {
      await expect(w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'))).to.eventually.be.fulfilled()
    })

    it('rejects when failing to load a file URL', async () => {
      await expect(w.loadURL('file:non-existent')).to.eventually.be.rejected()
        .and.have.property('code', 'ERR_FILE_NOT_FOUND')
    })

    it('rejects when loading fails due to DNS not resolved', async () => {
      await expect(w.loadURL('https://err.name.not.resolved')).to.eventually.be.rejected()
        .and.have.property('code', 'ERR_NAME_NOT_RESOLVED')
    })

    it('rejects when navigation is cancelled due to a bad scheme', async () => {
      await expect(w.loadURL('bad-scheme://foo')).to.eventually.be.rejected()
        .and.have.property('code', 'ERR_FAILED')
    })

    it('sets appropriate error information on rejection', async () => {
      let err
      try {
        await w.loadURL('file:non-existent')
      } catch (e) {
        err = e
      }
      expect(err).not.to.be.null()
      expect(err.code).to.eql('ERR_FILE_NOT_FOUND')
      expect(err.errno).to.eql(-6)
      expect(err.url).to.eql(process.platform === 'win32' ? 'file://non-existent/' : 'file:///non-existent')
    })

    it('rejects if the load is aborted', async () => {
      const s = http.createServer((req, res) => { /* never complete the request */ })
      await new Promise(resolve => s.listen(0, '127.0.0.1', resolve))
      const { port } = s.address() as AddressInfo
      const p = expect(w.loadURL(`http://127.0.0.1:${port}`)).to.eventually.be.rejectedWith(Error, /ERR_ABORTED/)
      // load a different file before the first load completes, causing the
      // first load to be aborted.
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'))
      await p
      s.close()
    })

    it("doesn't reject when a subframe fails to load", async () => {
      let resp = null as unknown as http.ServerResponse
      const s = http.createServer((req, res) => {
        res.writeHead(200, { 'Content-Type': 'text/html' })
        res.write('<iframe src="http://err.name.not.resolved"></iframe>')
        resp = res
        // don't end the response yet
      })
      await new Promise(resolve => s.listen(0, '127.0.0.1', resolve))
      const { port } = s.address() as AddressInfo
      const p = new Promise(resolve => {
        w.webContents.on('did-fail-load', (event, errorCode, errorDescription, validatedURL, isMainFrame, frameProcessId, frameRoutingId) => {
          if (!isMainFrame) {
            resolve()
          }
        })
      })
      const main = w.loadURL(`http://127.0.0.1:${port}`)
      await p
      resp.end()
      await main
      s.close()
    })

    it("doesn't resolve when a subframe loads", async () => {
      let resp = null as unknown as http.ServerResponse
      const s = http.createServer((req, res) => {
        res.writeHead(200, { 'Content-Type': 'text/html' })
        res.write('<iframe src="data:text/html,hi"></iframe>')
        resp = res
        // don't end the response yet
      })
      await new Promise(resolve => s.listen(0, '127.0.0.1', resolve))
      const { port } = s.address() as AddressInfo
      const p = new Promise(resolve => {
        w.webContents.on('did-frame-finish-load', (event, isMainFrame, frameProcessId, frameRoutingId) => {
          if (!isMainFrame) {
            resolve()
          }
        })
      })
      const main = w.loadURL(`http://127.0.0.1:${port}`)
      await p
      resp.destroy() // cause the main request to fail
      await expect(main).to.eventually.be.rejected()
        .and.have.property('errno', -355) // ERR_INCOMPLETE_CHUNKED_ENCODING
      s.close()
    })
  })

  describe('getFocusedWebContents() API', () => {
    afterEach(closeAllWindows)
    it('returns the focused web contents', async () => {
      const w = new BrowserWindow({show: true})
      await w.loadURL('about:blank')
      expect(webContents.getFocusedWebContents().id).to.equal(w.id)

      const devToolsOpened = emittedOnce(w.webContents, 'devtools-opened')
      w.webContents.openDevTools()
      await devToolsOpened
      expect(webContents.getFocusedWebContents().id).to.equal(w.webContents.devToolsWebContents.id)
      const devToolsClosed = emittedOnce(w.webContents, 'devtools-closed')
      w.webContents.closeDevTools()
      await devToolsClosed
      expect(webContents.getFocusedWebContents().id).to.equal(w.webContents.id)
    })

    it('does not crash when called on a detached dev tools window', async () => {
      const w = new BrowserWindow({show: true})
      const devToolsOpened = emittedOnce(w.webContents, 'devtools-opened')
      w.webContents.openDevTools({ mode: 'detach' })
      w.webContents.inspectElement(100, 100)
      await devToolsOpened
      expect(webContents.getFocusedWebContents()).not.to.be.null()
      const devToolsClosed = emittedOnce(w.webContents, 'devtools-closed')
      w.webContents.closeDevTools()
      await devToolsClosed
      expect(webContents.getFocusedWebContents()).to.be.null()
    })
  })

  describe('setDevToolsWebContents() API', () => {
    afterEach(closeAllWindows)
    it('sets arbitrary webContents as devtools', async () => {
      const w = new BrowserWindow({ show: false })
      const devtools = new BrowserWindow({ show: false })
      const promise = emittedOnce(devtools.webContents, 'dom-ready')
      w.webContents.setDevToolsWebContents(devtools.webContents)
      w.webContents.openDevTools()
      await promise
      expect(devtools.webContents.getURL().startsWith('devtools://devtools')).to.be.true()
      const result = await devtools.webContents.executeJavaScript('InspectorFrontendHost.constructor.name')
      expect(result).to.equal('InspectorFrontendHostImpl')
      devtools.destroy()
    })
  })

  describe('isFocused() API', () => {
    it('returns false when the window is hidden', async () => {
      const w = new BrowserWindow({ show: false })
      await w.loadURL('about:blank')
      expect(w.isVisible()).to.be.false()
      expect(w.webContents.isFocused()).to.be.false()
    })
  })

  describe('isCurrentlyAudible() API', () => {
    afterEach(closeAllWindows)
    it('returns whether audio is playing', async () => {
      const w = new BrowserWindow({ show: false })
      await w.loadURL('about:blank')
      await w.webContents.executeJavaScript(`
        window.context = new AudioContext
        // Start in suspended state, because of the
        // new web audio api policy.
        context.suspend()
        window.oscillator = context.createOscillator()
        oscillator.connect(context.destination)
        oscillator.start()
      `)
      let p = emittedOnce(w.webContents, '-audio-state-changed')
      w.webContents.executeJavaScript('context.resume()')
      await p
      expect(w.webContents.isCurrentlyAudible()).to.be.true()
      p = emittedOnce(w.webContents, '-audio-state-changed')
      w.webContents.executeJavaScript('oscillator.stop()')
      await p
      expect(w.webContents.isCurrentlyAudible()).to.be.false()
    })
  })
})

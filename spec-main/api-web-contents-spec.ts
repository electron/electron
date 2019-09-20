import * as chai from 'chai'
import { AddressInfo } from 'net'
import * as chaiAsPromised from 'chai-as-promised'
import * as path from 'path'
import * as http from 'http'
import { BrowserWindow, ipcMain, webContents, session, clipboard } from 'electron'
import { emittedOnce } from './events-helpers'
import { closeAllWindows } from './window-helpers'
import { ifdescribe, ifit } from './spec-helpers'

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
    afterEach(closeAllWindows)
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

      it('works with result objects that have DOM class prototypes', async () => {
        w.loadURL(serverUrl)
        const result = await w.webContents.executeJavaScript('document.location')
        expect(result.origin).to.equal(serverUrl)
        expect(result.protocol).to.equal('http:')
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

    // Temporarily disable on WOA until
    // https://github.com/electron/electron/issues/20008 is resolved
    const testFn = (process.platform === 'win32' && process.arch === 'arm64' ? it.skip : it)
    testFn('rejects when loading fails due to DNS not resolved', async () => {
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

    const testFn = (process.platform === 'win32' && process.arch === 'arm64' ? it.skip : it)
    testFn('returns the focused web contents', async () => {
      const w = new BrowserWindow({show: true})
      await w.loadURL('about:blank')
      expect(webContents.getFocusedWebContents().id).to.equal(w.webContents.id)

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

      w.webContents.openDevTools({ mode: 'detach' })
      w.webContents.inspectElement(100, 100)

      // For some reason we have to wait for two focused events...?
      await emittedOnce(w.webContents, 'devtools-focused')
      await emittedOnce(w.webContents, 'devtools-focused')

      expect(() => { webContents.getFocusedWebContents() }).to.not.throw()

      // Work around https://github.com/electron/electron/issues/19985
      await new Promise(r => setTimeout(r, 0))

      const devToolsClosed = emittedOnce(w.webContents, 'devtools-closed')
      w.webContents.closeDevTools()
      await devToolsClosed
      expect(() => { webContents.getFocusedWebContents() }).to.not.throw()
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

  describe('getWebPreferences() API', () => {
    afterEach(closeAllWindows)
    it('should not crash when called for devTools webContents', (done) => {
      const w = new BrowserWindow({show: false})
      w.webContents.openDevTools()
      w.webContents.once('devtools-opened', () => {
        expect(w.webContents.devToolsWebContents.getWebPreferences()).to.be.null()
        done()
      })
    })
  })

  describe('openDevTools() API', () => {
    afterEach(closeAllWindows)
    it('can show window with activation', async () => {
      const w = new BrowserWindow({show: false})
      const focused = emittedOnce(w, 'focus')
      w.show()
      await focused
      expect(w.isFocused()).to.be.true()
      w.webContents.openDevTools({ mode: 'detach', activate: true })
      await emittedOnce(w.webContents, 'devtools-focused')
      await emittedOnce(w.webContents, 'devtools-focused')
      await emittedOnce(w.webContents, 'devtools-opened')
      await new Promise(resolve => setTimeout(resolve, 0))
      expect(w.isFocused()).to.be.false()
    })

    it('can show window without activation', async () => {
      const w = new BrowserWindow({show: false})
      const devtoolsOpened = emittedOnce(w.webContents, 'devtools-opened')
      w.webContents.openDevTools({ mode: 'detach', activate: false })
      await devtoolsOpened
      expect(w.webContents.isDevToolsOpened()).to.be.true()
    })
  })

  describe('before-input-event event', () => {
    afterEach(closeAllWindows)
    it('can prevent document keyboard events', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      await w.loadFile(path.join(fixturesPath, 'pages', 'key-events.html'))
      const keyDown = new Promise(resolve => {
        ipcMain.once('keydown', (event, key) => resolve(key))
      })
      w.webContents.once('before-input-event', (event, input) => {
        if ('a' === input.key) event.preventDefault()
      })
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'a' })
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'b' })
      expect(await keyDown).to.equal('b')
    })

    it('has the correct properties', async () => {
      const w = new BrowserWindow({show: false})
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'))
      const testBeforeInput = async (opts: any) => {
        const modifiers = []
        if (opts.shift) modifiers.push('shift')
        if (opts.control) modifiers.push('control')
        if (opts.alt) modifiers.push('alt')
        if (opts.meta) modifiers.push('meta')
        if (opts.isAutoRepeat) modifiers.push('isAutoRepeat')

        const p = emittedOnce(w.webContents, 'before-input-event')
        w.webContents.sendInputEvent({
          type: opts.type,
          keyCode: opts.keyCode,
          modifiers: modifiers as any
        })
        const [, input] = await p

        expect(input.type).to.equal(opts.type)
        expect(input.key).to.equal(opts.key)
        expect(input.code).to.equal(opts.code)
        expect(input.isAutoRepeat).to.equal(opts.isAutoRepeat)
        expect(input.shift).to.equal(opts.shift)
        expect(input.control).to.equal(opts.control)
        expect(input.alt).to.equal(opts.alt)
        expect(input.meta).to.equal(opts.meta)
      }
      await testBeforeInput({
        type: 'keyDown',
        key: 'A',
        code: 'KeyA',
        keyCode: 'a',
        shift: true,
        control: true,
        alt: true,
        meta: true,
        isAutoRepeat: true
      })
      await testBeforeInput({
        type: 'keyUp',
        key: '.',
        code: 'Period',
        keyCode: '.',
        shift: false,
        control: true,
        alt: true,
        meta: false,
        isAutoRepeat: false
      })
      await testBeforeInput({
        type: 'keyUp',
        key: '!',
        code: 'Digit1',
        keyCode: '1',
        shift: true,
        control: false,
        alt: false,
        meta: true,
        isAutoRepeat: false
      })
      await testBeforeInput({
        type: 'keyUp',
        key: 'Tab',
        code: 'Tab',
        keyCode: 'Tab',
        shift: false,
        control: true,
        alt: false,
        meta: false,
        isAutoRepeat: true
      })
    })
  })

  // On Mac, zooming isn't done with the mouse wheel.
  ifdescribe(process.platform !== 'darwin')('zoom-changed', () => {
    afterEach(closeAllWindows)
    it('is emitted with the correct zooming info', async () => {
      const w = new BrowserWindow({ show: false })
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'))

      const testZoomChanged = async ({ zoomingIn }: { zoomingIn: boolean }) => {
        w.webContents.sendInputEvent({
          type: 'mouseWheel',
          x: 300,
          y: 300,
          deltaX: 0,
          deltaY: zoomingIn ? 1 : -1,
          wheelTicksX: 0,
          wheelTicksY: zoomingIn ? 1 : -1,
          modifiers: ['control', 'meta']
        })

        const [, zoomDirection] = await emittedOnce(w.webContents, 'zoom-changed')
        expect(zoomDirection).to.equal(zoomingIn ? 'in' : 'out')
        // Apparently we get two zoom-changed events??
        await emittedOnce(w.webContents, 'zoom-changed')
      }

      await testZoomChanged({ zoomingIn: true })
      await testZoomChanged({ zoomingIn: false })
    })
  })

  describe('sendInputEvent(event)', () => {
    let w: BrowserWindow
    beforeEach(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      await w.loadFile(path.join(fixturesPath, 'pages', 'key-events.html'))
    })
    afterEach(closeAllWindows)

    it('can send keydown events', (done) => {
      ipcMain.once('keydown', (event, key, code, keyCode, shiftKey, ctrlKey, altKey) => {
        expect(key).to.equal('a')
        expect(code).to.equal('KeyA')
        expect(keyCode).to.equal(65)
        expect(shiftKey).to.be.false()
        expect(ctrlKey).to.be.false()
        expect(altKey).to.be.false()
        done()
      })
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'A' })
    })

    it('can send keydown events with modifiers', (done) => {
      ipcMain.once('keydown', (event, key, code, keyCode, shiftKey, ctrlKey, altKey) => {
        expect(key).to.equal('Z')
        expect(code).to.equal('KeyZ')
        expect(keyCode).to.equal(90)
        expect(shiftKey).to.be.true()
        expect(ctrlKey).to.be.true()
        expect(altKey).to.be.false()
        done()
      })
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Z', modifiers: ['shift', 'ctrl'] })
    })

    it('can send keydown events with special keys', (done) => {
      ipcMain.once('keydown', (event, key, code, keyCode, shiftKey, ctrlKey, altKey) => {
        expect(key).to.equal('Tab')
        expect(code).to.equal('Tab')
        expect(keyCode).to.equal(9)
        expect(shiftKey).to.be.false()
        expect(ctrlKey).to.be.false()
        expect(altKey).to.be.true()
        done()
      })
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Tab', modifiers: ['alt'] })
    })

    it('can send char events', (done) => {
      ipcMain.once('keypress', (event, key, code, keyCode, shiftKey, ctrlKey, altKey) => {
        expect(key).to.equal('a')
        expect(code).to.equal('KeyA')
        expect(keyCode).to.equal(65)
        expect(shiftKey).to.be.false()
        expect(ctrlKey).to.be.false()
        expect(altKey).to.be.false()
        done()
      })
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'A' })
      w.webContents.sendInputEvent({ type: 'char', keyCode: 'A' })
    })

    it('can send char events with modifiers', (done) => {
      ipcMain.once('keypress', (event, key, code, keyCode, shiftKey, ctrlKey, altKey) => {
        expect(key).to.equal('Z')
        expect(code).to.equal('KeyZ')
        expect(keyCode).to.equal(90)
        expect(shiftKey).to.be.true()
        expect(ctrlKey).to.be.true()
        expect(altKey).to.be.false()
        done()
      })
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Z' })
      w.webContents.sendInputEvent({ type: 'char', keyCode: 'Z', modifiers: ['shift', 'ctrl'] })
    })
  })

  describe('insertCSS', () => {
    afterEach(closeAllWindows)
    it('supports inserting CSS', async () => {
      const w = new BrowserWindow({ show: false })
      w.loadURL('about:blank')
      await w.webContents.insertCSS('body { background-repeat: round; }')
      const result = await w.webContents.executeJavaScript('window.getComputedStyle(document.body).getPropertyValue("background-repeat")')
      expect(result).to.equal('round')
    })

    it('supports removing inserted CSS', async () => {
      const w = new BrowserWindow({ show: false })
      w.loadURL('about:blank')
      const key = await w.webContents.insertCSS('body { background-repeat: round; }')
      await w.webContents.removeInsertedCSS(key)
      const result = await w.webContents.executeJavaScript('window.getComputedStyle(document.body).getPropertyValue("background-repeat")')
      expect(result).to.equal('repeat')
    })
  })

  describe('inspectElement()', () => {
    afterEach(closeAllWindows)
    it('supports inspecting an element in the devtools', (done) => {
      const w = new BrowserWindow({ show: false })
      w.loadURL('about:blank')
      w.webContents.once('devtools-opened', () => { done() })
      w.webContents.inspectElement(10, 10)
    })
  })

  describe('startDrag({file, icon})', () => {
    it('throws errors for a missing file or a missing/empty icon', () => {
      const w = new BrowserWindow({ show: false })
      expect(() => {
        w.webContents.startDrag({ icon: path.join(fixturesPath, 'assets', 'logo.png') } as any)
      }).to.throw(`Must specify either 'file' or 'files' option`)

      expect(() => {
        w.webContents.startDrag({ file: __filename } as any)
      }).to.throw(`Must specify 'icon' option`)

      if (process.platform === 'darwin') {
        expect(() => {
          w.webContents.startDrag({ file: __filename, icon: __filename })
        }).to.throw(`Must specify non-empty 'icon' option`)
      }
    })
  })

  describe('focus()', () => {
    describe('when the web contents is hidden', () => {
      afterEach(closeAllWindows)
      it('does not blur the focused window', (done) => {
        const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
        ipcMain.once('answer', (event, parentFocused, childFocused) => {
          expect(parentFocused).to.be.true()
          expect(childFocused).to.be.false()
          done()
        })
        w.show()
        w.loadFile(path.join(fixturesPath, 'pages', 'focus-web-contents.html'))
      })
    })
  })

  describe('getOSProcessId()', () => {
    afterEach(closeAllWindows)
    it('returns a valid procress id', async () => {
      const w = new BrowserWindow({ show: false })
      expect(w.webContents.getOSProcessId()).to.equal(0)

      await w.loadURL('about:blank')
      expect(w.webContents.getOSProcessId()).to.be.above(0)
    })
  })

  describe('zoom api', () => {
    const scheme = (global as any).standardScheme
    const hostZoomMap: Record<string, number> = {
      host1: 0.3,
      host2: 0.7,
      host3: 0.2
    }

    before((done) => {
      const protocol = session.defaultSession.protocol
      protocol.registerStringProtocol(scheme, (request, callback) => {
        const response = `<script>
                            const {ipcRenderer, remote} = require('electron')
                            ipcRenderer.send('set-zoom', window.location.hostname)
                            ipcRenderer.on(window.location.hostname + '-zoom-set', () => {
                              const { zoomLevel } = remote.getCurrentWebContents()
                              ipcRenderer.send(window.location.hostname + '-zoom-level', zoomLevel)
                            })
                          </script>`
        callback({ data: response, mimeType: 'text/html' })
      }, (error) => done(error))
    })

    after((done) => {
      const protocol = session.defaultSession.protocol
      protocol.unregisterProtocol(scheme, (error) => done(error))
    })

    afterEach(closeAllWindows)

    // TODO(codebytere): remove in Electron v8.0.0
    it('can set the correct zoom level (functions)', async () => {
      const w = new BrowserWindow({ show: false })
      try {
        await w.loadURL('about:blank')
        const zoomLevel = w.webContents.getZoomLevel()
        expect(zoomLevel).to.eql(0.0)
        w.webContents.setZoomLevel(0.5)
        const newZoomLevel = w.webContents.getZoomLevel()
        expect(newZoomLevel).to.eql(0.5)
      } finally {
        w.webContents.setZoomLevel(0)
      }
    })

    it('can set the correct zoom level', async () => {
      const w = new BrowserWindow({ show: false })
      try {
        await w.loadURL('about:blank')
        const zoomLevel = w.webContents.zoomLevel
        expect(zoomLevel).to.eql(0.0)
        w.webContents.zoomLevel = 0.5
        const newZoomLevel = w.webContents.zoomLevel
        expect(newZoomLevel).to.eql(0.5)
      } finally {
        w.webContents.zoomLevel = 0
      }
    })

    it('can persist zoom level across navigation', (done) => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      let finalNavigation = false
      ipcMain.on('set-zoom', (e, host) => {
        const zoomLevel = hostZoomMap[host]
        if (!finalNavigation) w.webContents.zoomLevel = zoomLevel
        e.sender.send(`${host}-zoom-set`)
      })
      ipcMain.on('host1-zoom-level', (e, zoomLevel) => {
        const expectedZoomLevel = hostZoomMap.host1
        expect(zoomLevel).to.equal(expectedZoomLevel)
        if (finalNavigation) {
          done()
        } else {
          w.loadURL(`${scheme}://host2`)
        }
      })
      ipcMain.once('host2-zoom-level', (e, zoomLevel) => {
        const expectedZoomLevel = hostZoomMap.host2
        expect(zoomLevel).to.equal(expectedZoomLevel)
        finalNavigation = true
        w.webContents.goBack()
      })
      w.loadURL(`${scheme}://host1`)
    })

    it('can propagate zoom level across same session', (done) => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      const w2 = new BrowserWindow({ show: false })
      w2.webContents.on('did-finish-load', () => {
        const zoomLevel1 = w.webContents.zoomLevel
        expect(zoomLevel1).to.equal(hostZoomMap.host3)

        const zoomLevel2 = w2.webContents.zoomLevel
        expect(zoomLevel1).to.equal(zoomLevel2)
        w2.setClosable(true)
        w2.close()
        done()
      })
      w.webContents.on('did-finish-load', () => {
        w.webContents.zoomLevel = hostZoomMap.host3
        w2.loadURL(`${scheme}://host3`)
      })
      w.loadURL(`${scheme}://host3`)
    })

    it('cannot propagate zoom level across different session', (done) => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      const w2 = new BrowserWindow({
        show: false,
        webPreferences: {
          partition: 'temp'
        }
      })
      const protocol = w2.webContents.session.protocol
      protocol.registerStringProtocol(scheme, (request, callback) => {
        callback('hello')
      }, (error) => {
        if (error) return done(error)
        w2.webContents.on('did-finish-load', () => {
          const zoomLevel1 = w.webContents.zoomLevel
          expect(zoomLevel1).to.equal(hostZoomMap.host3)

          const zoomLevel2 = w2.webContents.zoomLevel
          expect(zoomLevel2).to.equal(0)
          expect(zoomLevel1).to.not.equal(zoomLevel2)

          protocol.unregisterProtocol(scheme, (error) => {
            if (error) return done(error)
            w2.setClosable(true)
            w2.close()
            done()
          })
        })
        w.webContents.on('did-finish-load', () => {
          w.webContents.zoomLevel = hostZoomMap.host3
          w2.loadURL(`${scheme}://host3`)
        })
        w.loadURL(`${scheme}://host3`)
      })
    })

    it('can persist when it contains iframe', (done) => {
      const w = new BrowserWindow({ show: false })
      const server = http.createServer((req, res) => {
        setTimeout(() => {
          res.end()
        }, 200)
      })
      server.listen(0, '127.0.0.1', () => {
        const url = 'http://127.0.0.1:' + (server.address() as AddressInfo).port
        const content = `<iframe src=${url}></iframe>`
        w.webContents.on('did-frame-finish-load', (e, isMainFrame) => {
          if (!isMainFrame) {
            const zoomLevel = w.webContents.zoomLevel
            expect(zoomLevel).to.equal(2.0)

            w.webContents.zoomLevel = 0
            server.close()
            done()
          }
        })
        w.webContents.on('dom-ready', () => {
          w.webContents.zoomLevel = 2.0
        })
        w.loadURL(`data:text/html,${content}`)
      })
    })

    it('cannot propagate when used with webframe', (done) => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      let finalZoomLevel = 0
      const w2 = new BrowserWindow({
        show: false
      })
      w2.webContents.on('did-finish-load', () => {
        const zoomLevel1 = w.webContents.zoomLevel
        expect(zoomLevel1).to.equal(finalZoomLevel)

        const zoomLevel2 = w2.webContents.zoomLevel
        expect(zoomLevel2).to.equal(0)
        expect(zoomLevel1).to.not.equal(zoomLevel2)

        w2.setClosable(true)
        w2.close()
        done()
      })
      ipcMain.once('temporary-zoom-set', (e, zoomLevel) => {
        w2.loadFile(path.join(fixturesPath, 'pages', 'c.html'))
        finalZoomLevel = zoomLevel
      })
      w.loadFile(path.join(fixturesPath, 'pages', 'webframe-zoom.html'))
    })

    it('cannot persist zoom level after navigation with webFrame', (done) => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      let initialNavigation = true
      const source = `
        const {ipcRenderer, webFrame} = require('electron')
        webFrame.setZoomLevel(0.6)
        ipcRenderer.send('zoom-level-set', webFrame.getZoomLevel())
      `
      w.webContents.on('did-finish-load', () => {
        if (initialNavigation) {
          w.webContents.executeJavaScript(source)
        } else {
          const zoomLevel = w.webContents.zoomLevel
          expect(zoomLevel).to.equal(0)
          done()
        }
      })
      ipcMain.once('zoom-level-set', (e, zoomLevel) => {
        expect(zoomLevel).to.equal(0.6)
        w.loadFile(path.join(fixturesPath, 'pages', 'd.html'))
        initialNavigation = false
      })
      w.loadFile(path.join(fixturesPath, 'pages', 'c.html'))
    })
  })

  describe('devtools window', () => {
    let hasRobotJS = false
    try {
      // We have other tests that check if native modules work, if we fail to require
      // robotjs let's skip this test to avoid false negatives
      require('robotjs')
      hasRobotJS = true
    } catch (err) { /* no-op */ }

    afterEach(closeAllWindows)

    // NB. on macOS, this requires that you grant your terminal the ability to
    // control your computer. Open System Preferences > Security & Privacy >
    // Privacy > Accessibility and grant your terminal the permission to control
    // your computer.
    ifit(hasRobotJS)('can receive and handle menu events', async () => {
      const w = new BrowserWindow({ show: true, webPreferences: { nodeIntegration: true } })
      w.loadFile(path.join(fixturesPath, 'pages', 'key-events.html'))

      // Ensure the devtools are loaded
      w.webContents.closeDevTools()
      const opened = emittedOnce(w.webContents, 'devtools-opened')
      w.webContents.openDevTools()
      await opened
      await emittedOnce(w.webContents.devToolsWebContents, 'did-finish-load')
      w.webContents.devToolsWebContents.focus()

      // Focus an input field
      await w.webContents.devToolsWebContents.executeJavaScript(`
        const input = document.createElement('input')
        document.body.innerHTML = ''
        document.body.appendChild(input)
        input.focus()
      `)

      // Write something to the clipboard
      clipboard.writeText('test value')

      const pasted = w.webContents.devToolsWebContents.executeJavaScript(`new Promise(resolve => {
        document.querySelector('input').addEventListener('paste', (e) => {
          resolve(e.target.value)
        })
      })`)

      // Fake a paste request using robotjs to emulate a REAL keyboard paste event
      require('robotjs').keyTap('v', process.platform === 'darwin' ? ['command'] : ['control'])

      const val = await pasted

      // Once we're done expect the paste to have been successful
      expect(val).to.equal('test value', 'value should eventually become the pasted value')
    })
  })
})

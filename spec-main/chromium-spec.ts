import * as chai from 'chai'
import { expect } from 'chai'
import * as chaiAsPromised from 'chai-as-promised'
import { BrowserWindow, WebContents, session, ipcMain, app } from 'electron'
import { emittedOnce } from './events-helpers'
import { closeAllWindows } from './window-helpers'
import * as https from 'https'
import * as http from 'http'
import * as path from 'path'
import * as fs from 'fs'
import * as url from 'url'
import * as ChildProcess from 'child_process'
import { EventEmitter } from 'events'

const features = process.electronBinding('features')

chai.use(chaiAsPromised)
const fixturesPath = path.resolve(__dirname, '..', 'spec', 'fixtures')

describe('reporting api', () => {
  it('sends a report for a deprecation', async () => {
    const reports = new EventEmitter

    // The Reporting API only works on https with valid certs. To dodge having
    // to set up a trusted certificate, hack the validator.
    session.defaultSession.setCertificateVerifyProc((req, cb) => {
      cb(0)
    })
    const certPath = path.join(fixturesPath, 'certificates')
    const options = {
      key: fs.readFileSync(path.join(certPath, 'server.key')),
      cert: fs.readFileSync(path.join(certPath, 'server.pem')),
      ca: [
        fs.readFileSync(path.join(certPath, 'rootCA.pem')),
        fs.readFileSync(path.join(certPath, 'intermediateCA.pem'))
      ],
      requestCert: true,
      rejectUnauthorized: false
    }

    const server = https.createServer(options, (req, res) => {
      if (req.url === '/report') {
        let data = ''
        req.on('data', (d) => data += d.toString('utf-8'))
        req.on('end', () => {
          reports.emit('report', JSON.parse(data))
        })
      }
      res.setHeader('Report-To', JSON.stringify({
        group: 'default',
        max_age: 120,
        endpoints: [ {url: `https://localhost:${(server.address() as any).port}/report`} ],
      }))
      res.setHeader('Content-Type', 'text/html')
      // using the deprecated `webkitRequestAnimationFrame` will trigger a
      // "deprecation" report.
      res.end('<script>webkitRequestAnimationFrame(() => {})</script>')
    })
    await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
    const bw = new BrowserWindow({
      show: false,
    })
    try {
      const reportGenerated = emittedOnce(reports, 'report')
      const url = `https://localhost:${(server.address() as any).port}/a`
      await bw.loadURL(url)
      const [report] = await reportGenerated
      expect(report).to.be.an('array')
      expect(report[0].type).to.equal('deprecation')
      expect(report[0].url).to.equal(url)
      expect(report[0].body.id).to.equal('PrefixedRequestAnimationFrame')
    } finally {
      bw.destroy()
      server.close()
    }
  })
})

describe('window.postMessage', () => {
  afterEach(async () => {
    await closeAllWindows()
  })

  it('sets the source and origin correctly', async () => {
    const w = new BrowserWindow({show: false, webPreferences: {nodeIntegration: true}})
    w.loadURL(`file://${fixturesPath}/pages/window-open-postMessage-driver.html`)
    const [, message] = await emittedOnce(ipcMain, 'complete')
    expect(message.data).to.equal('testing')
    expect(message.origin).to.equal('file://')
    expect(message.sourceEqualsOpener).to.equal(true)
    expect(message.eventOrigin).to.equal('file://')
  })
})

describe('focus handling', () => {
  let webviewContents: WebContents = null as unknown as WebContents
  let w: BrowserWindow = null as unknown as BrowserWindow

  beforeEach(async () => {
    w = new BrowserWindow({
      show: true,
      webPreferences: {
        nodeIntegration: true,
        webviewTag: true
      }
    })

    const webviewReady = emittedOnce(w.webContents, 'did-attach-webview')
    await w.loadFile(path.join(fixturesPath, 'pages', 'tab-focus-loop-elements.html'))
    const [, wvContents] = await webviewReady
    webviewContents = wvContents
    await emittedOnce(webviewContents, 'did-finish-load')
    w.focus()
  })

  afterEach(() => {
    webviewContents = null as unknown as WebContents
    w.destroy()
    w = null as unknown as BrowserWindow
  })

  const expectFocusChange = async () => {
    const [, focusedElementId] = await emittedOnce(ipcMain, 'focus-changed')
    return focusedElementId
  }

  describe('a TAB press', () => {
    const tabPressEvent: any = {
      type: 'keyDown',
      keyCode: 'Tab'
    }

    it('moves focus to the next focusable item', async () => {
      let focusChange = expectFocusChange()
      w.webContents.sendInputEvent(tabPressEvent)
      let focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-element-1', `should start focused in element-1, it's instead in ${focusedElementId}`)

      focusChange = expectFocusChange()
      w.webContents.sendInputEvent(tabPressEvent)
      focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-element-2', `focus should've moved to element-2, it's instead in ${focusedElementId}`)

      focusChange = expectFocusChange()
      w.webContents.sendInputEvent(tabPressEvent)
      focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-wv-element-1', `focus should've moved to the webview's element-1, it's instead in ${focusedElementId}`)

      focusChange = expectFocusChange()
      webviewContents.sendInputEvent(tabPressEvent)
      focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-wv-element-2', `focus should've moved to the webview's element-2, it's instead in ${focusedElementId}`)

      focusChange = expectFocusChange()
      webviewContents.sendInputEvent(tabPressEvent)
      focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-element-3', `focus should've moved to element-3, it's instead in ${focusedElementId}`)

      focusChange = expectFocusChange()
      w.webContents.sendInputEvent(tabPressEvent)
      focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-element-1', `focus should've looped back to element-1, it's instead in ${focusedElementId}`)
    })
  })

  describe('a SHIFT + TAB press', () => {
    const shiftTabPressEvent: any = {
      type: 'keyDown',
      modifiers: ['Shift'],
      keyCode: 'Tab'
    }

    it('moves focus to the previous focusable item', async () => {
      let focusChange = expectFocusChange()
      w.webContents.sendInputEvent(shiftTabPressEvent)
      let focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-element-3', `should start focused in element-3, it's instead in ${focusedElementId}`)

      focusChange = expectFocusChange()
      w.webContents.sendInputEvent(shiftTabPressEvent)
      focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-wv-element-2', `focus should've moved to the webview's element-2, it's instead in ${focusedElementId}`)

      focusChange = expectFocusChange()
      webviewContents.sendInputEvent(shiftTabPressEvent)
      focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-wv-element-1', `focus should've moved to the webview's element-1, it's instead in ${focusedElementId}`)

      focusChange = expectFocusChange()
      webviewContents.sendInputEvent(shiftTabPressEvent)
      focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-element-2', `focus should've moved to element-2, it's instead in ${focusedElementId}`)

      focusChange = expectFocusChange()
      w.webContents.sendInputEvent(shiftTabPressEvent)
      focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-element-1', `focus should've moved to element-1, it's instead in ${focusedElementId}`)

      focusChange = expectFocusChange()
      w.webContents.sendInputEvent(shiftTabPressEvent)
      focusedElementId = await focusChange
      expect(focusedElementId).to.equal('BUTTON-element-3', `focus should've looped back to element-3, it's instead in ${focusedElementId}`)
    })
  })
})

describe('web security', () => {
  afterEach(closeAllWindows)
  let server: http.Server
  let serverUrl: string
  before(async () => {
    server = http.createServer((req, res) => {
      res.setHeader('Content-Type', 'text/html')
      res.end('<body>')
    })
    await new Promise(resolve => server.listen(0, '127.0.0.1', resolve))
    serverUrl = `http://localhost:${(server.address() as any).port}`
  })
  after(() => {
    server.close()
  })

  it('engages CORB when web security is not disabled', async () => {
    const w = new BrowserWindow({ show: true, webPreferences: { webSecurity: true, nodeIntegration: true } })
    const p = emittedOnce(ipcMain, 'success')
    await w.loadURL(`data:text/html,<script>
        const s = document.createElement('script')
        s.src = "${serverUrl}"
        // The script will load successfully but its body will be emptied out
        // by CORB, so we don't expect a syntax error.
        s.onload = () => { require('electron').ipcRenderer.send('success') }
        document.documentElement.appendChild(s)
      </script>`)
    await p
  })

  it('bypasses CORB when web security is disabled', async () => {
    const w = new BrowserWindow({ show: true, webPreferences: { webSecurity: false, nodeIntegration: true } })
    const p = emittedOnce(ipcMain, 'success')
    await w.loadURL(`data:text/html,
      <script>
        window.onerror = (e) => { require('electron').ipcRenderer.send('success', e) }
      </script>
      <script src="${serverUrl}"></script>`)
    await p
  })
})

describe('command line switches', () => {
  describe('--lang switch', () => {
    const currentLocale = app.getLocale()
    const testLocale = (locale: string, result: string, done: () => void) => {
      const appPath = path.join(fixturesPath, 'api', 'locale-check')
      const electronPath = process.execPath
      let output = ''
      const appProcess = ChildProcess.spawn(electronPath, [appPath, `--lang=${locale}`])

      appProcess.stdout.on('data', (data) => { output += data })
      appProcess.stdout.on('end', () => {
        output = output.replace(/(\r\n|\n|\r)/gm, '')
        expect(output).to.equal(result)
        done()
      })
    }

    it('should set the locale', (done) => testLocale('fr', 'fr', done))
    it('should not set an invalid locale', (done) => testLocale('asdfkl', currentLocale, done))
  })

  describe('--remote-debugging-port switch', () => {
    it('should display the discovery page', (done) => {
      const electronPath = process.execPath
      let output = ''
      const appProcess = ChildProcess.spawn(electronPath, [`--remote-debugging-port=`])

      appProcess.stderr.on('data', (data) => {
        output += data
        const m = /DevTools listening on ws:\/\/127.0.0.1:(\d+)\//.exec(output)
        if (m) {
          appProcess.stderr.removeAllListeners('data')
          const port = m[1]
          http.get(`http://127.0.0.1:${port}`, (res) => {
            res.destroy()
            appProcess.kill()
            expect(res.statusCode).to.eql(200)
            expect(parseInt(res.headers['content-length']!)).to.be.greaterThan(0)
            done()
          })
        }
      })
    })
  })
})

describe('chromium features', () => {
  afterEach(closeAllWindows)

  describe('accessing key names also used as Node.js module names', () => {
    it('does not crash', (done) => {
      const w = new BrowserWindow({ show: false })
      w.webContents.once('did-finish-load', () => { done() })
      w.webContents.once('crashed', () => done(new Error('WebContents crashed.')))
      w.loadFile(path.join(fixturesPath, 'pages', 'external-string.html'))
    })
  })

  describe('loading jquery', () => {
    it('does not crash', (done) => {
      const w = new BrowserWindow({ show: false })
      w.webContents.once('did-finish-load', () => { done() })
      w.webContents.once('crashed', () => done(new Error('WebContents crashed.')))
      w.loadFile(path.join(fixturesPath, 'pages', 'jquery.html'))
    })
  })

  describe('navigator.languages', () => {
    it('should return the system locale only', async () => {
      const appLocale = app.getLocale()
      const w = new BrowserWindow({ show: false })
      await w.loadURL('about:blank')
      const languages = await w.webContents.executeJavaScript(`navigator.languages`)
      expect(languages).to.deep.equal([appLocale])
    })
  })

  // FIXME(robo/nornagon): re-enable these once service workers work
  describe('navigator.serviceWorker', () => {
    it('should register for file scheme', (done) => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          partition: 'sw-file-scheme-spec'
        }
      })
      w.webContents.on('ipc-message', (event, channel, message) => {
        if (channel === 'reload') {
          w.webContents.reload()
        } else if (channel === 'error') {
          done(message)
        } else if (channel === 'response') {
          expect(message).to.equal('Hello from serviceWorker!')
          session.fromPartition('sw-file-scheme-spec').clearStorageData({
            storages: ['serviceworkers']
          }).then(() => done())
        }
      })
      w.webContents.on('crashed', () => done(new Error('WebContents crashed.')))
      w.loadFile(path.join(fixturesPath, 'pages', 'service-worker', 'index.html'))
    })

    it('should register for intercepted file scheme', (done) => {
      const customSession = session.fromPartition('intercept-file')
      customSession.protocol.interceptBufferProtocol('file', (request, callback) => {
        let file = url.parse(request.url).pathname!
        if (file[0] === '/' && process.platform === 'win32') file = file.slice(1)

        const content = fs.readFileSync(path.normalize(file))
        const ext = path.extname(file)
        let type = 'text/html'

        if (ext === '.js') type = 'application/javascript'
        callback({ data: content, mimeType: type } as any)
      }, (error) => {
        if (error) done(error)
      })

      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          session: customSession
        }
      })
      w.webContents.on('ipc-message', (event, channel, message) => {
        if (channel === 'reload') {
          w.webContents.reload()
        } else if (channel === 'error') {
          done(`unexpected error : ${message}`)
        } else if (channel === 'response') {
          expect(message).to.equal('Hello from serviceWorker!')
          customSession.clearStorageData({
            storages: ['serviceworkers']
          }).then(() => {
            customSession.protocol.uninterceptProtocol('file', error => done(error))
          })
        }
      })
      w.webContents.on('crashed', () => done(new Error('WebContents crashed.')))
      w.loadFile(path.join(fixturesPath, 'pages', 'service-worker', 'index.html'))
    })
  })

  describe('navigator.geolocation', () => {
    before(function () {
      if (!features.isFakeLocationProviderEnabled()) {
        return this.skip()
      }
    })

    it('returns error when permission is denied', (done) => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          partition: 'geolocation-spec'
        }
      })
      w.webContents.on('ipc-message', (event, channel) => {
        if (channel === 'success') {
          done()
        } else {
          done('unexpected response from geolocation api')
        }
      })
      w.webContents.session.setPermissionRequestHandler((wc, permission, callback) => {
        if (permission === 'geolocation') {
          callback(false)
        } else {
          callback(true)
        }
      })
      w.loadFile(path.join(fixturesPath, 'pages', 'geolocation', 'index.html'))
    })
  })

  describe('window.open', () => {
    for (const show of [true, false]) {
      it(`inherits parent visibility over parent {show=${show}} option`, (done) => {
        const w = new BrowserWindow({ show })

        // toggle visibility
        if (show) {
          w.hide()
        } else {
          w.show()
        }

        w.webContents.once('new-window', (e, url, frameName, disposition, options) => {
          expect(options.show).to.equal(w.isVisible())
          w.close()
          done()
        })
        w.loadFile(path.join(fixturesPath, 'pages', 'window-open.html'))
      })
    }

    it('disables node integration when it is disabled on the parent window for chrome devtools URLs', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      w.loadURL('about:blank')
      w.webContents.executeJavaScript(`
        b = window.open('devtools://devtools/bundled/inspector.html', '', 'nodeIntegration=no,show=no')
      `)
      const [, contents] = await emittedOnce(app, 'web-contents-created')
      const typeofProcessGlobal = await contents.executeJavaScript('typeof process')
      expect(typeofProcessGlobal).to.equal('undefined')
    })

    it('disables JavaScript when it is disabled on the parent window', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
      w.webContents.loadURL('about:blank')
      const windowUrl = require('url').format({
        pathname: `${fixturesPath}/pages/window-no-javascript.html`,
        protocol: 'file',
        slashes: true
      })
      w.webContents.executeJavaScript(`
        b = window.open(${JSON.stringify(windowUrl)}, '', 'javascript=no,show=no')
      `)
      const [, contents] = await emittedOnce(app, 'web-contents-created')
      await emittedOnce(contents, 'did-finish-load')
      // Click link on page
      contents.sendInputEvent({ type: 'mouseDown', clickCount: 1, x: 1, y: 1 })
      contents.sendInputEvent({ type: 'mouseUp', clickCount: 1, x: 1, y: 1 })
      const [, window] = await emittedOnce(app, 'browser-window-created')
      const preferences = (window.webContents as any).getLastWebPreferences()
      expect(preferences.javascript).to.be.false()
    })

    it('handles cycles when merging the parent options into the child options', (done) => {
      const foo = {} as any
      foo.bar = foo
      foo.baz = {
        hello: {
          world: true
        }
      }
      foo.baz2 = foo.baz
      const w = new BrowserWindow({ show: false, foo: foo } as any)

      w.loadFile(path.join(fixturesPath, 'pages', 'window-open.html'))
      w.webContents.once('new-window', (event, url, frameName, disposition, options) => {
        expect(options.show).to.be.false()
        expect((options as any).foo).to.deep.equal({
          bar: undefined,
          baz: {
            hello: {
              world: true
            }
          },
          baz2: {
            hello: {
              world: true
            }
          }
        })
        done()
      })
    })

    it('defines a window.location getter', async () => {
      let targetURL: string
      if (process.platform === 'win32') {
        targetURL = `file:///${fixturesPath.replace(/\\/g, '/')}/pages/base-page.html`
      } else {
        targetURL = `file://${fixturesPath}/pages/base-page.html`
      }
      const w = new BrowserWindow({ show: false })
      w.loadURL('about:blank')
      w.webContents.executeJavaScript(`b = window.open(${JSON.stringify(targetURL)})`)
      const [, window] = await emittedOnce(app, 'browser-window-created')
      await emittedOnce(window.webContents, 'did-finish-load')
      expect(await w.webContents.executeJavaScript(`b.location.href`)).to.equal(targetURL)
    })

    it('defines a window.location setter', async () => {
      const w = new BrowserWindow({ show: false })
      w.loadURL('about:blank')
      w.webContents.executeJavaScript(`b = window.open("about:blank")`)
      const [, { webContents }] = await emittedOnce(app, 'browser-window-created')
      await emittedOnce(webContents, 'did-finish-load')
      // When it loads, redirect
      w.webContents.executeJavaScript(`b.location = ${JSON.stringify(`file://${fixturesPath}/pages/base-page.html`)}`)
      await emittedOnce(webContents, 'did-finish-load')
    })

    it('defines a window.location.href setter', async () => {
      const w = new BrowserWindow({ show: false })
      w.loadURL('about:blank')
      w.webContents.executeJavaScript(`b = window.open("about:blank")`)
      const [, { webContents }] = await emittedOnce(app, 'browser-window-created')
      await emittedOnce(webContents, 'did-finish-load')
      // When it loads, redirect
      w.webContents.executeJavaScript(`b.location.href = ${JSON.stringify(`file://${fixturesPath}/pages/base-page.html`)}`)
      await emittedOnce(webContents, 'did-finish-load')
    })

    it('open a blank page when no URL is specified', async () => {
      const w = new BrowserWindow({ show: false })
      w.loadURL('about:blank')
      w.webContents.executeJavaScript(`b = window.open()`)
      const [, { webContents }] = await emittedOnce(app, 'browser-window-created')
      await emittedOnce(webContents, 'did-finish-load')
      expect(await w.webContents.executeJavaScript(`b.location.href`)).to.equal('about:blank')
    })

    it('open a blank page when an empty URL is specified', async () => {
      const w = new BrowserWindow({ show: false })
      w.loadURL('about:blank')
      w.webContents.executeJavaScript(`b = window.open('')`)
      const [, { webContents }] = await emittedOnce(app, 'browser-window-created')
      await emittedOnce(webContents, 'did-finish-load')
      expect(await w.webContents.executeJavaScript(`b.location.href`)).to.equal('about:blank')
    })

    it('sets the window title to the specified frameName', async () => {
      const w = new BrowserWindow({ show: false })
      w.loadURL('about:blank')
      w.webContents.executeJavaScript(`b = window.open('', 'hello')`)
      const [, window] = await emittedOnce(app, 'browser-window-created')
      expect(window.getTitle()).to.equal('hello')
    })

    it('does not throw an exception when the frameName is a built-in object property', async () => {
      const w = new BrowserWindow({ show: false })
      w.loadURL('about:blank')
      w.webContents.executeJavaScript(`b = window.open('', '__proto__')`)
      const [, window] = await emittedOnce(app, 'browser-window-created')
      expect(window.getTitle()).to.equal('__proto__')
    })
  })

  describe('window.opener', () => {
    it('is null for main window', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true
        }
      })
      w.loadFile(path.join(fixturesPath, 'pages', 'window-opener.html'))
      const [, channel, opener] = await emittedOnce(w.webContents, 'ipc-message')
      expect(channel).to.equal('opener')
      expect(opener).to.equal(null)
    })
  })
})

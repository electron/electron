import { expect } from 'chai'
import { BrowserWindow, WebContents, session, ipcMain, app, protocol, webContents } from 'electron'
import { emittedOnce } from './events-helpers'
import { closeAllWindows } from './window-helpers'
import * as https from 'https'
import * as http from 'http'
import * as path from 'path'
import * as fs from 'fs'
import * as url from 'url'
import * as ChildProcess from 'child_process'
import { EventEmitter } from 'events'
import { promisify } from 'util'
import { ifit, ifdescribe } from './spec-helpers'
import { AddressInfo } from 'net'

const features = process.electronBinding('features')

const fixturesPath = path.resolve(__dirname, '..', 'spec', 'fixtures')

describe('reporting api', () => {
  it('sends a report for a deprecation', async () => {
    const reports = new EventEmitter()

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
        req.on('data', (d) => { data += d.toString('utf-8') })
        req.on('end', () => {
          reports.emit('report', JSON.parse(data))
        })
      }
      res.setHeader('Report-To', JSON.stringify({
        group: 'default',
        max_age: 120,
        endpoints: [ { url: `https://localhost:${(server.address() as any).port}/report` } ]
      }))
      res.setHeader('Content-Type', 'text/html')
      // using the deprecated `webkitRequestAnimationFrame` will trigger a
      // "deprecation" report.
      res.end('<script>webkitRequestAnimationFrame(() => {})</script>')
    })
    await new Promise(resolve => server.listen(0, '127.0.0.1', resolve))
    const bw = new BrowserWindow({
      show: false
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
    const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } })
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
      w.loadFile(path.join(__dirname, 'fixtures', 'pages', 'jquery.html'))
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

  describe('form submit', () => {
    let server: http.Server
    let serverUrl: string

    before(async () => {
      server = http.createServer((req, res) => {
        let body = ''
        req.on('data', (chunk) => {
          body += chunk
        })
        res.setHeader('Content-Type', 'application/json')
        req.on('end', () => {
          res.end(`body:${body}`)
        })
      })
      await new Promise(resolve => server.listen(0, '127.0.0.1', resolve))
      serverUrl = `http://localhost:${(server.address() as any).port}`
    })
    after(async () => {
      server.close()
      await closeAllWindows()
    });

    [true, false].forEach((isSandboxEnabled) =>
      describe(`sandbox=${isSandboxEnabled}`, () => {
        it('posts data in the same window', () => {
          const w = new BrowserWindow({
            show: false,
            webPreferences: {
              sandbox: isSandboxEnabled
            }
          })

          return new Promise(async (resolve) => {
            await w.loadFile(path.join(fixturesPath, 'pages', 'form-with-data.html'))

            w.webContents.once('did-finish-load', async () => {
              const res = await w.webContents.executeJavaScript('document.body.innerText')
              expect(res).to.equal('body:greeting=hello')
              resolve()
            })

            w.webContents.executeJavaScript(`
              const form = document.querySelector('form')
              form.action = '${serverUrl}';
              form.submit();
            `)
          })
        })

        it('posts data to a new window with target=_blank', () => {
          const w = new BrowserWindow({
            show: false,
            webPreferences: {
              sandbox: isSandboxEnabled
            }
          })

          return new Promise(async (resolve) => {
            await w.loadFile(path.join(fixturesPath, 'pages', 'form-with-data.html'))

            app.once('browser-window-created', async (event, newWin) => {
              const res = await newWin.webContents.executeJavaScript('document.body.innerText')
              expect(res).to.equal('body:greeting=hello')
              resolve()
            })

            w.webContents.executeJavaScript(`
              const form = document.querySelector('form')
              form.action = '${serverUrl}';
              form.target = '_blank';
              form.submit();
            `)
          })
        })
      })
    )
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

  describe('navigator.mediaDevices', () => {
    afterEach(closeAllWindows)
    afterEach(() => {
      session.defaultSession.setPermissionCheckHandler(null)
    })

    it('can return labels of enumerated devices', async () => {
      const w = new BrowserWindow({ show: false })
      w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'))
      const labels = await w.webContents.executeJavaScript(`navigator.mediaDevices.enumerateDevices().then(ds => ds.map(d => d.label))`)
      expect(labels.some((l: any) => l)).to.be.true()
    })

    it('does not return labels of enumerated devices when permission denied', async () => {
      session.defaultSession.setPermissionCheckHandler(() => false)
      const w = new BrowserWindow({ show: false })
      w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'))
      const labels = await w.webContents.executeJavaScript(`navigator.mediaDevices.enumerateDevices().then(ds => ds.map(d => d.label))`)
      expect(labels.some((l: any) => l)).to.be.false()
    })

    it('can return new device id when cookie storage is cleared', async () => {
      const ses = session.fromPartition('persist:media-device-id')
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          session: ses
        }
      })
      w.loadFile(path.join(fixturesPath, 'pages', 'media-id-reset.html'))
      const [, firstDeviceIds] = await emittedOnce(ipcMain, 'deviceIds')
      await ses.clearStorageData({ storages: ['cookies'] })
      w.webContents.reload()
      const [, secondDeviceIds] = await emittedOnce(ipcMain, 'deviceIds')
      expect(firstDeviceIds).to.not.deep.equal(secondDeviceIds)
    })
  })

  describe('window.opener access', () => {
    const scheme = 'app'

    const fileUrl = `file://${fixturesPath}/pages/window-opener-location.html`
    const httpUrl1 = `${scheme}://origin1`
    const httpUrl2 = `${scheme}://origin2`
    const fileBlank = `file://${fixturesPath}/pages/blank.html`
    const httpBlank = `${scheme}://origin1/blank`

    const table = [
      { parent: fileBlank, child: httpUrl1, nodeIntegration: false, nativeWindowOpen: false, openerAccessible: false },
      { parent: fileBlank, child: httpUrl1, nodeIntegration: false, nativeWindowOpen: true, openerAccessible: false },
      { parent: fileBlank, child: httpUrl1, nodeIntegration: true, nativeWindowOpen: false, openerAccessible: true },
      { parent: fileBlank, child: httpUrl1, nodeIntegration: true, nativeWindowOpen: true, openerAccessible: false },

      { parent: httpBlank, child: fileUrl, nodeIntegration: false, nativeWindowOpen: false, openerAccessible: false },
      // {parent: httpBlank, child: fileUrl, nodeIntegration: false, nativeWindowOpen: true, openerAccessible: false}, // can't window.open()
      { parent: httpBlank, child: fileUrl, nodeIntegration: true, nativeWindowOpen: false, openerAccessible: true },
      // {parent: httpBlank, child: fileUrl, nodeIntegration: true, nativeWindowOpen: true, openerAccessible: false}, // can't window.open()

      // NB. this is different from Chrome's behavior, which isolates file: urls from each other
      { parent: fileBlank, child: fileUrl, nodeIntegration: false, nativeWindowOpen: false, openerAccessible: true },
      { parent: fileBlank, child: fileUrl, nodeIntegration: false, nativeWindowOpen: true, openerAccessible: true },
      { parent: fileBlank, child: fileUrl, nodeIntegration: true, nativeWindowOpen: false, openerAccessible: true },
      { parent: fileBlank, child: fileUrl, nodeIntegration: true, nativeWindowOpen: true, openerAccessible: true },

      { parent: httpBlank, child: httpUrl1, nodeIntegration: false, nativeWindowOpen: false, openerAccessible: true },
      { parent: httpBlank, child: httpUrl1, nodeIntegration: false, nativeWindowOpen: true, openerAccessible: true },
      { parent: httpBlank, child: httpUrl1, nodeIntegration: true, nativeWindowOpen: false, openerAccessible: true },
      { parent: httpBlank, child: httpUrl1, nodeIntegration: true, nativeWindowOpen: true, openerAccessible: true },

      { parent: httpBlank, child: httpUrl2, nodeIntegration: false, nativeWindowOpen: false, openerAccessible: false },
      { parent: httpBlank, child: httpUrl2, nodeIntegration: false, nativeWindowOpen: true, openerAccessible: false },
      { parent: httpBlank, child: httpUrl2, nodeIntegration: true, nativeWindowOpen: false, openerAccessible: true },
      { parent: httpBlank, child: httpUrl2, nodeIntegration: true, nativeWindowOpen: true, openerAccessible: false }
    ]
    const s = (url: string) => url.startsWith('file') ? 'file://...' : url

    before(async () => {
      await promisify(protocol.registerFileProtocol)(scheme, (request, callback) => {
        if (request.url.includes('blank')) {
          callback(`${fixturesPath}/pages/blank.html`)
        } else {
          callback(`${fixturesPath}/pages/window-opener-location.html`)
        }
      })
    })
    after(async () => {
      await promisify(protocol.unregisterProtocol)(scheme)
    })
    afterEach(closeAllWindows)

    describe('when opened from main window', () => {
      for (const { parent, child, nodeIntegration, nativeWindowOpen, openerAccessible } of table) {
        for (const sandboxPopup of [false, true]) {
          const description = `when parent=${s(parent)} opens child=${s(child)} with nodeIntegration=${nodeIntegration} nativeWindowOpen=${nativeWindowOpen} sandboxPopup=${sandboxPopup}, child should ${openerAccessible ? '' : 'not '}be able to access opener`
          it(description, async () => {
            const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, nativeWindowOpen } })
            w.webContents.once('new-window', (e, url, frameName, disposition, options) => {
              options!.webPreferences!.sandbox = sandboxPopup
            })
            await w.loadURL(parent)
            const childOpenerLocation = await w.webContents.executeJavaScript(`new Promise(resolve => {
              window.addEventListener('message', function f(e) {
                resolve(e.data)
              })
              window.open(${JSON.stringify(child)}, "", "show=no,nodeIntegration=${nodeIntegration ? 'yes' : 'no'}")
            })`)
            if (openerAccessible) {
              expect(childOpenerLocation).to.be.a('string')
            } else {
              expect(childOpenerLocation).to.be.null()
            }
          })
        }
      }
    })

    describe('when opened from <webview>', () => {
      for (const { parent, child, nodeIntegration, nativeWindowOpen, openerAccessible } of table) {
        const description = `when parent=${s(parent)} opens child=${s(child)} with nodeIntegration=${nodeIntegration} nativeWindowOpen=${nativeWindowOpen}, child should ${openerAccessible ? '' : 'not '}be able to access opener`
        // WebView erroneously allows access to the parent window when nativeWindowOpen is false.
        const skip = !nativeWindowOpen && !openerAccessible
        ifit(!skip)(description, async () => {
          // This test involves three contexts:
          //  1. The root BrowserWindow in which the test is run,
          //  2. A <webview> belonging to the root window,
          //  3. A window opened by calling window.open() from within the <webview>.
          // We are testing whether context (3) can access context (2) under various conditions.

          // This is context (1), the base window for the test.
          const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, webviewTag: true } })
          await w.loadURL('about:blank')

          const parentCode = `new Promise((resolve) => {
            // This is context (3), a child window of the WebView.
            const child = window.open(${JSON.stringify(child)}, "", "show=no")
            window.addEventListener("message", e => {
              resolve(e.data)
            })
          })`
          const childOpenerLocation = await w.webContents.executeJavaScript(`new Promise((resolve, reject) => {
            // This is context (2), a WebView which will call window.open()
            const webview = new WebView()
            webview.setAttribute('nodeintegration', '${nodeIntegration ? 'on' : 'off'}')
            webview.setAttribute('webpreferences', 'nativeWindowOpen=${nativeWindowOpen ? 'yes' : 'no'}')
            webview.setAttribute('allowpopups', 'on')
            webview.src = ${JSON.stringify(parent + '?p=' + encodeURIComponent(child))}
            webview.addEventListener('dom-ready', async () => {
              webview.executeJavaScript(${JSON.stringify(parentCode)}).then(resolve, reject)
            })
            document.body.appendChild(webview)
          })`)
          if (openerAccessible) {
            expect(childOpenerLocation).to.be.a('string')
          } else {
            expect(childOpenerLocation).to.be.null()
          }
        })
      }
    })
  })

  describe('storage', () => {
    describe('custom non standard schemes', () => {
      const protocolName = 'storage'
      let contents: WebContents
      before((done) => {
        protocol.registerFileProtocol(protocolName, (request, callback) => {
          const parsedUrl = url.parse(request.url)
          let filename
          switch (parsedUrl.pathname) {
            case '/localStorage' : filename = 'local_storage.html'; break
            case '/sessionStorage' : filename = 'session_storage.html'; break
            case '/WebSQL' : filename = 'web_sql.html'; break
            case '/indexedDB' : filename = 'indexed_db.html'; break
            case '/cookie' : filename = 'cookie.html'; break
            default : filename = ''
          }
          callback({ path: `${fixturesPath}/pages/storage/${filename}` })
        }, (error) => done(error))
      })

      after((done) => {
        protocol.unregisterProtocol(protocolName, () => done())
      })

      beforeEach(() => {
        contents = (webContents as any).create({
          nodeIntegration: true
        })
      })

      afterEach(() => {
        (contents as any).destroy()
        contents = null as any
      })

      it('cannot access localStorage', (done) => {
        ipcMain.once('local-storage-response', (event, error) => {
          expect(error).to.equal(`Failed to read the 'localStorage' property from 'Window': Access is denied for this document.`)
          done()
        })
        contents.loadURL(protocolName + '://host/localStorage')
      })

      it('cannot access sessionStorage', (done) => {
        ipcMain.once('session-storage-response', (event, error) => {
          expect(error).to.equal(`Failed to read the 'sessionStorage' property from 'Window': Access is denied for this document.`)
          done()
        })
        contents.loadURL(`${protocolName}://host/sessionStorage`)
      })

      it('cannot access WebSQL database', (done) => {
        ipcMain.once('web-sql-response', (event, error) => {
          expect(error).to.equal(`Failed to execute 'openDatabase' on 'Window': Access to the WebDatabase API is denied in this context.`)
          done()
        })
        contents.loadURL(`${protocolName}://host/WebSQL`)
      })

      it('cannot access indexedDB', (done) => {
        ipcMain.once('indexed-db-response', (event, error) => {
          expect(error).to.equal(`Failed to execute 'open' on 'IDBFactory': access to the Indexed Database API is denied in this context.`)
          done()
        })
        contents.loadURL(`${protocolName}://host/indexedDB`)
      })

      it('cannot access cookie', (done) => {
        ipcMain.once('cookie-response', (event, error) => {
          expect(error).to.equal(`Failed to set the 'cookie' property on 'Document': Access is denied for this document.`)
          done()
        })
        contents.loadURL(`${protocolName}://host/cookie`)
      })
    })

    describe('can be accessed', () => {
      let server: http.Server
      let serverUrl: string
      let serverCrossSiteUrl: string
      before((done) => {
        server = http.createServer((req, res) => {
          const respond = () => {
            if (req.url === '/redirect-cross-site') {
              res.setHeader('Location', `${serverCrossSiteUrl}/redirected`)
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
          serverUrl = `http://127.0.0.1:${(server.address() as AddressInfo).port}`
          serverCrossSiteUrl = `http://localhost:${(server.address() as AddressInfo).port}`
          done()
        })
      })

      after(() => {
        server.close()
        server = null as any
      })

      afterEach(closeAllWindows)

      const testLocalStorageAfterXSiteRedirect = (testTitle: string, extraPreferences = {}) => {
        it(testTitle, (done) => {
          const w = new BrowserWindow({
            show: false,
            ...extraPreferences
          })
          let redirected = false
          w.webContents.on('crashed', () => {
            expect.fail('renderer crashed / was killed')
          })
          w.webContents.on('did-redirect-navigation', (event, url) => {
            expect(url).to.equal(`${serverCrossSiteUrl}/redirected`)
            redirected = true
          })
          w.webContents.on('did-finish-load', () => {
            expect(redirected).to.be.true('didnt redirect')
            done()
          })
          w.loadURL(`${serverUrl}/redirect-cross-site`)
        })
      }

      testLocalStorageAfterXSiteRedirect('after a cross-site redirect')
      testLocalStorageAfterXSiteRedirect('after a cross-site redirect in sandbox mode', { sandbox: true })
    })
  })

  ifdescribe(features.isPDFViewerEnabled())('PDF Viewer', () => {
    const pdfSource = url.format({
      pathname: path.join(fixturesPath, 'assets', 'cat.pdf').replace(/\\/g, '/'),
      protocol: 'file',
      slashes: true
    })
    const pdfSourceWithParams = url.format({
      pathname: path.join(fixturesPath, 'assets', 'cat.pdf').replace(/\\/g, '/'),
      query: {
        a: 1,
        b: 2
      },
      protocol: 'file',
      slashes: true
    })

    const createBrowserWindow = ({ plugins, preload }: { plugins: boolean, preload: string }) => {
      return new BrowserWindow({
        show: false,
        webPreferences: {
          preload: path.join(fixturesPath, 'module', preload),
          plugins: plugins
        }
      })
    }

    const testPDFIsLoadedInSubFrame = (page: string, preloadFile: string, done: Function) => {
      const pagePath = url.format({
        pathname: path.join(fixturesPath, 'pages', page).replace(/\\/g, '/'),
        protocol: 'file',
        slashes: true
      })

      const w = createBrowserWindow({ plugins: true, preload: preloadFile })
      ipcMain.once('pdf-loaded', (event, state) => {
        expect(state).to.equal('success')
        done()
      })
      w.webContents.on('page-title-updated', () => {
        const parsedURL = url.parse(w.webContents.getURL(), true)
        expect(parsedURL.protocol).to.equal('chrome:')
        expect(parsedURL.hostname).to.equal('pdf-viewer')
        expect(parsedURL.query.src).to.equal(pagePath)
        expect(w.webContents.getTitle()).to.equal('cat.pdf')
      })
      w.loadFile(path.join(fixturesPath, 'pages', page))
    }

    it('opens when loading a pdf resource as top level navigation', (done) => {
      const w = createBrowserWindow({ plugins: true, preload: 'preload-pdf-loaded.js' })
      ipcMain.once('pdf-loaded', (event, state) => {
        expect(state).to.equal('success')
        done()
      })
      w.webContents.on('page-title-updated', () => {
        const parsedURL = url.parse(w.webContents.getURL(), true)
        expect(parsedURL.protocol).to.equal('chrome:')
        expect(parsedURL.hostname).to.equal('pdf-viewer')
        expect(parsedURL.query.src).to.equal(pdfSource)
        expect(w.webContents.getTitle()).to.equal('cat.pdf')
      })
      w.webContents.loadURL(pdfSource)
    })

    it('opens a pdf link given params, the query string should be escaped', (done) => {
      const w = createBrowserWindow({ plugins: true, preload: 'preload-pdf-loaded.js' })
      ipcMain.once('pdf-loaded', (event, state) => {
        expect(state).to.equal('success')
        done()
      })
      w.webContents.on('page-title-updated', () => {
        const parsedURL = url.parse(w.webContents.getURL(), true)
        expect(parsedURL.protocol).to.equal('chrome:')
        expect(parsedURL.hostname).to.equal('pdf-viewer')
        expect(parsedURL.query.src).to.equal(pdfSourceWithParams)
        expect(parsedURL.query.b).to.be.undefined()
        expect(parsedURL.search!.endsWith('%3Fa%3D1%26b%3D2')).to.be.true()
        expect(w.webContents.getTitle()).to.equal('cat.pdf')
      })
      w.webContents.loadURL(pdfSourceWithParams)
    })

    it('should download a pdf when plugins are disabled', async () => {
      const w = createBrowserWindow({ plugins: false, preload: 'preload-pdf-loaded.js' })
      w.webContents.loadURL(pdfSource)
      const [state, filename, mimeType] = await new Promise(resolve => {
        session.defaultSession.once('will-download', (event, item) => {
          item.setSavePath(path.join(fixturesPath, 'mock.pdf'))
          item.on('done', (e, state) => {
            resolve([state, item.getFilename(), item.getMimeType()])
          })
        })
      })
      expect(state).to.equal('completed')
      expect(filename).to.equal('cat.pdf')
      expect(mimeType).to.equal('application/pdf')
      fs.unlinkSync(path.join(fixturesPath, 'mock.pdf'))
    })

    it('should not open when pdf is requested as sub resource', async () => {
      const w = new BrowserWindow({ show: false })
      w.loadURL('about:blank')
      const [status, title] = await w.webContents.executeJavaScript(`fetch(${JSON.stringify(pdfSource)}).then(res => [res.status, document.title])`)
      expect(status).to.equal(200)
      expect(title).to.not.equal('cat.pdf')
    })

    it('opens when loading a pdf resource in a iframe', (done) => {
      testPDFIsLoadedInSubFrame('pdf-in-iframe.html', 'preload-pdf-loaded-in-subframe.js', done)
    })

    it('opens when loading a pdf resource in a nested iframe', (done) => {
      testPDFIsLoadedInSubFrame('pdf-in-nested-iframe.html', 'preload-pdf-loaded-in-nested-subframe.js', done)
    })
  })

  describe('window.history', () => {
    describe('window.history.pushState', () => {
      it('should push state after calling history.pushState() from the same url', async () => {
        const w = new BrowserWindow({ show: false })
        await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'))
        // History should have current page by now.
        expect((w.webContents as any).length()).to.equal(1)

        const waitCommit = emittedOnce(w.webContents, 'navigation-entry-commited')
        w.webContents.executeJavaScript('window.history.pushState({}, "")')
        await waitCommit
        // Initial page + pushed state.
        expect((w.webContents as any).length()).to.equal(2)
      })
    })
  })
})

describe('font fallback', () => {
  async function getRenderedFonts (html: string) {
    const w = new BrowserWindow({ show: false })
    try {
      await w.loadURL(`data:text/html,${html}`)
      w.webContents.debugger.attach()
      const sendCommand = (method: string, commandParams?: any) => w.webContents.debugger.sendCommand(method, commandParams)
      const { nodeId } = (await sendCommand('DOM.getDocument')).root.children[0]
      await sendCommand('CSS.enable')
      const { fonts } = await sendCommand('CSS.getPlatformFontsForNode', { nodeId })
      return fonts
    } finally {
      w.close()
    }
  }

  it('should use Helvetica for sans-serif on Mac, and Arial on Windows and Linux', async () => {
    const html = `<body style="font-family: sans-serif">test</body>`
    const fonts = await getRenderedFonts(html)
    expect(fonts).to.be.an('array')
    expect(fonts).to.have.length(1)
    if (process.platform === 'win32') { expect(fonts[0].familyName).to.equal('Arial') } else if (process.platform === 'darwin') { expect(fonts[0].familyName).to.equal('Helvetica') } else if (process.platform === 'linux') { expect(fonts[0].familyName).to.equal('DejaVu Sans') } // I think this depends on the distro? We don't specify a default.
  })

  ifit(process.platform !== 'linux')('should fall back to Japanese font for sans-serif Japanese script', async function () {
    const html = `
    <html lang="ja-JP">
      <head>
        <meta charset="utf-8" />
      </head>
      <body style="font-family: sans-serif">test 智史</body>
    </html>
    `
    const fonts = await getRenderedFonts(html)
    expect(fonts).to.be.an('array')
    expect(fonts).to.have.length(1)
    if (process.platform === 'win32') { expect(fonts[0].familyName).to.be.oneOf(['Meiryo', 'Yu Gothic']) } else if (process.platform === 'darwin') { expect(fonts[0].familyName).to.equal('Hiragino Kaku Gothic ProN') }
  })
})

describe('iframe using HTML fullscreen API while window is OS-fullscreened', () => {
  const fullscreenChildHtml = promisify(fs.readFile)(
    path.join(fixturesPath, 'pages', 'fullscreen-oopif.html')
  )
  let w: BrowserWindow, server: http.Server

  before(() => {
    server = http.createServer(async (_req, res) => {
      res.writeHead(200, { 'Content-Type': 'text/html' })
      res.write(await fullscreenChildHtml)
      res.end()
    })

    server.listen(8989, '127.0.0.1')
  })

  beforeEach(() => {
    w = new BrowserWindow({
      show: true,
      fullscreen: true,
      webPreferences: {
        nodeIntegration: true,
        nodeIntegrationInSubFrames: true
      }
    })
  })

  afterEach(async () => {
    await closeAllWindows()
    ;(w as any) = null
    server.close()
  })

  it('can fullscreen from out-of-process iframes (OOPIFs)', done => {
    ipcMain.once('fullscreenChange', async () => {
      const fullscreenWidth = await w.webContents.executeJavaScript(
        "document.querySelector('iframe').offsetWidth"
      )
      expect(fullscreenWidth > 0).to.be.true()

      await w.webContents.executeJavaScript(
        "document.querySelector('iframe').contentWindow.postMessage('exitFullscreen', '*')"
      )

      await new Promise(resolve => setTimeout(resolve, 500))

      const width = await w.webContents.executeJavaScript(
        "document.querySelector('iframe').offsetWidth"
      )
      expect(width).to.equal(0)

      done()
    })

    const html =
      '<iframe style="width: 0" frameborder=0 src="http://localhost:8989" allowfullscreen></iframe>'
    w.loadURL(`data:text/html,${html}`)
  })

  it('can fullscreen from in-process iframes', done => {
    ipcMain.once('fullscreenChange', async () => {
      const fullscreenWidth = await w.webContents.executeJavaScript(
        "document.querySelector('iframe').offsetWidth"
      )
      expect(fullscreenWidth > 0).to.true()

      await w.webContents.executeJavaScript('document.exitFullscreen()')
      const width = await w.webContents.executeJavaScript(
        "document.querySelector('iframe').offsetWidth"
      )
      expect(width).to.equal(0)
      done()
    })

    w.loadFile(path.join(fixturesPath, 'pages', 'fullscreen-ipif.html'))
  })
})

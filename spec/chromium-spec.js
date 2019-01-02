const assert = require('assert')
const chai = require('chai')
const dirtyChai = require('dirty-chai')
const fs = require('fs')
const http = require('http')
const path = require('path')
const ws = require('ws')
const url = require('url')
const ChildProcess = require('child_process')
const { ipcRenderer, remote } = require('electron')
const { emittedOnce } = require('./events-helpers')
const { closeWindow } = require('./window-helpers')
const { resolveGetters } = require('./assert-helpers')
const { app, BrowserWindow, ipcMain, protocol, session, webContents } = remote
const isCI = remote.getGlobal('isCi')
const features = process.atomBinding('features')

const { expect } = chai
chai.use(dirtyChai)

/* Most of the APIs here don't use standard callbacks */
/* eslint-disable standard/no-callback-literal */

describe('chromium feature', () => {
  const fixtures = path.resolve(__dirname, 'fixtures')
  let listener = null
  let w = null

  afterEach(() => {
    if (listener != null) {
      window.removeEventListener('message', listener)
    }
    listener = null
  })

  describe('command line switches', () => {
    describe('--lang switch', () => {
      const currentLocale = app.getLocale()
      const testLocale = (locale, result, done) => {
        const appPath = path.join(__dirname, 'fixtures', 'api', 'locale-check')
        const electronPath = remote.getGlobal('process').execPath
        let output = ''
        const appProcess = ChildProcess.spawn(electronPath, [appPath, `--lang=${locale}`])

        appProcess.stdout.on('data', (data) => { output += data })
        appProcess.stdout.on('end', () => {
          output = output.replace(/(\r\n|\n|\r)/gm, '')
          assert.strictEqual(output, result)
          done()
        })
      }

      it('should set the locale', (done) => testLocale('fr', 'fr', done))
      it('should not set an invalid locale', (done) => testLocale('asdfkl', currentLocale, done))
    })

    describe('--remote-debugging-port switch', () => {
      it('should display the discovery page', (done) => {
        const electronPath = remote.getGlobal('process').execPath
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
              expect(parseInt(res.headers['content-length'])).to.be.greaterThan(0)
              done()
            })
          }
        })
      })
    })
  })

  afterEach(() => closeWindow(w).then(() => { w = null }))

  describe('heap snapshot', () => {
    it('does not crash', function () {
      process.atomBinding('v8_util').takeHeapSnapshot()
    })
  })

  describe('sending request of http protocol urls', () => {
    it('does not crash', (done) => {
      const server = http.createServer((req, res) => {
        res.end()
        server.close()
        done()
      })
      server.listen(0, '127.0.0.1', () => {
        const port = server.address().port
        $.get(`http://127.0.0.1:${port}`)
      })
    })
  })

  describe('accessing key names also used as Node.js module names', () => {
    it('does not crash', (done) => {
      w = new BrowserWindow({ show: false })
      w.webContents.once('did-finish-load', () => { done() })
      w.webContents.once('crashed', () => done(new Error('WebContents crashed.')))
      w.loadFile(path.join(fixtures, 'pages', 'external-string.html'))
    })
  })

  describe('loading jquery', () => {
    it('does not crash', (done) => {
      w = new BrowserWindow({ show: false })
      w.webContents.once('did-finish-load', () => { done() })
      w.webContents.once('crashed', () => done(new Error('WebContents crashed.')))
      w.loadFile(path.join(fixtures, 'pages', 'jquery.html'))
    })
  })

  describe('navigator.webkitGetUserMedia', () => {
    it('calls its callbacks', (done) => {
      navigator.webkitGetUserMedia({
        audio: true,
        video: false
      }, () => done(),
      () => done())
    })
  })

  describe('navigator.mediaDevices', () => {
    if (isCI) return

    afterEach(() => {
      remote.getGlobal('permissionChecks').allow()
    })

    it('can return labels of enumerated devices', (done) => {
      navigator.mediaDevices.enumerateDevices().then((devices) => {
        const labels = devices.map((device) => device.label)
        const labelFound = labels.some((label) => !!label)
        if (labelFound) {
          done()
        } else {
          done(new Error(`No device labels found: ${JSON.stringify(labels)}`))
        }
      }).catch(done)
    })

    it('does not return labels of enumerated devices when permission denied', (done) => {
      remote.getGlobal('permissionChecks').reject()
      navigator.mediaDevices.enumerateDevices().then((devices) => {
        const labels = devices.map((device) => device.label)
        const labelFound = labels.some((label) => !!label)
        if (labelFound) {
          done(new Error(`Device labels were found: ${JSON.stringify(labels)}`))
        } else {
          done()
        }
      }).catch(done)
    })

    it('can return new device id when cookie storage is cleared', (done) => {
      const options = {
        origin: null,
        storages: ['cookies']
      }
      const deviceIds = []
      const ses = session.fromPartition('persist:media-device-id')
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          session: ses
        }
      })
      w.webContents.on('ipc-message', (event, args) => {
        if (args[0] === 'deviceIds') deviceIds.push(args[1])
        if (deviceIds.length === 2) {
          assert.notDeepStrictEqual(deviceIds[0], deviceIds[1])
          closeWindow(w).then(() => {
            w = null
            done()
          }).catch((error) => done(error))
        } else {
          ses.clearStorageData(options, () => {
            w.webContents.reload()
          })
        }
      })
      w.loadFile(path.join(fixtures, 'pages', 'media-id-reset.html'))
    })
  })

  describe('navigator.language', () => {
    it('should not be empty', () => {
      assert.notStrictEqual(navigator.language, '')
    })
  })

  describe('navigator.languages', (done) => {
    it('should return the system locale only', () => {
      const appLocale = app.getLocale()
      assert.strictEqual(navigator.languages.length, 1)
      assert.strictEqual(navigator.languages[0], appLocale)
    })
  })

  describe('navigator.serviceWorker', () => {
    it('should register for file scheme', (done) => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          partition: 'sw-file-scheme-spec'
        }
      })
      w.webContents.on('ipc-message', (event, args) => {
        if (args[0] === 'reload') {
          w.webContents.reload()
        } else if (args[0] === 'error') {
          done(args[1])
        } else if (args[0] === 'response') {
          assert.strictEqual(args[1], 'Hello from serviceWorker!')
          session.fromPartition('sw-file-scheme-spec').clearStorageData({
            storages: ['serviceworkers']
          }, () => done())
        }
      })
      w.webContents.on('crashed', () => done(new Error('WebContents crashed.')))
      w.loadFile(path.join(fixtures, 'pages', 'service-worker', 'index.html'))
    })

    it('should register for intercepted file scheme', (done) => {
      const customSession = session.fromPartition('intercept-file')
      customSession.protocol.interceptBufferProtocol('file', (request, callback) => {
        let file = url.parse(request.url).pathname
        if (file[0] === '/' && process.platform === 'win32') file = file.slice(1)

        const content = fs.readFileSync(path.normalize(file))
        const ext = path.extname(file)
        let type = 'text/html'

        if (ext === '.js') type = 'application/javascript'
        callback({ data: content, mimeType: type })
      }, (error) => {
        if (error) done(error)
      })

      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          session: customSession
        }
      })
      w.webContents.on('ipc-message', (event, args) => {
        if (args[0] === 'reload') {
          w.webContents.reload()
        } else if (args[0] === 'error') {
          done(`unexpected error : ${args[1]}`)
        } else if (args[0] === 'response') {
          assert.strictEqual(args[1], 'Hello from serviceWorker!')
          customSession.clearStorageData({
            storages: ['serviceworkers']
          }, () => {
            customSession.protocol.uninterceptProtocol('file', (error) => done(error))
          })
        }
      })
      w.webContents.on('crashed', () => done(new Error('WebContents crashed.')))
      w.loadFile(path.join(fixtures, 'pages', 'service-worker', 'index.html'))
    })
  })

  describe('navigator.geolocation', () => {
    before(function () {
      if (!features.isFakeLocationProviderEnabled()) {
        return this.skip()
      }
    })

    it('returns position when permission is granted', (done) => {
      navigator.geolocation.getCurrentPosition((position) => {
        assert(position.timestamp)
        done()
      }, (error) => {
        done(error)
      })
    })

    it('returns error when permission is denied', (done) => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          partition: 'geolocation-spec'
        }
      })
      w.webContents.on('ipc-message', (event, args) => {
        if (args[0] === 'success') {
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
      w.loadFile(path.join(fixtures, 'pages', 'geolocation', 'index.html'))
    })
  })

  describe('window.open', () => {
    it('returns a BrowserWindowProxy object', () => {
      const b = window.open('about:blank', '', 'show=no')
      assert.strictEqual(b.closed, false)
      assert.strictEqual(b.constructor.name, 'BrowserWindowProxy')
      b.close()
    })

    it('accepts "nodeIntegration" as feature', (done) => {
      let b = null
      listener = (event) => {
        assert.strictEqual(event.data.isProcessGlobalUndefined, true)
        b.close()
        done()
      }
      window.addEventListener('message', listener)
      b = window.open(`file://${fixtures}/pages/window-opener-node.html`, '', 'nodeIntegration=no,show=no')
    })

    it('inherit options of parent window', (done) => {
      let b = null
      listener = (event) => {
        const ref1 = remote.getCurrentWindow().getSize()
        const width = ref1[0]
        const height = ref1[1]
        assert.strictEqual(event.data, `size: ${width} ${height}`)
        b.close()
        done()
      }
      window.addEventListener('message', listener)
      b = window.open(`file://${fixtures}/pages/window-open-size.html`, '', 'show=no')
    })

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
          assert.strictEqual(options.show, w.isVisible())
          w.close()
          done()
        })
        w.loadFile(path.join(fixtures, 'pages', 'window-open.html'))
      })
    }

    it('disables node integration when it is disabled on the parent window', (done) => {
      let b = null
      listener = (event) => {
        assert.strictEqual(event.data.isProcessGlobalUndefined, true)
        b.close()
        done()
      }
      window.addEventListener('message', listener)

      const windowUrl = require('url').format({
        pathname: `${fixtures}/pages/window-opener-no-node-integration.html`,
        protocol: 'file',
        query: {
          p: `${fixtures}/pages/window-opener-node.html`
        },
        slashes: true
      })
      b = window.open(windowUrl, '', 'nodeIntegration=no,show=no')
    })

    // TODO(codebytere): re-enable this test
    xit('disables node integration when it is disabled on the parent window for chrome devtools URLs', (done) => {
      let b = null
      app.once('web-contents-created', (event, contents) => {
        contents.once('did-finish-load', () => {
          contents.executeJavaScript('typeof process').then((typeofProcessGlobal) => {
            assert.strictEqual(typeofProcessGlobal, 'undefined')
            b.close()
            done()
          }).catch(done)
        })
      })
      b = window.open('chrome-devtools://devtools/bundled/inspector.html', '', 'nodeIntegration=no,show=no')
    })

    it('disables JavaScript when it is disabled on the parent window', (done) => {
      let b = null
      app.once('web-contents-created', (event, contents) => {
        contents.once('did-finish-load', () => {
          app.once('browser-window-created', (event, window) => {
            const preferences = window.webContents.getLastWebPreferences()
            assert.strictEqual(preferences.javascript, false)
            window.destroy()
            b.close()
            done()
          })
          // Click link on page
          contents.sendInputEvent({ type: 'mouseDown', clickCount: 1, x: 1, y: 1 })
          contents.sendInputEvent({ type: 'mouseUp', clickCount: 1, x: 1, y: 1 })
        })
      })

      const windowUrl = require('url').format({
        pathname: `${fixtures}/pages/window-no-javascript.html`,
        protocol: 'file',
        slashes: true
      })
      b = window.open(windowUrl, '', 'javascript=no,show=no')
    })

    it('disables the <webview> tag when it is disabled on the parent window', (done) => {
      let b = null
      listener = (event) => {
        assert.strictEqual(event.data.isWebViewGlobalUndefined, true)
        b.close()
        done()
      }
      window.addEventListener('message', listener)

      const windowUrl = require('url').format({
        pathname: `${fixtures}/pages/window-opener-no-webview-tag.html`,
        protocol: 'file',
        query: {
          p: `${fixtures}/pages/window-opener-webview.html`
        },
        slashes: true
      })
      b = window.open(windowUrl, '', 'webviewTag=no,nodeIntegration=yes,show=no')
    })

    it('does not override child options', (done) => {
      let b = null
      const size = {
        width: 350,
        height: 450
      }
      listener = (event) => {
        assert.strictEqual(event.data, `size: ${size.width} ${size.height}`)
        b.close()
        done()
      }
      window.addEventListener('message', listener)
      b = window.open(`file://${fixtures}/pages/window-open-size.html`, '', 'show=no,width=' + size.width + ',height=' + size.height)
    })

    it('handles cycles when merging the parent options into the child options', (done) => {
      w = BrowserWindow.fromId(ipcRenderer.sendSync('create-window-with-options-cycle'))
      w.loadFile(path.join(fixtures, 'pages', 'window-open.html'))
      w.webContents.once('new-window', (event, url, frameName, disposition, options) => {
        assert.strictEqual(options.show, false)
        assert.deepStrictEqual(...resolveGetters(options.foo, {
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
        }))
        done()
      })
    })

    it('defines a window.location getter', (done) => {
      let b = null
      let targetURL
      if (process.platform === 'win32') {
        targetURL = `file:///${fixtures.replace(/\\/g, '/')}/pages/base-page.html`
      } else {
        targetURL = `file://${fixtures}/pages/base-page.html`
      }
      app.once('browser-window-created', (event, window) => {
        window.webContents.once('did-finish-load', () => {
          assert.strictEqual(b.location.href, targetURL)
          b.close()
          done()
        })
      })
      b = window.open(targetURL)
    })

    it('defines a window.location setter', (done) => {
      let b = null
      app.once('browser-window-created', (event, { webContents }) => {
        webContents.once('did-finish-load', () => {
          // When it loads, redirect
          b.location = `file://${fixtures}/pages/base-page.html`
          webContents.once('did-finish-load', () => {
            // After our second redirect, cleanup and callback
            b.close()
            done()
          })
        })
      })
      b = window.open('about:blank')
    })

    it('open a blank page when no URL is specified', (done) => {
      let b = null
      app.once('browser-window-created', (event, { webContents }) => {
        webContents.once('did-finish-load', () => {
          const { location } = b
          b.close()
          assert.strictEqual(location.href, 'about:blank')

          let c = null
          app.once('browser-window-created', (event, { webContents }) => {
            webContents.once('did-finish-load', () => {
              const { location } = c
              c.close()
              assert.strictEqual(location.href, 'about:blank')
              done()
            })
          })
          c = window.open('')
        })
      })
      b = window.open()
    })

    it('throws an exception when the arguments cannot be converted to strings', () => {
      assert.throws(() => {
        window.open('', { toString: null })
      }, /Cannot convert object to primitive value/)

      assert.throws(() => {
        window.open('', '', { toString: 3 })
      }, /Cannot convert object to primitive value/)
    })

    it('sets the window title to the specified frameName', (done) => {
      let b = null
      app.once('browser-window-created', (event, createdWindow) => {
        assert.strictEqual(createdWindow.getTitle(), 'hello')
        b.close()
        done()
      })
      b = window.open('', 'hello')
    })

    it('does not throw an exception when the frameName is a built-in object property', (done) => {
      let b = null
      app.once('browser-window-created', (event, createdWindow) => {
        assert.strictEqual(createdWindow.getTitle(), '__proto__')
        b.close()
        done()
      })
      b = window.open('', '__proto__')
    })

    it('does not throw an exception when the features include webPreferences', () => {
      let b = null
      assert.doesNotThrow(() => {
        b = window.open('', '', 'webPreferences=')
      })
      b.close()
    })
  })

  describe('window.opener', () => {
    const url = `file://${fixtures}/pages/window-opener.html`
    it('is null for main window', (done) => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true
        }
      })
      w.webContents.once('ipc-message', (event, args) => {
        assert.deepStrictEqual(args, ['opener', null])
        done()
      })
      w.loadFile(path.join(fixtures, 'pages', 'window-opener.html'))
    })

    it('is not null for window opened by window.open', (done) => {
      let b = null
      listener = (event) => {
        assert.strictEqual(event.data, 'object')
        b.close()
        done()
      }
      window.addEventListener('message', listener)
      b = window.open(url, '', 'show=no')
    })
  })

  describe('window.opener access from BrowserWindow', () => {
    const scheme = 'other'
    const url = `${scheme}://${fixtures}/pages/window-opener-location.html`
    let w = null

    before((done) => {
      protocol.registerFileProtocol(scheme, (request, callback) => {
        callback(`${fixtures}/pages/window-opener-location.html`)
      }, (error) => done(error))
    })

    after(() => {
      protocol.unregisterProtocol(scheme)
    })

    afterEach(() => {
      w.close()
    })

    it('does nothing when origin of current window does not match opener', (done) => {
      listener = (event) => {
        assert.strictEqual(event.data, '')
        done()
      }
      window.addEventListener('message', listener)
      w = window.open(url, '', 'show=no,nodeIntegration=no')
    })

    it('works when origin matches', (done) => {
      listener = (event) => {
        assert.strictEqual(event.data, location.href)
        done()
      }
      window.addEventListener('message', listener)
      w = window.open(`file://${fixtures}/pages/window-opener-location.html`, '', 'show=no,nodeIntegration=no')
    })

    it('works when origin does not match opener but has node integration', (done) => {
      listener = (event) => {
        assert.strictEqual(event.data, location.href)
        done()
      }
      window.addEventListener('message', listener)
      w = window.open(url, '', 'show=no,nodeIntegration=yes')
    })
  })

  describe('window.opener access from <webview>', () => {
    const scheme = 'other'
    const srcPath = `${fixtures}/pages/webview-opener-postMessage.html`
    const pageURL = `file://${fixtures}/pages/window-opener-location.html`
    let webview = null

    before((done) => {
      protocol.registerFileProtocol(scheme, (request, callback) => {
        callback(srcPath)
      }, (error) => done(error))
    })

    after(() => {
      protocol.unregisterProtocol(scheme)
    })

    afterEach(() => {
      if (webview != null) webview.remove()
    })

    it('does nothing when origin of webview src URL does not match opener', (done) => {
      webview = new WebView()
      webview.addEventListener('console-message', (e) => {
        assert.strictEqual(e.message, '')
        done()
      })
      webview.setAttribute('allowpopups', 'on')
      webview.src = url.format({
        pathname: srcPath,
        protocol: scheme,
        query: {
          p: pageURL
        },
        slashes: true
      })
      document.body.appendChild(webview)
    })

    it('works when origin matches', (done) => {
      webview = new WebView()
      webview.addEventListener('console-message', (e) => {
        assert.strictEqual(e.message, webview.src)
        done()
      })
      webview.setAttribute('allowpopups', 'on')
      webview.src = url.format({
        pathname: srcPath,
        protocol: 'file',
        query: {
          p: pageURL
        },
        slashes: true
      })
      document.body.appendChild(webview)
    })

    it('works when origin does not match opener but has node integration', (done) => {
      webview = new WebView()
      webview.addEventListener('console-message', (e) => {
        webview.remove()
        assert.strictEqual(e.message, webview.src)
        done()
      })
      webview.setAttribute('allowpopups', 'on')
      webview.setAttribute('nodeintegration', 'on')
      webview.src = url.format({
        pathname: srcPath,
        protocol: scheme,
        query: {
          p: pageURL
        },
        slashes: true
      })
      document.body.appendChild(webview)
    })
  })

  describe('window.postMessage', () => {
    it('sets the source and origin correctly', (done) => {
      let b = null
      listener = (event) => {
        window.removeEventListener('message', listener)
        b.close()
        const message = JSON.parse(event.data)
        assert.strictEqual(message.data, 'testing')
        assert.strictEqual(message.origin, 'file://')
        assert.strictEqual(message.sourceEqualsOpener, true)
        assert.strictEqual(event.origin, 'file://')
        done()
      }
      window.addEventListener('message', listener)
      app.once('browser-window-created', (event, { webContents }) => {
        webContents.once('did-finish-load', () => {
          b.postMessage('testing', '*')
        })
      })
      b = window.open(`file://${fixtures}/pages/window-open-postMessage.html`, '', 'show=no')
    })

    it('throws an exception when the targetOrigin cannot be converted to a string', () => {
      const b = window.open('')
      assert.throws(() => {
        b.postMessage('test', { toString: null })
      }, /Cannot convert object to primitive value/)
      b.close()
    })
  })

  describe('window.opener.postMessage', () => {
    it('sets source and origin correctly', (done) => {
      let b = null
      listener = (event) => {
        window.removeEventListener('message', listener)
        b.close()
        assert.strictEqual(event.source, b)
        assert.strictEqual(event.origin, 'file://')
        done()
      }
      window.addEventListener('message', listener)
      b = window.open(`file://${fixtures}/pages/window-opener-postMessage.html`, '', 'show=no')
    })

    it('supports windows opened from a <webview>', (done) => {
      const webview = new WebView()
      webview.addEventListener('console-message', (e) => {
        webview.remove()
        assert.strictEqual(e.message, 'message')
        done()
      })
      webview.allowpopups = true
      webview.src = url.format({
        pathname: `${fixtures}/pages/webview-opener-postMessage.html`,
        protocol: 'file',
        query: {
          p: `${fixtures}/pages/window-opener-postMessage.html`
        },
        slashes: true
      })
      document.body.appendChild(webview)
    })

    describe('targetOrigin argument', () => {
      let serverURL
      let server

      beforeEach((done) => {
        server = http.createServer((req, res) => {
          res.writeHead(200)
          const filePath = path.join(fixtures, 'pages', 'window-opener-targetOrigin.html')
          res.end(fs.readFileSync(filePath, 'utf8'))
        })
        server.listen(0, '127.0.0.1', () => {
          serverURL = `http://127.0.0.1:${server.address().port}`
          done()
        })
      })

      afterEach(() => {
        server.close()
      })

      it('delivers messages that match the origin', (done) => {
        let b = null
        listener = (event) => {
          window.removeEventListener('message', listener)
          b.close()
          assert.strictEqual(event.data, 'deliver')
          done()
        }
        window.addEventListener('message', listener)
        b = window.open(serverURL, '', 'show=no')
      })
    })
  })

  describe('creating a Uint8Array under browser side', () => {
    it('does not crash', () => {
      const RUint8Array = remote.getGlobal('Uint8Array')
      const arr = new RUint8Array()
      assert(arr)
    })
  })

  describe('webgl', () => {
    before(function () {
      if (isCI && process.platform === 'win32') {
        this.skip()
      }
    })

    it('can be get as context in canvas', () => {
      if (process.platform === 'linux') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return
      }

      const webgl = document.createElement('canvas').getContext('webgl')
      assert.notStrictEqual(webgl, null)
    })
  })

  describe('web workers', () => {
    it('Worker can work', (done) => {
      const worker = new Worker('../fixtures/workers/worker.js')
      const message = 'ping'
      worker.onmessage = (event) => {
        assert.strictEqual(event.data, message)
        worker.terminate()
        done()
      }
      worker.postMessage(message)
    })

    it('Worker has no node integration by default', (done) => {
      const worker = new Worker('../fixtures/workers/worker_node.js')
      worker.onmessage = (event) => {
        assert.strictEqual(event.data, 'undefined undefined undefined undefined')
        worker.terminate()
        done()
      }
    })

    it('Worker has node integration with nodeIntegrationInWorker', (done) => {
      const webview = new WebView()
      webview.addEventListener('ipc-message', (e) => {
        assert.strictEqual(e.channel, 'object function object function')
        webview.remove()
        done()
      })
      webview.src = `file://${fixtures}/pages/worker.html`
      webview.setAttribute('webpreferences', 'nodeIntegration, nodeIntegrationInWorker')
      document.body.appendChild(webview)
    })

    it('SharedWorker can work', (done) => {
      const worker = new SharedWorker('../fixtures/workers/shared_worker.js')
      const message = 'ping'
      worker.port.onmessage = (event) => {
        assert.strictEqual(event.data, message)
        done()
      }
      worker.port.postMessage(message)
    })

    it('SharedWorker has no node integration by default', (done) => {
      const worker = new SharedWorker('../fixtures/workers/shared_worker_node.js')
      worker.port.onmessage = (event) => {
        assert.strictEqual(event.data, 'undefined undefined undefined undefined')
        done()
      }
    })

    it('SharedWorker has node integration with nodeIntegrationInWorker', (done) => {
      const webview = new WebView()
      webview.addEventListener('console-message', (e) => {
        console.log(e)
      })
      webview.addEventListener('ipc-message', (e) => {
        assert.strictEqual(e.channel, 'object function object function')
        webview.remove()
        done()
      })
      webview.src = `file://${fixtures}/pages/shared_worker.html`
      webview.setAttribute('webpreferences', 'nodeIntegration, nodeIntegrationInWorker')
      document.body.appendChild(webview)
    })
  })

  describe('iframe', () => {
    let iframe = null

    beforeEach(() => {
      iframe = document.createElement('iframe')
    })

    afterEach(() => {
      document.body.removeChild(iframe)
    })

    it('does not have node integration', (done) => {
      iframe.src = `file://${fixtures}/pages/set-global.html`
      document.body.appendChild(iframe)
      iframe.onload = () => {
        assert.strictEqual(iframe.contentWindow.test, 'undefined undefined undefined')
        done()
      }
    })
  })

  describe('storage', () => {
    describe('DOM storage quota override', () => {
      ['localStorage', 'sessionStorage'].forEach((storageName) => {
        it(`allows saving at least 50MiB in ${storageName}`, () => {
          const storage = window[storageName]
          const testKeyName = '_electronDOMStorageQuotaOverrideTest'
          // 25 * 2^20 UTF-16 characters will require 50MiB
          const arraySize = 25 * Math.pow(2, 20)
          storage[testKeyName] = new Array(arraySize).fill('X').join('')
          expect(storage[testKeyName]).to.have.lengthOf(arraySize)
          delete storage[testKeyName]
        })
      })
    })

    it('requesting persitent quota works', (done) => {
      navigator.webkitPersistentStorage.requestQuota(1024 * 1024, (grantedBytes) => {
        assert.strictEqual(grantedBytes, 1048576)
        done()
      })
    })

    describe('custom non standard schemes', () => {
      const protocolName = 'storage'
      let contents = null
      before((done) => {
        const handler = (request, callback) => {
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
          callback({ path: `${fixtures}/pages/storage/${filename}` })
        }
        protocol.registerFileProtocol(protocolName, handler, (error) => done(error))
      })

      after((done) => {
        protocol.unregisterProtocol(protocolName, () => done())
      })

      beforeEach(() => {
        contents = webContents.create({
          nodeIntegration: true
        })
      })

      afterEach(() => {
        contents.destroy()
        contents = null
      })

      it('cannot access localStorage', (done) => {
        contents.on('crashed', (event, killed) => {
          // Site isolation ON: process is killed for trying to access resources without permission.
          if (process.platform !== 'win32') {
            // Chromium on Windows does not set this flag correctly.
            assert.strictEqual(killed, true, 'Process should\'ve been killed')
          }
          done()
        })
        ipcMain.once('local-storage-response', (event, message) => {
          // Site isolation OFF: access is refused.
          assert.strictEqual(
            message,
            'Failed to read the \'localStorage\' property from \'Window\': Access is denied for this document.')
        })
        contents.loadURL(protocolName + '://host/localStorage')
      })

      it('cannot access sessionStorage', (done) => {
        ipcMain.once('session-storage-response', (event, error) => {
          assert.strictEqual(
            error,
            'Failed to read the \'sessionStorage\' property from \'Window\': Access is denied for this document.')
          done()
        })
        contents.loadURL(`${protocolName}://host/sessionStorage`)
      })

      it('cannot access WebSQL database', (done) => {
        ipcMain.once('web-sql-response', (event, error) => {
          assert.strictEqual(
            error,
            'An attempt was made to break through the security policy of the user agent.')
          done()
        })
        contents.loadURL(`${protocolName}://host/WebSQL`)
      })

      it('cannot access indexedDB', (done) => {
        ipcMain.once('indexed-db-response', (event, error) => {
          assert.strictEqual(error, 'The user denied permission to access the database.')
          done()
        })
        contents.loadURL(`${protocolName}://host/indexedDB`)
      })

      it('cannot access cookie', (done) => {
        ipcMain.once('cookie-response', (event, cookie) => {
          assert(!cookie)
          done()
        })
        contents.loadURL(`${protocolName}://host/cookie`)
      })
    })

    describe('can be accessed', () => {
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

      const testLocalStorageAfterXSiteRedirect = (testTitle, extraPreferences = {}) => {
        it(testTitle, (done) => {
          w = new BrowserWindow({
            show: false,
            ...extraPreferences
          })
          let redirected = false
          w.webContents.on('crashed', () => {
            assert.fail('renderer crashed / was killed')
          })
          w.webContents.on('did-redirect-navigation', (event, url) => {
            assert.strictEqual(url, `${server.cross_site_url}/redirected`)
            redirected = true
          })
          w.webContents.on('did-finish-load', () => {
            assert.strictEqual(redirected, true, 'didnt redirect')
            done()
          })
          w.loadURL(`${server.url}/redirect-cross-site`)
        })
      }

      testLocalStorageAfterXSiteRedirect('after a cross-site redirect')
      testLocalStorageAfterXSiteRedirect('after a cross-site redirect in sandbox mode', { sandbox: true })
    })
  })

  describe('websockets', () => {
    let wss = null
    let server = null
    const WebSocketServer = ws.Server

    afterEach(() => {
      wss.close()
      server.close()
    })

    it('has user agent', (done) => {
      server = http.createServer()
      server.listen(0, '127.0.0.1', () => {
        const port = server.address().port
        wss = new WebSocketServer({ server: server })
        wss.on('error', done)
        wss.on('connection', (ws, upgradeReq) => {
          if (upgradeReq.headers['user-agent']) {
            done()
          } else {
            done('user agent is empty')
          }
        })
        const socket = new WebSocket(`ws://127.0.0.1:${port}`)
        assert(socket)
      })
    })
  })

  describe('Promise', () => {
    it('resolves correctly in Node.js calls', (done) => {
      document.registerElement('x-element', {
        prototype: Object.create(HTMLElement.prototype, {
          createdCallback: {
            value: () => {}
          }
        })
      })
      setImmediate(() => {
        let called = false
        Promise.resolve().then(() => {
          done(called ? void 0 : new Error('wrong sequence'))
        })
        document.createElement('x-element')
        called = true
      })
    })

    it('resolves correctly in Electron calls', (done) => {
      document.registerElement('y-element', {
        prototype: Object.create(HTMLElement.prototype, {
          createdCallback: {
            value: () => {}
          }
        })
      })
      remote.getGlobal('setImmediate')(() => {
        let called = false
        Promise.resolve().then(() => {
          done(called ? void 0 : new Error('wrong sequence'))
        })
        document.createElement('y-element')
        called = true
      })
    })
  })

  describe('fetch', () => {
    it('does not crash', (done) => {
      const server = http.createServer((req, res) => {
        res.end('test')
        server.close()
      })
      server.listen(0, '127.0.0.1', () => {
        const port = server.address().port
        fetch(`http://127.0.0.1:${port}`).then((res) => res.body.getReader())
          .then((reader) => {
            reader.read().then((r) => {
              reader.cancel()
              done()
            })
          }).catch((e) => done(e))
      })
    })
  })

  describe('PDF Viewer', () => {
    before(function () {
      if (!features.isPDFViewerEnabled()) {
        return this.skip()
      }
    })

    beforeEach(() => {
      this.pdfSource = url.format({
        pathname: path.join(fixtures, 'assets', 'cat.pdf').replace(/\\/g, '/'),
        protocol: 'file',
        slashes: true
      })

      this.pdfSourceWithParams = url.format({
        pathname: path.join(fixtures, 'assets', 'cat.pdf').replace(/\\/g, '/'),
        query: {
          a: 1,
          b: 2
        },
        protocol: 'file',
        slashes: true
      })

      this.createBrowserWindow = ({ plugins, preload }) => {
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            preload: path.join(fixtures, 'module', preload),
            plugins: plugins
          }
        })
      }

      this.testPDFIsLoadedInSubFrame = (page, preloadFile, done) => {
        const pagePath = url.format({
          pathname: path.join(fixtures, 'pages', page).replace(/\\/g, '/'),
          protocol: 'file',
          slashes: true
        })

        this.createBrowserWindow({ plugins: true, preload: preloadFile })
        ipcMain.once('pdf-loaded', (event, state) => {
          assert.strictEqual(state, 'success')
          done()
        })
        w.webContents.on('page-title-updated', () => {
          const parsedURL = url.parse(w.webContents.getURL(), true)
          assert.strictEqual(parsedURL.protocol, 'chrome:')
          assert.strictEqual(parsedURL.hostname, 'pdf-viewer')
          assert.strictEqual(parsedURL.query.src, pagePath)
          assert.strictEqual(w.webContents.getTitle(), 'cat.pdf')
        })
        w.loadFile(path.join(fixtures, 'pages', page))
      }
    })

    it('opens when loading a pdf resource as top level navigation', (done) => {
      this.createBrowserWindow({ plugins: true, preload: 'preload-pdf-loaded.js' })
      ipcMain.once('pdf-loaded', (event, state) => {
        assert.strictEqual(state, 'success')
        done()
      })
      w.webContents.on('page-title-updated', () => {
        const parsedURL = url.parse(w.webContents.getURL(), true)
        assert.strictEqual(parsedURL.protocol, 'chrome:')
        assert.strictEqual(parsedURL.hostname, 'pdf-viewer')
        assert.strictEqual(parsedURL.query.src, this.pdfSource)
        assert.strictEqual(w.webContents.getTitle(), 'cat.pdf')
      })
      w.webContents.loadURL(this.pdfSource)
    })

    it('opens a pdf link given params, the query string should be escaped', (done) => {
      this.createBrowserWindow({ plugins: true, preload: 'preload-pdf-loaded.js' })
      ipcMain.once('pdf-loaded', (event, state) => {
        assert.strictEqual(state, 'success')
        done()
      })
      w.webContents.on('page-title-updated', () => {
        const parsedURL = url.parse(w.webContents.getURL(), true)
        assert.strictEqual(parsedURL.protocol, 'chrome:')
        assert.strictEqual(parsedURL.hostname, 'pdf-viewer')
        assert.strictEqual(parsedURL.query.src, this.pdfSourceWithParams)
        assert.strictEqual(parsedURL.query.b, undefined)
        assert(parsedURL.search.endsWith('%3Fa%3D1%26b%3D2'))
        assert.strictEqual(w.webContents.getTitle(), 'cat.pdf')
      })
      w.webContents.loadURL(this.pdfSourceWithParams)
    })

    it('should download a pdf when plugins are disabled', (done) => {
      this.createBrowserWindow({ plugins: false, preload: 'preload-pdf-loaded.js' })
      ipcRenderer.sendSync('set-download-option', false, false)
      ipcRenderer.once('download-done', (event, state, url, mimeType, receivedBytes, totalBytes, disposition, filename) => {
        assert.strictEqual(state, 'completed')
        assert.strictEqual(filename, 'cat.pdf')
        assert.strictEqual(mimeType, 'application/pdf')
        fs.unlinkSync(path.join(fixtures, 'mock.pdf'))
        done()
      })
      w.webContents.loadURL(this.pdfSource)
    })

    it('should not open when pdf is requested as sub resource', (done) => {
      fetch(this.pdfSource).then((res) => {
        assert.strictEqual(res.status, 200)
        assert.notStrictEqual(document.title, 'cat.pdf')
        done()
      }).catch((e) => done(e))
    })

    it('opens when loading a pdf resource in a iframe', (done) => {
      this.testPDFIsLoadedInSubFrame('pdf-in-iframe.html', 'preload-pdf-loaded-in-subframe.js', done)
    })

    it('opens when loading a pdf resource in a nested iframe', (done) => {
      this.testPDFIsLoadedInSubFrame('pdf-in-nested-iframe.html', 'preload-pdf-loaded-in-nested-subframe.js', done)
    })
  })

  describe('window.alert(message, title)', () => {
    it('throws an exception when the arguments cannot be converted to strings', () => {
      assert.throws(() => {
        window.alert({ toString: null })
      }, /Cannot convert object to primitive value/)
    })
  })

  describe('window.confirm(message, title)', () => {
    it('throws an exception when the arguments cannot be converted to strings', () => {
      assert.throws(() => {
        window.confirm({ toString: null }, 'title')
      }, /Cannot convert object to primitive value/)
    })
  })

  describe('window.history', () => {
    describe('window.history.go(offset)', () => {
      it('throws an exception when the argumnet cannot be converted to a string', () => {
        assert.throws(() => {
          window.history.go({ toString: null })
        }, /Cannot convert object to primitive value/)
      })
    })

    describe('window.history.pushState', () => {
      it('should push state after calling history.pushState() from the same url', (done) => {
        w = new BrowserWindow({ show: false })
        w.webContents.once('did-finish-load', () => {
          // History should have current page by now.
          assert.strictEqual(w.webContents.length(), 1)

          w.webContents.executeJavaScript('window.history.pushState({}, "")', () => {
            // Initial page + pushed state
            assert.strictEqual(w.webContents.length(), 2)
            done()
          })
        })
        w.loadURL('about:blank')
      })
    })
  })

  describe('SpeechSynthesis', () => {
    before(function () {
      if (isCI || !features.isTtsEnabled()) {
        this.skip()
      }
    })

    it('should emit lifecycle events', async () => {
      const sentence = `long sentence which will take at least a few seconds to
          utter so that it's possible to pause and resume before the end`
      const utter = new SpeechSynthesisUtterance(sentence)
      // Create a dummy utterence so that speech synthesis state
      // is initialized for later calls.
      speechSynthesis.speak(new SpeechSynthesisUtterance())
      speechSynthesis.cancel()
      speechSynthesis.speak(utter)
      // paused state after speak()
      expect(speechSynthesis.paused).to.be.false()
      await new Promise((resolve) => { utter.onstart = resolve })
      // paused state after start event
      expect(speechSynthesis.paused).to.be.false()

      speechSynthesis.pause()
      // paused state changes async, right before the pause event
      expect(speechSynthesis.paused).to.be.false()
      await new Promise((resolve) => { utter.onpause = resolve })
      expect(speechSynthesis.paused).to.be.true()

      speechSynthesis.resume()
      await new Promise((resolve) => { utter.onresume = resolve })
      // paused state after resume event
      expect(speechSynthesis.paused).to.be.false()

      await new Promise((resolve) => { utter.onend = resolve })
    })
  })

  describe('focus handling', () => {
    let webviewContents = null

    beforeEach(async () => {
      w = new BrowserWindow({
        show: true,
        webPreferences: {
          nodeIntegration: true,
          webviewTag: true
        }
      })

      const webviewReady = emittedOnce(w.webContents, 'did-attach-webview')
      await w.loadFile(path.join(fixtures, 'pages', 'tab-focus-loop-elements.html'))
      const [, wvContents] = await webviewReady
      webviewContents = wvContents
      await emittedOnce(webviewContents, 'did-finish-load')
      w.focus()
    })

    afterEach(() => {
      webviewContents = null
    })

    const expectFocusChange = async () => {
      const [, focusedElementId] = await emittedOnce(ipcMain, 'focus-changed')
      return focusedElementId
    }

    describe('a TAB press', () => {
      const tabPressEvent = {
        type: 'keyDown',
        keyCode: 'Tab'
      }

      it('moves focus to the next focusable item', async () => {
        let focusChange = expectFocusChange()
        w.webContents.sendInputEvent(tabPressEvent)
        let focusedElementId = await focusChange
        assert.strictEqual(focusedElementId, 'BUTTON-element-1', `should start focused in element-1, it's instead in ${focusedElementId}`)

        focusChange = expectFocusChange()
        w.webContents.sendInputEvent(tabPressEvent)
        focusedElementId = await focusChange
        assert.strictEqual(focusedElementId, 'BUTTON-element-2', `focus should've moved to element-2, it's instead in ${focusedElementId}`)

        focusChange = expectFocusChange()
        w.webContents.sendInputEvent(tabPressEvent)
        focusedElementId = await focusChange
        assert.strictEqual(focusedElementId, 'BUTTON-wv-element-1', `focus should've moved to the webview's element-1, it's instead in ${focusedElementId}`)

        focusChange = expectFocusChange()
        webviewContents.sendInputEvent(tabPressEvent)
        focusedElementId = await focusChange
        assert.strictEqual(focusedElementId, 'BUTTON-wv-element-2', `focus should've moved to the webview's element-2, it's instead in ${focusedElementId}`)

        focusChange = expectFocusChange()
        webviewContents.sendInputEvent(tabPressEvent)
        focusedElementId = await focusChange
        assert.strictEqual(focusedElementId, 'BUTTON-element-3', `focus should've moved to element-3, it's instead in ${focusedElementId}`)

        focusChange = expectFocusChange()
        w.webContents.sendInputEvent(tabPressEvent)
        focusedElementId = await focusChange
        assert.strictEqual(focusedElementId, 'BUTTON-element-1', `focus should've looped back to element-1, it's instead in ${focusedElementId}`)
      })
    })

    describe('a SHIFT + TAB press', () => {
      const shiftTabPressEvent = {
        type: 'keyDown',
        modifiers: ['Shift'],
        keyCode: 'Tab'
      }

      it('moves focus to the previous focusable item', async () => {
        let focusChange = expectFocusChange()
        w.webContents.sendInputEvent(shiftTabPressEvent)
        let focusedElementId = await focusChange
        assert.strictEqual(focusedElementId, 'BUTTON-element-3', `should start focused in element-3, it's instead in ${focusedElementId}`)

        focusChange = expectFocusChange()
        w.webContents.sendInputEvent(shiftTabPressEvent)
        focusedElementId = await focusChange
        assert.strictEqual(focusedElementId, 'BUTTON-wv-element-2', `focus should've moved to the webview's element-2, it's instead in ${focusedElementId}`)

        focusChange = expectFocusChange()
        webviewContents.sendInputEvent(shiftTabPressEvent)
        focusedElementId = await focusChange
        assert.strictEqual(focusedElementId, 'BUTTON-wv-element-1', `focus should've moved to the webview's element-1, it's instead in ${focusedElementId}`)

        focusChange = expectFocusChange()
        webviewContents.sendInputEvent(shiftTabPressEvent)
        focusedElementId = await focusChange
        assert.strictEqual(focusedElementId, 'BUTTON-element-2', `focus should've moved to element-2, it's instead in ${focusedElementId}`)

        focusChange = expectFocusChange()
        w.webContents.sendInputEvent(shiftTabPressEvent)
        focusedElementId = await focusChange
        assert.strictEqual(focusedElementId, 'BUTTON-element-1', `focus should've moved to element-1, it's instead in ${focusedElementId}`)

        focusChange = expectFocusChange()
        w.webContents.sendInputEvent(shiftTabPressEvent)
        focusedElementId = await focusChange
        assert.strictEqual(focusedElementId, 'BUTTON-element-3', `focus should've looped back to element-3, it's instead in ${focusedElementId}`)
      })
    })
  })
})

describe('font fallback', () => {
  async function getRenderedFonts (html) {
    const w = new BrowserWindow({ show: false })
    try {
      await w.loadURL(`data:text/html,${html}`)
      w.webContents.debugger.attach()
      const sendCommand = (...args) => new Promise((resolve, reject) => {
        w.webContents.debugger.sendCommand(...args, (e, r) => {
          if (e) { reject(e) } else { resolve(r) }
        })
      })
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
    expect(fonts[0].familyName).to.equal({
      'win32': 'Arial',
      'darwin': 'Helvetica',
      'linux': 'DejaVu Sans' // I think this depends on the distro? We don't specify a default.
    }[process.platform])
  })

  it('should fall back to Japanese font for sans-serif Japanese script', async function () {
    if (process.platform === 'linux') {
      return this.skip()
    }
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
    expect(fonts[0].familyName).to.equal({
      'win32': 'Meiryo',
      'darwin': 'Hiragino Kaku Gothic ProN'
    }[process.platform])
  })
})

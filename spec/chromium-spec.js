const assert = require('assert')
const fs = require('fs')
const http = require('http')
const path = require('path')
const ws = require('ws')
const url = require('url')
const {ipcRenderer, remote, webFrame} = require('electron')
const {closeWindow} = require('./window-helpers')

const {app, BrowserWindow, ipcMain, protocol, session, webContents} = remote

const isCI = remote.getGlobal('isCi')

describe('chromium feature', function () {
  var fixtures = path.resolve(__dirname, 'fixtures')
  var listener = null
  let w = null

  afterEach(function () {
    if (listener != null) {
      window.removeEventListener('message', listener)
    }
    listener = null
  })

  afterEach(function () {
    return closeWindow(w).then(function () { w = null })
  })

  describe('heap snapshot', function () {
    it('does not crash', function () {
      if (process.env.TRAVIS === 'true') return
      process.atomBinding('v8_util').takeHeapSnapshot()
    })
  })

  describe('sending request of http protocol urls', function () {
    it('does not crash', function (done) {
      var server = http.createServer(function (req, res) {
        res.end()
        server.close()
        done()
      })
      server.listen(0, '127.0.0.1', function () {
        var port = server.address().port
        $.get('http://127.0.0.1:' + port)
      })
    })
  })

  xdescribe('navigator.webkitGetUserMedia', function () {
    it('calls its callbacks', function (done) {
      navigator.webkitGetUserMedia({
        audio: true,
        video: false
      }, function () {
        done()
      }, function () {
        done()
      })
    })
  })

  describe('navigator.mediaDevices', function () {
    if (isCI) {
      return
    }

    it('can return labels of enumerated devices', function (done) {
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

    it('can return new device id when cookie storage is cleared', function (done) {
      const options = {
        origin: null,
        storages: ['cookies']
      }
      const deviceIds = []
      const ses = session.fromPartition('persist:media-device-id')
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          session: ses
        }
      })
      w.webContents.on('ipc-message', function (event, args) {
        if (args[0] === 'deviceIds') {
          deviceIds.push(args[1])
        }
        if (deviceIds.length === 2) {
          assert.notDeepEqual(deviceIds[0], deviceIds[1])
          closeWindow(w).then(function () {
            w = null
            done()
          }).catch(function (error) {
            done(error)
          })
        } else {
          ses.clearStorageData(options, function () {
            w.webContents.reload()
          })
        }
      })
      w.loadURL('file://' + fixtures + '/pages/media-id-reset.html')
    })
  })

  describe('navigator.language', function () {
    it('should not be empty', function () {
      assert.notEqual(navigator.language, '')
    })
  })

  describe('navigator.serviceWorker', function () {
    it('should register for file scheme', function (done) {
      w = new BrowserWindow({
        show: false
      })
      w.webContents.on('ipc-message', function (event, args) {
        if (args[0] === 'reload') {
          w.webContents.reload()
        } else if (args[0] === 'error') {
          done('unexpected error : ' + args[1])
        } else if (args[0] === 'response') {
          assert.equal(args[1], 'Hello from serviceWorker!')
          session.defaultSession.clearStorageData({
            storages: ['serviceworkers']
          }, function () {
            done()
          })
        }
      })
      w.loadURL(`file://${fixtures}/pages/service-worker/index.html`)
    })

    it('should register for intercepted file scheme', function (done) {
      const customSession = session.fromPartition('intercept-file')
      customSession.protocol.interceptBufferProtocol('file', function (request, callback) {
        let file = url.parse(request.url).pathname
        // Remove leading slash before drive letter on Windows
        if (file[0] === '/' && process.platform === 'win32') {
          file = file.slice(1)
        }
        const content = fs.readFileSync(path.normalize(file))
        const ext = path.extname(file)
        let type = 'text/html'
        if (ext === '.js') {
          type = 'application/javascript'
        }
        callback({data: content, mimeType: type})
      }, function (error) {
        if (error) done(error)
      })

      w = new BrowserWindow({
        show: false,
        webPreferences: {
          session: customSession
        }
      })
      w.webContents.on('ipc-message', function (event, args) {
        if (args[0] === 'reload') {
          w.webContents.reload()
        } else if (args[0] === 'error') {
          done('unexpected error : ' + args[1])
        } else if (args[0] === 'response') {
          assert.equal(args[1], 'Hello from serviceWorker!')
          customSession.clearStorageData({
            storages: ['serviceworkers']
          }, function () {
            customSession.protocol.uninterceptProtocol('file', (error) => done(error))
          })
        }
      })
      w.loadURL(`file://${fixtures}/pages/service-worker/index.html`)
    })
  })

  describe('window.open', function () {
    if (process.env.TRAVIS === 'true' && process.platform === 'darwin') {
      return
    }

    it('returns a BrowserWindowProxy object', function () {
      var b = window.open('about:blank', '', 'show=no')
      assert.equal(b.closed, false)
      assert.equal(b.constructor.name, 'BrowserWindowProxy')
      b.close()
    })

    it('accepts "nodeIntegration" as feature', function (done) {
      var b
      listener = function (event) {
        assert.equal(event.data.isProcessGlobalUndefined, true)
        b.close()
        done()
      }
      window.addEventListener('message', listener)
      b = window.open('file://' + fixtures + '/pages/window-opener-node.html', '', 'nodeIntegration=no,show=no')
    })

    it('inherit options of parent window', function (done) {
      var b
      listener = function (event) {
        var ref1 = remote.getCurrentWindow().getSize()
        var width = ref1[0]
        var height = ref1[1]
        assert.equal(event.data, 'size: ' + width + ' ' + height)
        b.close()
        done()
      }
      window.addEventListener('message', listener)
      b = window.open('file://' + fixtures + '/pages/window-open-size.html', '', 'show=no')
    })

    it('disables node integration when it is disabled on the parent window', function (done) {
      var b
      listener = function (event) {
        assert.equal(event.data.isProcessGlobalUndefined, true)
        b.close()
        done()
      }
      window.addEventListener('message', listener)

      var windowUrl = require('url').format({
        pathname: `${fixtures}/pages/window-opener-no-node-integration.html`,
        protocol: 'file',
        query: {
          p: `${fixtures}/pages/window-opener-node.html`
        },
        slashes: true
      })
      b = window.open(windowUrl, '', 'nodeIntegration=no,show=no')
    })

    it('disables node integration when it is disabled on the parent window for chrome devtools URLs', function (done) {
      var b
      app.once('web-contents-created', (event, contents) => {
        contents.once('did-finish-load', () => {
          contents.executeJavaScript('typeof process').then((typeofProcessGlobal) => {
            assert.equal(typeofProcessGlobal, 'undefined')
            b.close()
            done()
          }).catch(done)
        })
      })
      b = window.open('chrome-devtools://devtools/bundled/inspector.html', '', 'nodeIntegration=no,show=no')
    })

    it('disables JavaScript when it is disabled on the parent window', function (done) {
      var b
      app.once('web-contents-created', (event, contents) => {
        contents.once('did-finish-load', () => {
          app.once('browser-window-created', (event, window) => {
            const preferences = window.webContents.getWebPreferences()
            assert.equal(preferences.javascript, false)
            window.destroy()
            b.close()
            done()
          })
          // Click link on page
          contents.sendInputEvent({type: 'mouseDown', clickCount: 1, x: 1, y: 1})
          contents.sendInputEvent({type: 'mouseUp', clickCount: 1, x: 1, y: 1})
        })
      })

      var windowUrl = require('url').format({
        pathname: `${fixtures}/pages/window-no-javascript.html`,
        protocol: 'file',
        slashes: true
      })
      b = window.open(windowUrl, '', 'javascript=no,show=no')
    })

    it('disables the <webview> tag when it is disabled on the parent window', function (done) {
      let b
      listener = function (event) {
        assert.equal(event.data.isWebViewGlobalUndefined, true)
        b.close()
        done()
      }
      window.addEventListener('message', listener)

      var windowUrl = require('url').format({
        pathname: `${fixtures}/pages/window-opener-no-webview-tag.html`,
        protocol: 'file',
        query: {
          p: `${fixtures}/pages/window-opener-webview.html`
        },
        slashes: true
      })
      b = window.open(windowUrl, '', 'webviewTag=no,nodeIntegration=yes,show=no')
    })

    it('does not override child options', function (done) {
      var b, size
      size = {
        width: 350,
        height: 450
      }
      listener = function (event) {
        assert.equal(event.data, 'size: ' + size.width + ' ' + size.height)
        b.close()
        done()
      }
      window.addEventListener('message', listener)
      b = window.open('file://' + fixtures + '/pages/window-open-size.html', '', 'show=no,width=' + size.width + ',height=' + size.height)
    })

    it('handles cycles when merging the parent options into the child options', (done) => {
      w = BrowserWindow.fromId(ipcRenderer.sendSync('create-window-with-options-cycle'))
      w.loadURL('file://' + fixtures + '/pages/window-open.html')
      w.webContents.once('new-window', (event, url, frameName, disposition, options) => {
        assert.equal(options.show, false)
        assert.deepEqual(options.foo, {
          bar: null,
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

    it('defines a window.location getter', function (done) {
      var b, targetURL
      if (process.platform === 'win32') {
        targetURL = 'file:///' + fixtures.replace(/\\/g, '/') + '/pages/base-page.html'
      } else {
        targetURL = 'file://' + fixtures + '/pages/base-page.html'
      }
      app.once('browser-window-created', (event, window) => {
        window.webContents.once('did-finish-load', () => {
          assert.equal(b.location, targetURL)
          b.close()
          done()
        })
      })
      b = window.open(targetURL)
    })

    it('defines a window.location setter', function (done) {
      let b
      app.once('browser-window-created', (event, {webContents}) => {
        webContents.once('did-finish-load', function () {
          // When it loads, redirect
          b.location = 'file://' + fixtures + '/pages/base-page.html'
          webContents.once('did-finish-load', function () {
            // After our second redirect, cleanup and callback
            b.close()
            done()
          })
        })
      })
      // Load a page that definitely won't redirect
      b = window.open('about:blank')
    })

    it('open a blank page when no URL is specified', function (done) {
      let b
      app.once('browser-window-created', (event, {webContents}) => {
        webContents.once('did-finish-load', function () {
          const {location} = b
          b.close()
          assert.equal(location, 'about:blank')

          let c
          app.once('browser-window-created', (event, {webContents}) => {
            webContents.once('did-finish-load', function () {
              const {location} = c
              c.close()
              assert.equal(location, 'about:blank')
              done()
            })
          })
          c = window.open('')
        })
      })
      b = window.open()
    })

    it('throws an exception when the arguments cannot be converted to strings', function () {
      assert.throws(function () {
        window.open('', {toString: null})
      }, /Cannot convert object to primitive value/)

      assert.throws(function () {
        window.open('', '', {toString: 3})
      }, /Cannot convert object to primitive value/)
    })

    it('sets the window title to the specified frameName', function (done) {
      let b
      app.once('browser-window-created', (event, createdWindow) => {
        assert.equal(createdWindow.getTitle(), 'hello')
        b.close()
        done()
      })
      b = window.open('', 'hello')
    })

    it('does not throw an exception when the frameName is a built-in object property', function (done) {
      let b
      app.once('browser-window-created', (event, createdWindow) => {
        assert.equal(createdWindow.getTitle(), '__proto__')
        b.close()
        done()
      })
      b = window.open('', '__proto__')
    })

    it('does not throw an exception when the features include webPreferences', function () {
      let b
      assert.doesNotThrow(function () {
        b = window.open('', '', 'webPreferences=')
      })
      b.close()
    })
  })

  describe('window.opener', function () {
    let url = 'file://' + fixtures + '/pages/window-opener.html'

    it('is null for main window', function (done) {
      w = new BrowserWindow({
        show: false
      })
      w.webContents.once('ipc-message', function (event, args) {
        assert.deepEqual(args, ['opener', null])
        done()
      })
      w.loadURL(url)
    })

    it('is not null for window opened by window.open', function (done) {
      let b
      listener = function (event) {
        assert.equal(event.data, 'object')
        b.close()
        done()
      }
      window.addEventListener('message', listener)
      b = window.open(url, '', 'show=no')
    })
  })

  describe('window.opener access from BrowserWindow', function () {
    const scheme = 'other'
    let url = `${scheme}://${fixtures}/pages/window-opener-location.html`
    let w = null

    before(function (done) {
      protocol.registerFileProtocol(scheme, function (request, callback) {
        callback(`${fixtures}/pages/window-opener-location.html`)
      }, function (error) {
        done(error)
      })
    })

    after(function () {
      protocol.unregisterProtocol(scheme)
    })

    afterEach(function () {
      w.close()
    })

    it('does nothing when origin of current window does not match opener', function (done) {
      listener = function (event) {
        assert.equal(event.data, undefined)
        done()
      }
      window.addEventListener('message', listener)
      w = window.open(url, '', 'show=no')
    })

    it('works when origin matches', function (done) {
      listener = function (event) {
        assert.equal(event.data, location.href)
        done()
      }
      window.addEventListener('message', listener)
      w = window.open(`file://${fixtures}/pages/window-opener-location.html`, '', 'show=no')
    })

    it('works when origin does not match opener but has node integration', function (done) {
      listener = function (event) {
        assert.equal(event.data, location.href)
        done()
      }
      window.addEventListener('message', listener)
      w = window.open(url, '', 'show=no,nodeIntegration=yes')
    })
  })

  describe('window.opener access from <webview>', function () {
    const scheme = 'other'
    const srcPath = `${fixtures}/pages/webview-opener-postMessage.html`
    const pageURL = `file://${fixtures}/pages/window-opener-location.html`
    let webview = null

    before(function (done) {
      protocol.registerFileProtocol(scheme, function (request, callback) {
        callback(srcPath)
      }, function (error) {
        done(error)
      })
    })

    after(function () {
      protocol.unregisterProtocol(scheme)
    })

    afterEach(function () {
      if (webview != null) webview.remove()
    })

    it('does nothing when origin of webview src URL does not match opener', function (done) {
      webview = new WebView()
      webview.addEventListener('console-message', function (e) {
        assert.equal(e.message, 'null')
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

    it('works when origin matches', function (done) {
      webview = new WebView()
      webview.addEventListener('console-message', function (e) {
        assert.equal(e.message, webview.src)
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

    it('works when origin does not match opener but has node integration', function (done) {
      webview = new WebView()
      webview.addEventListener('console-message', function (e) {
        webview.remove()
        assert.equal(e.message, webview.src)
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

  describe('window.postMessage', function () {
    it('sets the source and origin correctly', function (done) {
      var b
      listener = function (event) {
        window.removeEventListener('message', listener)
        b.close()
        var message = JSON.parse(event.data)
        assert.equal(message.data, 'testing')
        assert.equal(message.origin, 'file://')
        assert.equal(message.sourceEqualsOpener, true)
        assert.equal(event.origin, 'file://')
        done()
      }
      window.addEventListener('message', listener)
      app.once('browser-window-created', (event, {webContents}) => {
        webContents.once('did-finish-load', function () {
          b.postMessage('testing', '*')
        })
      })
      b = window.open('file://' + fixtures + '/pages/window-open-postMessage.html', '', 'show=no')
    })

    it('throws an exception when the targetOrigin cannot be converted to a string', function () {
      var b = window.open('')
      assert.throws(function () {
        b.postMessage('test', {toString: null})
      }, /Cannot convert object to primitive value/)
      b.close()
    })
  })

  describe('window.opener.postMessage', function () {
    it('sets source and origin correctly', function (done) {
      var b
      listener = function (event) {
        window.removeEventListener('message', listener)
        b.close()
        assert.equal(event.source, b)
        assert.equal(event.origin, 'file://')
        done()
      }
      window.addEventListener('message', listener)
      b = window.open('file://' + fixtures + '/pages/window-opener-postMessage.html', '', 'show=no')
    })

    it('supports windows opened from a <webview>', function (done) {
      const webview = new WebView()
      webview.addEventListener('console-message', function (e) {
        webview.remove()
        assert.equal(e.message, 'message')
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

    describe('targetOrigin argument', function () {
      let serverURL
      let server

      beforeEach(function (done) {
        server = http.createServer(function (req, res) {
          res.writeHead(200)
          const filePath = path.join(fixtures, 'pages', 'window-opener-targetOrigin.html')
          res.end(fs.readFileSync(filePath, 'utf8'))
        })
        server.listen(0, '127.0.0.1', function () {
          serverURL = `http://127.0.0.1:${server.address().port}`
          done()
        })
      })

      afterEach(function () {
        server.close()
      })

      it('delivers messages that match the origin', function (done) {
        let b
        listener = function (event) {
          window.removeEventListener('message', listener)
          b.close()
          assert.equal(event.data, 'deliver')
          done()
        }
        window.addEventListener('message', listener)
        b = window.open(serverURL, '', 'show=no')
      })
    })
  })

  describe('creating a Uint8Array under browser side', function () {
    it('does not crash', function () {
      var RUint8Array = remote.getGlobal('Uint8Array')
      var arr = new RUint8Array()
      assert(arr)
    })
  })

  describe('webgl', function () {
    if (isCI && process.platform === 'win32') {
      return
    }

    it('can be get as context in canvas', function () {
      if (process.platform === 'linux') return

      var webgl = document.createElement('canvas').getContext('webgl')
      assert.notEqual(webgl, null)
    })
  })

  describe('web workers', function () {
    it('Worker can work', function (done) {
      var worker = new Worker('../fixtures/workers/worker.js')
      var message = 'ping'
      worker.onmessage = function (event) {
        assert.equal(event.data, message)
        worker.terminate()
        done()
      }
      worker.postMessage(message)
    })

    it('Worker has no node integration by default', function (done) {
      let worker = new Worker('../fixtures/workers/worker_node.js')
      worker.onmessage = function (event) {
        assert.equal(event.data, 'undefined undefined undefined undefined')
        worker.terminate()
        done()
      }
    })

    it('Worker has node integration with nodeIntegrationInWorker', function (done) {
      let webview = new WebView()
      webview.addEventListener('ipc-message', function (e) {
        assert.equal(e.channel, 'object function object function')
        webview.remove()
        done()
      })
      webview.src = 'file://' + fixtures + '/pages/worker.html'
      webview.setAttribute('webpreferences', 'nodeIntegration, nodeIntegrationInWorker')
      document.body.appendChild(webview)
    })

    it('SharedWorker can work', function (done) {
      var worker = new SharedWorker('../fixtures/workers/shared_worker.js')
      var message = 'ping'
      worker.port.onmessage = function (event) {
        assert.equal(event.data, message)
        done()
      }
      worker.port.postMessage(message)
    })

    it('SharedWorker has no node integration by default', function (done) {
      let worker = new SharedWorker('../fixtures/workers/shared_worker_node.js')
      worker.port.onmessage = function (event) {
        assert.equal(event.data, 'undefined undefined undefined undefined')
        done()
      }
    })

    it('SharedWorker has node integration with nodeIntegrationInWorker', function (done) {
      let webview = new WebView()
      webview.addEventListener('console-message', function (e) {
        console.log(e)
      })
      webview.addEventListener('ipc-message', function (e) {
        assert.equal(e.channel, 'object function object function')
        webview.remove()
        done()
      })
      webview.src = 'file://' + fixtures + '/pages/shared_worker.html'
      webview.setAttribute('webpreferences', 'nodeIntegration, nodeIntegrationInWorker')
      document.body.appendChild(webview)
    })
  })

  describe('iframe', function () {
    var iframe = null

    beforeEach(function () {
      iframe = document.createElement('iframe')
    })

    afterEach(function () {
      document.body.removeChild(iframe)
    })

    it('does not have node integration', function (done) {
      iframe.src = 'file://' + fixtures + '/pages/set-global.html'
      document.body.appendChild(iframe)
      iframe.onload = function () {
        assert.equal(iframe.contentWindow.test, 'undefined undefined undefined')
        done()
      }
    })
  })

  describe('storage', function () {
    it('requesting persitent quota works', function (done) {
      navigator.webkitPersistentStorage.requestQuota(1024 * 1024, function (grantedBytes) {
        assert.equal(grantedBytes, 1048576)
        done()
      })
    })

    describe('custom non standard schemes', function () {
      const protocolName = 'storage'
      let contents = null
      before(function (done) {
        const handler = function (request, callback) {
          let parsedUrl = url.parse(request.url)
          let filename
          switch (parsedUrl.pathname) {
            case '/localStorage' : filename = 'local_storage.html'; break
            case '/sessionStorage' : filename = 'session_storage.html'; break
            case '/WebSQL' : filename = 'web_sql.html'; break
            case '/indexedDB' : filename = 'indexed_db.html'; break
            case '/cookie' : filename = 'cookie.html'; break
            default : filename = ''
          }
          callback({path: fixtures + '/pages/storage/' + filename})
        }
        protocol.registerFileProtocol(protocolName, handler, function (error) {
          done(error)
        })
      })

      after(function (done) {
        protocol.unregisterProtocol(protocolName, () => done())
      })

      beforeEach(function () {
        contents = webContents.create({})
      })

      afterEach(function () {
        contents.destroy()
        contents = null
      })

      it('cannot access localStorage', function (done) {
        ipcMain.once('local-storage-response', function (event, error) {
          assert.equal(
            error,
            'Failed to read the \'localStorage\' property from \'Window\': Access is denied for this document.')
          done()
        })
        contents.loadURL(protocolName + '://host/localStorage')
      })

      it('cannot access sessionStorage', function (done) {
        ipcMain.once('session-storage-response', function (event, error) {
          assert.equal(
            error,
            'Failed to read the \'sessionStorage\' property from \'Window\': Access is denied for this document.')
          done()
        })
        contents.loadURL(protocolName + '://host/sessionStorage')
      })

      it('cannot access WebSQL database', function (done) {
        ipcMain.once('web-sql-response', function (event, error) {
          assert.equal(
            error,
            'An attempt was made to break through the security policy of the user agent.')
          done()
        })
        contents.loadURL(protocolName + '://host/WebSQL')
      })

      it('cannot access indexedDB', function (done) {
        ipcMain.once('indexed-db-response', function (event, error) {
          assert.equal(error, 'The user denied permission to access the database.')
          done()
        })
        contents.loadURL(protocolName + '://host/indexedDB')
      })

      it('cannot access cookie', function (done) {
        ipcMain.once('cookie-response', function (event, cookie) {
          assert(!cookie)
          done()
        })
        contents.loadURL(protocolName + '://host/cookie')
      })
    })
  })

  describe('websockets', function () {
    var wss = null
    var server = null
    var WebSocketServer = ws.Server

    afterEach(function () {
      wss.close()
      server.close()
    })

    it('has user agent', function (done) {
      server = http.createServer()
      server.listen(0, '127.0.0.1', function () {
        var port = server.address().port
        wss = new WebSocketServer({
          server: server
        })
        wss.on('error', done)
        wss.on('connection', function (ws) {
          if (ws.upgradeReq.headers['user-agent']) {
            done()
          } else {
            done('user agent is empty')
          }
        })
        var socket = new WebSocket(`ws://127.0.0.1:${port}`)
        assert(socket)
      })
    })
  })

  describe('Promise', function () {
    it('resolves correctly in Node.js calls', function (done) {
      document.registerElement('x-element', {
        prototype: Object.create(HTMLElement.prototype, {
          createdCallback: {
            value: function () {}
          }
        })
      })
      setImmediate(function () {
        var called = false
        Promise.resolve().then(function () {
          done(called ? void 0 : new Error('wrong sequence'))
        })
        document.createElement('x-element')
        called = true
      })
    })

    it('resolves correctly in Electron calls', function (done) {
      document.registerElement('y-element', {
        prototype: Object.create(HTMLElement.prototype, {
          createdCallback: {
            value: function () {}
          }
        })
      })
      remote.getGlobal('setImmediate')(function () {
        var called = false
        Promise.resolve().then(function () {
          done(called ? void 0 : new Error('wrong sequence'))
        })
        document.createElement('y-element')
        called = true
      })
    })
  })

  describe('fetch', function () {
    it('does not crash', function (done) {
      const server = http.createServer(function (req, res) {
        res.end('test')
        server.close()
      })
      server.listen(0, '127.0.0.1', function () {
        const port = server.address().port
        fetch(`http://127.0.0.1:${port}`).then((res) => {
          return res.body.getReader()
        }).then((reader) => {
          reader.read().then((r) => {
            reader.cancel()
            done()
          })
        }).catch(function (e) {
          done(e)
        })
      })
    })
  })

  describe('PDF Viewer', function () {
    const pdfSource = url.format({
      pathname: path.join(fixtures, 'assets', 'cat.pdf').replace(/\\/g, '/'),
      protocol: 'file',
      slashes: true
    })

    function createBrowserWindow ({plugins}) {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          preload: path.join(fixtures, 'module', 'preload-inject-ipc.js'),
          plugins: plugins
        }
      })
    }

    it('opens when loading a pdf resource as top level navigation', function (done) {
      createBrowserWindow({plugins: true})
      ipcMain.once('pdf-loaded', function (event, success) {
        if (success) done()
      })
      w.webContents.on('page-title-updated', function () {
        const source = `
          if (window.viewer) {
            window.viewer.setLoadCallback(function(success) {
              window.ipcRenderer.send('pdf-loaded', success);
            });
          }
        `
        const parsedURL = url.parse(w.webContents.getURL(), true)
        assert.equal(parsedURL.protocol, 'chrome:')
        assert.equal(parsedURL.hostname, 'pdf-viewer')
        assert.equal(parsedURL.query.src, pdfSource)
        assert.equal(w.webContents.getTitle(), 'cat.pdf')
        w.webContents.executeJavaScript(source)
      })
      w.webContents.loadURL(pdfSource)
    })

    it('should download a pdf when plugins are disabled', function (done) {
      createBrowserWindow({plugins: false})
      ipcRenderer.sendSync('set-download-option', false, false)
      ipcRenderer.once('download-done', function (event, state, url, mimeType, receivedBytes, totalBytes, disposition, filename) {
        assert.equal(state, 'completed')
        assert.equal(filename, 'cat.pdf')
        assert.equal(mimeType, 'application/pdf')
        fs.unlinkSync(path.join(fixtures, 'mock.pdf'))
        done()
      })
      w.webContents.loadURL(pdfSource)
    })

    it('should not open when pdf is requested as sub resource', function (done) {
      createBrowserWindow({plugins: true})
      webFrame.registerURLSchemeAsPrivileged('file', {
        secure: false,
        bypassCSP: false,
        allowServiceWorkers: false,
        corsEnabled: false
      })
      fetch(pdfSource).then(function (res) {
        assert.equal(res.status, 200)
        assert.notEqual(document.title, 'cat.pdf')
        done()
      }).catch(function (e) {
        done(e)
      })
    })
  })

  describe('window.alert(message, title)', function () {
    it('throws an exception when the arguments cannot be converted to strings', function () {
      assert.throws(function () {
        window.alert({toString: null})
      }, /Cannot convert object to primitive value/)

      assert.throws(function () {
        window.alert('message', {toString: 3})
      }, /Cannot convert object to primitive value/)
    })
  })

  describe('window.confirm(message, title)', function () {
    it('throws an exception when the arguments cannot be converted to strings', function () {
      assert.throws(function () {
        window.confirm({toString: null}, 'title')
      }, /Cannot convert object to primitive value/)

      assert.throws(function () {
        window.confirm('message', {toString: 3})
      }, /Cannot convert object to primitive value/)
    })
  })

  describe('window.history', function () {
    describe('window.history.go(offset)', function () {
      it('throws an exception when the argumnet cannot be converted to a string', function () {
        assert.throws(function () {
          window.history.go({toString: null})
        }, /Cannot convert object to primitive value/)
      })
    })

    describe('window.history.pushState', function () {
      it('should push state after calling history.pushState() from the same url', (done) => {
        w = new BrowserWindow({
          show: false
        })
        w.webContents.once('did-finish-load', () => {
          // History should have current page by now.
          assert.equal(w.webContents.length(), 1)

          w.webContents.executeJavaScript('window.history.pushState({}, "")', () => {
            // Initial page + pushed state
            assert.equal(w.webContents.length(), 2)
            done()
          })
        })
        w.loadURL('about:blank')
      })
    })
  })
})

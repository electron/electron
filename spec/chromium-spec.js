const assert = require('assert')
const http = require('http')
const path = require('path')
const ws = require('ws')
const url = require('url')
const remote = require('electron').remote

const {BrowserWindow, ipcMain, protocol, session, webContents} = remote

const isCI = remote.getGlobal('isCi')

describe('chromium feature', function () {
  var fixtures = path.resolve(__dirname, 'fixtures')
  var listener = null

  afterEach(function () {
    if (listener != null) {
      window.removeEventListener('message', listener)
    }
    listener = null
  })

  xdescribe('heap snapshot', function () {
    it('does not crash', function () {
      process.atomBinding('v8_util').takeHeapSnapshot()
    })
  })

  describe('sending request of http protocol urls', function () {
    it('does not crash', function (done) {
      this.timeout(5000)

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

  describe('document.hidden', function () {
    var url = 'file://' + fixtures + '/pages/document-hidden.html'
    var w = null

    afterEach(function () {
      w != null ? w.destroy() : void 0
    })

    it('is set correctly when window is not shown', function (done) {
      w = new BrowserWindow({
        show: false
      })
      w.webContents.once('ipc-message', function (event, args) {
        assert.deepEqual(args, ['hidden', true])
        done()
      })
      w.loadURL(url)
    })

    if (isCI && process.platform === 'win32') {
      return
    }

    it('is set correctly when window is inactive', function (done) {
      w = new BrowserWindow({
        show: false
      })
      w.webContents.once('ipc-message', function (event, args) {
        assert.deepEqual(args, ['hidden', false])
        done()
      })
      w.showInactive()
      w.loadURL(url)
    })
  })

  xdescribe('navigator.webkitGetUserMedia', function () {
    it('calls its callbacks', function (done) {
      this.timeout(5000)

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
    if (process.env.TRAVIS === 'true') {
      return
    }
    if (isCI && process.platform === 'linux') {
      return
    }
    if (isCI && process.platform === 'win32') {
      return
    }

    it('can return labels of enumerated devices', function (done) {
      navigator.mediaDevices.enumerateDevices().then((devices) => {
        const labels = devices.map((device) => device.label)
        const labelFound = labels.some((label) => !!label)
        if (labelFound) {
          done()
        } else {
          done('No device labels found: ' + JSON.stringify(labels))
        }
      }).catch(done)
    })
  })

  describe('navigator.language', function () {
    it('should not be empty', function () {
      assert.notEqual(navigator.language, '')
    })
  })

  describe('navigator.serviceWorker', function () {
    var url = 'file://' + fixtures + '/pages/service-worker/index.html'
    var w = null

    afterEach(function () {
      w != null ? w.destroy() : void 0
    })

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
      w.loadURL(url)
    })
  })

  describe('window.open', function () {
    if (process.env.TRAVIS === 'true' && process.platform === 'darwin') {
      return
    }

    this.timeout(20000)

    it('returns a BrowserWindowProxy object', function () {
      var b = window.open('about:blank', '', 'show=no')
      assert.equal(b.closed, false)
      assert.equal(b.constructor.name, 'BrowserWindowProxy')

      // Check that guestId is not writeable
      assert(b.guestId)
      b.guestId = 'anotherValue'
      assert.notEqual(b.guestId, 'anoterValue')

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

    it('defines a window.location getter', function (done) {
      var b, targetURL
      if (process.platform === 'win32') {
        targetURL = 'file:///' + fixtures.replace(/\\/g, '/') + '/pages/base-page.html'
      } else {
        targetURL = 'file://' + fixtures + '/pages/base-page.html'
      }
      b = window.open(targetURL)
      webContents.fromId(b.guestId).once('did-finish-load', function () {
        assert.equal(b.location, targetURL)
        b.close()
        done()
      })
    })

    it('defines a window.location setter', function (done) {
      // Load a page that definitely won't redirect
      var b = window.open('about:blank')
      webContents.fromId(b.guestId).once('did-finish-load', function () {
        // When it loads, redirect
        b.location = 'file://' + fixtures + '/pages/base-page.html'
        webContents.fromId(b.guestId).once('did-finish-load', function () {
          // After our second redirect, cleanup and callback
          b.close()
          done()
        })
      })
    })

    it('open a blank page when no URL is specified', function (done) {
      let b = window.open()
      webContents.fromId(b.guestId).once('did-finish-load', function () {
        const {location} = b
        b.close()
        assert.equal(location, 'about:blank')

        let c = window.open('')
        webContents.fromId(c.guestId).once('did-finish-load', function () {
          const {location} = c
          c.close()
          assert.equal(location, 'about:blank')
          done()
        })
      })
    })
  })

  describe('window.opener', function () {
    this.timeout(10000)

    let url = 'file://' + fixtures + '/pages/window-opener.html'
    let w = null

    afterEach(function () {
      if (w) w.destroy()
    })

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

  describe('window.opener security', function () {
    this.timeout(10000)

    let scheme = 'other'
    let url = `${scheme}://${fixtures}/pages/window-opener-location.html`
    let w = null

    before(function () {
      protocol.registerFileProtocol(scheme, function (request, callback) {
        callback(`${fixtures}/pages/window-opener-location.html`)
      }, function (error) {
        if (error) done(error)
      })
    })

    after(function() {
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

    it('works when origin does not match opener but has node integration', function (done) {
      listener = function (event) {
        assert.equal(event.data, location.href)
        done()
      }
      window.addEventListener('message', listener)
      w = window.open(url, '', 'show=no,nodeIntegration=yes')
    })
  })

  describe('window.postMessage', function () {
    it('sets the source and origin correctly', function (done) {
      var b, sourceId
      sourceId = remote.getCurrentWindow().id
      listener = function (event) {
        window.removeEventListener('message', listener)
        b.close()
        var message = JSON.parse(event.data)
        assert.equal(message.data, 'testing')
        assert.equal(message.origin, 'file://')
        assert.equal(message.sourceEqualsOpener, true)
        assert.equal(message.sourceId, sourceId)
        assert.equal(event.origin, 'file://')
        done()
      }
      window.addEventListener('message', listener)
      b = window.open('file://' + fixtures + '/pages/window-open-postMessage.html', '', 'show=no')
      webContents.fromId(b.guestId).once('did-finish-load', function () {
        b.postMessage('testing', '*')
      })
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

    it('SharedWorker can work', function (done) {
      var worker = new SharedWorker('../fixtures/workers/shared_worker.js')
      var message = 'ping'
      worker.port.onmessage = function (event) {
        assert.equal(event.data, message)
        done()
      }
      worker.port.postMessage(message)
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
})

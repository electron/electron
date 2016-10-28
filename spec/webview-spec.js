const assert = require('assert')
const path = require('path')
const http = require('http')
const url = require('url')
const {app, session, getGuestWebContents, ipcMain, BrowserWindow} = require('electron').remote

describe('<webview> tag', function () {
  this.timeout(20000)

  var fixtures = path.join(__dirname, 'fixtures')

  var webview = null
  let w = null

  beforeEach(function () {
    webview = new WebView()
  })

  afterEach(function () {
    if (!document.body.contains(webview)) {
      document.body.appendChild(webview)
    }
    webview.remove()
    if (w) {
      w.destroy()
      w = null
    }
  })

  it('works without script tag in page', function (done) {
    w = new BrowserWindow({show: false})
    ipcMain.once('pong', function () {
      done()
    })
    w.loadURL('file://' + fixtures + '/pages/webview-no-script.html')
  })

  it('is disabled when nodeIntegration is disabled', function (done) {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        nodeIntegration: false,
        preload: path.join(fixtures, 'module', 'preload-webview.js')
      }
    })
    ipcMain.once('webview', function (event, type) {
      if (type === 'undefined') {
        done()
      } else {
        done('WebView still exists')
      }
    })
    w.loadURL('file://' + fixtures + '/pages/webview-no-script.html')
  })

  describe('src attribute', function () {
    it('specifies the page to load', function (done) {
      webview.addEventListener('console-message', function (e) {
        assert.equal(e.message, 'a')
        done()
      })
      webview.src = 'file://' + fixtures + '/pages/a.html'
      document.body.appendChild(webview)
    })

    it('navigates to new page when changed', function (done) {
      var listener = function () {
        webview.src = 'file://' + fixtures + '/pages/b.html'
        webview.addEventListener('console-message', function (e) {
          assert.equal(e.message, 'b')
          done()
        })
        webview.removeEventListener('did-finish-load', listener)
      }
      webview.addEventListener('did-finish-load', listener)
      webview.src = 'file://' + fixtures + '/pages/a.html'
      document.body.appendChild(webview)
    })

    it('resolves relative URLs', function (done) {
      var listener = function (e) {
        assert.equal(e.message, 'Window script is loaded before preload script')
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.src = '../fixtures/pages/e.html'
      document.body.appendChild(webview)
    })

    it('ignores empty values', function () {
      assert.equal(webview.src, '')
      webview.src = ''
      assert.equal(webview.src, '')
      webview.src = null
      assert.equal(webview.src, '')
      webview.src = undefined
      assert.equal(webview.src, '')
    })
  })

  describe('nodeintegration attribute', function () {
    it('inserts no node symbols when not set', function (done) {
      webview.addEventListener('console-message', function (e) {
        assert.equal(e.message, 'undefined undefined undefined undefined')
        done()
      })
      webview.src = 'file://' + fixtures + '/pages/c.html'
      document.body.appendChild(webview)
    })

    it('inserts node symbols when set', function (done) {
      webview.addEventListener('console-message', function (e) {
        assert.equal(e.message, 'function object object')
        done()
      })
      webview.setAttribute('nodeintegration', 'on')
      webview.src = 'file://' + fixtures + '/pages/d.html'
      document.body.appendChild(webview)
    })

    it('loads node symbols after POST navigation when set', function (done) {
      webview.addEventListener('console-message', function (e) {
        assert.equal(e.message, 'function object object')
        done()
      })
      webview.setAttribute('nodeintegration', 'on')
      webview.src = 'file://' + fixtures + '/pages/post.html'
      document.body.appendChild(webview)
    })

    it('disables node integration on child windows when it is disabled on the webview', function (done) {
      app.once('browser-window-created', function (event, window) {
        assert.equal(window.webContents.getWebPreferences().nodeIntegration, false)
        done()
      })

      webview.setAttribute('allowpopups', 'on')

      webview.src = url.format({
        pathname: `${fixtures}/pages/webview-opener-no-node-integration.html`,
        protocol: 'file',
        query: {
          p: `${fixtures}/pages/window-opener-node.html`
        },
        slashes: true
      })
      document.body.appendChild(webview)
    })

    if (process.platform !== 'win32' || process.execPath.toLowerCase().indexOf('\\out\\d\\') === -1) {
      it('loads native modules when navigation happens', function (done) {
        var listener = function () {
          webview.removeEventListener('did-finish-load', listener)
          var listener2 = function (e) {
            assert.equal(e.message, 'function')
            done()
          }
          webview.addEventListener('console-message', listener2)
          webview.reload()
        }
        webview.addEventListener('did-finish-load', listener)
        webview.setAttribute('nodeintegration', 'on')
        webview.src = 'file://' + fixtures + '/pages/native-module.html'
        document.body.appendChild(webview)
      })
    }
  })

  describe('preload attribute', function () {
    it('loads the script before other scripts in window', function (done) {
      var listener = function (e) {
        assert.equal(e.message, 'function object object')
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.setAttribute('preload', fixtures + '/module/preload.js')
      webview.src = 'file://' + fixtures + '/pages/e.html'
      document.body.appendChild(webview)
    })

    it('preload script can still use "process" in required modules when nodeintegration is off', function (done) {
      webview.addEventListener('console-message', function (e) {
        assert.equal(e.message, 'object undefined object')
        done()
      })
      webview.setAttribute('preload', fixtures + '/module/preload-node-off.js')
      webview.src = 'file://' + fixtures + '/api/blank.html'
      document.body.appendChild(webview)
    })

    it('receives ipc message in preload script', function (done) {
      var message = 'boom!'
      var listener = function (e) {
        assert.equal(e.channel, 'pong')
        assert.deepEqual(e.args, [message])
        webview.removeEventListener('ipc-message', listener)
        done()
      }
      var listener2 = function () {
        webview.send('ping', message)
        webview.removeEventListener('did-finish-load', listener2)
      }
      webview.addEventListener('ipc-message', listener)
      webview.addEventListener('did-finish-load', listener2)
      webview.setAttribute('preload', fixtures + '/module/preload-ipc.js')
      webview.src = 'file://' + fixtures + '/pages/e.html'
      document.body.appendChild(webview)
    })

    it('works without script tag in page', function (done) {
      var listener = function (e) {
        assert.equal(e.message, 'function object object')
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.setAttribute('preload', fixtures + '/module/preload.js')
      webview.src = 'file://' + fixtures + '/pages/base-page.html'
      document.body.appendChild(webview)
    })

    it('resolves relative URLs', function (done) {
      var listener = function (e) {
        assert.equal(e.message, 'function object object')
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.src = 'file://' + fixtures + '/pages/e.html'
      webview.preload = '../fixtures/module/preload.js'
      document.body.appendChild(webview)
    })

    it('ignores empty values', function () {
      assert.equal(webview.preload, '')
      webview.preload = ''
      assert.equal(webview.preload, '')
      webview.preload = null
      assert.equal(webview.preload, '')
      webview.preload = undefined
      assert.equal(webview.preload, '')
    })
  })

  describe('httpreferrer attribute', function () {
    it('sets the referrer url', function (done) {
      var referrer = 'http://github.com/'
      var listener = function (e) {
        assert.equal(e.message, referrer)
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.setAttribute('httpreferrer', referrer)
      webview.src = 'file://' + fixtures + '/pages/referrer.html'
      document.body.appendChild(webview)
    })
  })

  describe('useragent attribute', function () {
    it('sets the user agent', function (done) {
      var referrer = 'Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; AS; rv:11.0) like Gecko'
      var listener = function (e) {
        assert.equal(e.message, referrer)
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.setAttribute('useragent', referrer)
      webview.src = 'file://' + fixtures + '/pages/useragent.html'
      document.body.appendChild(webview)
    })
  })

  describe('disablewebsecurity attribute', function () {
    it('does not disable web security when not set', function (done) {
      var jqueryPath = path.join(__dirname, '/static/jquery-2.0.3.min.js')
      var src = `<script src='file://${jqueryPath}'></script> <script>console.log('ok');</script>`
      var encoded = btoa(unescape(encodeURIComponent(src)))
      var listener = function (e) {
        assert(/Not allowed to load local resource/.test(e.message))
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.src = 'data:text/html;base64,' + encoded
      document.body.appendChild(webview)
    })

    it('disables web security when set', function (done) {
      var jqueryPath = path.join(__dirname, '/static/jquery-2.0.3.min.js')
      var src = `<script src='file://${jqueryPath}'></script> <script>console.log('ok');</script>`
      var encoded = btoa(unescape(encodeURIComponent(src)))
      var listener = function (e) {
        assert.equal(e.message, 'ok')
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.setAttribute('disablewebsecurity', '')
      webview.src = 'data:text/html;base64,' + encoded
      document.body.appendChild(webview)
    })

    it('does not break node integration', function (done) {
      webview.addEventListener('console-message', function (e) {
        assert.equal(e.message, 'function object object')
        done()
      })
      webview.setAttribute('nodeintegration', 'on')
      webview.setAttribute('disablewebsecurity', '')
      webview.src = 'file://' + fixtures + '/pages/d.html'
      document.body.appendChild(webview)
    })

    it('does not break preload script', function (done) {
      var listener = function (e) {
        assert.equal(e.message, 'function object object')
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.setAttribute('disablewebsecurity', '')
      webview.setAttribute('preload', fixtures + '/module/preload.js')
      webview.src = 'file://' + fixtures + '/pages/e.html'
      document.body.appendChild(webview)
    })
  })

  describe('partition attribute', function () {
    it('inserts no node symbols when not set', function (done) {
      webview.addEventListener('console-message', function (e) {
        assert.equal(e.message, 'undefined undefined undefined undefined')
        done()
      })
      webview.src = 'file://' + fixtures + '/pages/c.html'
      webview.partition = 'test1'
      document.body.appendChild(webview)
    })

    it('inserts node symbols when set', function (done) {
      webview.addEventListener('console-message', function (e) {
        assert.equal(e.message, 'function object object')
        done()
      })
      webview.setAttribute('nodeintegration', 'on')
      webview.src = 'file://' + fixtures + '/pages/d.html'
      webview.partition = 'test2'
      document.body.appendChild(webview)
    })

    it('isolates storage for different id', function (done) {
      var listener = function (e) {
        assert.equal(e.message, ' 0')
        webview.removeEventListener('console-message', listener)
        done()
      }
      window.localStorage.setItem('test', 'one')
      webview.addEventListener('console-message', listener)
      webview.src = 'file://' + fixtures + '/pages/partition/one.html'
      webview.partition = 'test3'
      document.body.appendChild(webview)
    })

    it('uses current session storage when no id is provided', function (done) {
      var listener = function (e) {
        assert.equal(e.message, 'one 1')
        webview.removeEventListener('console-message', listener)
        done()
      }
      window.localStorage.setItem('test', 'one')
      webview.addEventListener('console-message', listener)
      webview.src = 'file://' + fixtures + '/pages/partition/one.html'
      document.body.appendChild(webview)
    })
  })

  describe('allowpopups attribute', function () {
    if (process.env.TRAVIS === 'true' && process.platform === 'darwin') {
      return
    }

    it('can not open new window when not set', function (done) {
      var listener = function (e) {
        assert.equal(e.message, 'null')
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.src = 'file://' + fixtures + '/pages/window-open-hide.html'
      document.body.appendChild(webview)
    })

    it('can open new window when set', function (done) {
      var listener = function (e) {
        assert.equal(e.message, 'window')
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.setAttribute('allowpopups', 'on')
      webview.src = 'file://' + fixtures + '/pages/window-open-hide.html'
      document.body.appendChild(webview)
    })
  })

  describe('webpreferences attribute', function () {
    it('can enable nodeintegration', function (done) {
      webview.addEventListener('console-message', function (e) {
        assert.equal(e.message, 'function object object')
        done()
      })
      webview.setAttribute('webpreferences', 'nodeIntegration')
      webview.src = 'file://' + fixtures + '/pages/d.html'
      document.body.appendChild(webview)
    })

    it('can disables web security and enable nodeintegration', function (done) {
      var jqueryPath = path.join(__dirname, '/static/jquery-2.0.3.min.js')
      var src = `<script src='file://${jqueryPath}'></script> <script>console.log('ok '+(typeof require));</script>`
      var encoded = btoa(unescape(encodeURIComponent(src)))
      var listener = function (e) {
        assert.equal(e.message, 'ok function')
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.setAttribute('webpreferences', 'webSecurity=no, nodeIntegration=yes')
      webview.src = 'data:text/html;base64,' + encoded
      document.body.appendChild(webview)
    })
  })

  describe('new-window event', function () {
    if (process.env.TRAVIS === 'true' && process.platform === 'darwin') {
      return
    }

    it('emits when window.open is called', function (done) {
      webview.addEventListener('new-window', function (e) {
        assert.equal(e.url, 'http://host/')
        assert.equal(e.frameName, 'host')
        done()
      })
      webview.src = 'file://' + fixtures + '/pages/window-open.html'
      document.body.appendChild(webview)
    })

    it('emits when link with target is called', function (done) {
      webview.addEventListener('new-window', function (e) {
        assert.equal(e.url, 'http://host/')
        assert.equal(e.frameName, 'target')
        done()
      })
      webview.src = 'file://' + fixtures + '/pages/target-name.html'
      document.body.appendChild(webview)
    })
  })

  describe('ipc-message event', function () {
    it('emits when guest sends a ipc message to browser', function (done) {
      webview.addEventListener('ipc-message', function (e) {
        assert.equal(e.channel, 'channel')
        assert.deepEqual(e.args, ['arg1', 'arg2'])
        done()
      })
      webview.src = 'file://' + fixtures + '/pages/ipc-message.html'
      webview.setAttribute('nodeintegration', 'on')
      document.body.appendChild(webview)
    })
  })

  describe('page-title-set event', function () {
    it('emits when title is set', function (done) {
      webview.addEventListener('page-title-set', function (e) {
        assert.equal(e.title, 'test')
        assert(e.explicitSet)
        done()
      })
      webview.src = 'file://' + fixtures + '/pages/a.html'
      document.body.appendChild(webview)
    })
  })

  describe('page-favicon-updated event', function () {
    it('emits when favicon urls are received', function (done) {
      webview.addEventListener('page-favicon-updated', function (e) {
        assert.equal(e.favicons.length, 2)
        var pageUrl = process.platform === 'win32' ? 'file:///C:/favicon.png' : 'file:///favicon.png'
        assert.equal(e.favicons[0], pageUrl)
        done()
      })
      webview.src = 'file://' + fixtures + '/pages/a.html'
      document.body.appendChild(webview)
    })
  })

  describe('will-navigate event', function () {
    it('emits when a url that leads to oustide of the page is clicked', function (done) {
      webview.addEventListener('will-navigate', function (e) {
        assert.equal(e.url, 'http://host/')
        done()
      })
      webview.src = 'file://' + fixtures + '/pages/webview-will-navigate.html'
      document.body.appendChild(webview)
    })
  })

  describe('did-navigate event', function () {
    var p = path.join(fixtures, 'pages', 'webview-will-navigate.html')
    p = p.replace(/\\/g, '/')
    var pageUrl = url.format({
      protocol: 'file',
      slashes: true,
      pathname: p
    })

    it('emits when a url that leads to outside of the page is clicked', function (done) {
      webview.addEventListener('did-navigate', function (e) {
        assert.equal(e.url, pageUrl)
        done()
      })
      webview.src = pageUrl
      document.body.appendChild(webview)
    })
  })

  describe('did-navigate-in-page event', function () {
    it('emits when an anchor link is clicked', function (done) {
      var p = path.join(fixtures, 'pages', 'webview-did-navigate-in-page.html')
      p = p.replace(/\\/g, '/')
      var pageUrl = url.format({
        protocol: 'file',
        slashes: true,
        pathname: p
      })
      webview.addEventListener('did-navigate-in-page', function (e) {
        assert.equal(e.url, pageUrl + '#test_content')
        done()
      })
      webview.src = pageUrl
      document.body.appendChild(webview)
    })

    it('emits when window.history.replaceState is called', function (done) {
      webview.addEventListener('did-navigate-in-page', function (e) {
        assert.equal(e.url, 'http://host/')
        done()
      })
      webview.src = 'file://' + fixtures + '/pages/webview-did-navigate-in-page-with-history.html'
      document.body.appendChild(webview)
    })

    it('emits when window.location.hash is changed', function (done) {
      var p = path.join(fixtures, 'pages', 'webview-did-navigate-in-page-with-hash.html')
      p = p.replace(/\\/g, '/')
      var pageUrl = url.format({
        protocol: 'file',
        slashes: true,
        pathname: p
      })
      webview.addEventListener('did-navigate-in-page', function (e) {
        assert.equal(e.url, pageUrl + '#test')
        done()
      })
      webview.src = pageUrl
      document.body.appendChild(webview)
    })
  })

  describe('close event', function () {
    it('should fire when interior page calls window.close', function (done) {
      webview.addEventListener('close', function () {
        done()
      })
      webview.src = 'file://' + fixtures + '/pages/close.html'
      document.body.appendChild(webview)
    })
  })

  describe('devtools-opened event', function () {
    it('should fire when webview.openDevTools() is called', function (done) {
      var listener = function () {
        webview.removeEventListener('devtools-opened', listener)
        webview.closeDevTools()
        done()
      }
      webview.addEventListener('devtools-opened', listener)
      webview.addEventListener('dom-ready', function () {
        webview.openDevTools()
      })
      webview.src = 'file://' + fixtures + '/pages/base-page.html'
      document.body.appendChild(webview)
    })
  })

  describe('devtools-closed event', function () {
    it('should fire when webview.closeDevTools() is called', function (done) {
      var listener2 = function () {
        webview.removeEventListener('devtools-closed', listener2)
        done()
      }
      var listener = function () {
        webview.removeEventListener('devtools-opened', listener)
        webview.closeDevTools()
      }
      webview.addEventListener('devtools-opened', listener)
      webview.addEventListener('devtools-closed', listener2)
      webview.addEventListener('dom-ready', function () {
        webview.openDevTools()
      })
      webview.src = 'file://' + fixtures + '/pages/base-page.html'
      document.body.appendChild(webview)
    })
  })

  describe('devtools-focused event', function () {
    it('should fire when webview.openDevTools() is called', function (done) {
      var listener = function () {
        webview.removeEventListener('devtools-focused', listener)
        webview.closeDevTools()
        done()
      }
      webview.addEventListener('devtools-focused', listener)
      webview.addEventListener('dom-ready', function () {
        webview.openDevTools()
      })
      webview.src = 'file://' + fixtures + '/pages/base-page.html'
      document.body.appendChild(webview)
    })
  })

  describe('<webview>.reload()', function () {
    it('should emit beforeunload handler', function (done) {
      var listener = function (e) {
        assert.equal(e.channel, 'onbeforeunload')
        webview.removeEventListener('ipc-message', listener)
        done()
      }
      var listener2 = function () {
        webview.reload()
        webview.removeEventListener('did-finish-load', listener2)
      }
      webview.addEventListener('ipc-message', listener)
      webview.addEventListener('did-finish-load', listener2)
      webview.setAttribute('nodeintegration', 'on')
      webview.src = 'file://' + fixtures + '/pages/beforeunload-false.html'
      document.body.appendChild(webview)
    })
  })

  describe('<webview>.goForward()', function () {
    it('should work after a replaced history entry', function (done) {
      var loadCount = 1
      var listener = function (e) {
        if (loadCount === 1) {
          assert.equal(e.channel, 'history')
          assert.equal(e.args[0], 1)
          assert(!webview.canGoBack())
          assert(!webview.canGoForward())
        } else if (loadCount === 2) {
          assert.equal(e.channel, 'history')
          assert.equal(e.args[0], 2)
          assert(!webview.canGoBack())
          assert(webview.canGoForward())
          webview.removeEventListener('ipc-message', listener)
        }
      }

      var loadListener = function (e) {
        if (loadCount === 1) {
          webview.src = 'file://' + fixtures + '/pages/base-page.html'
        } else if (loadCount === 2) {
          assert(webview.canGoBack())
          assert(!webview.canGoForward())

          webview.goBack()
        } else if (loadCount === 3) {
          webview.goForward()
        } else if (loadCount === 4) {
          assert(webview.canGoBack())
          assert(!webview.canGoForward())

          webview.removeEventListener('did-finish-load', loadListener)
          done()
        }

        loadCount++
      }

      webview.addEventListener('ipc-message', listener)
      webview.addEventListener('did-finish-load', loadListener)
      webview.setAttribute('nodeintegration', 'on')
      webview.src = 'file://' + fixtures + '/pages/history-replace.html'
      document.body.appendChild(webview)
    })
  })

  describe('<webview>.clearHistory()', function () {
    it('should clear the navigation history', function (done) {
      var listener = function (e) {
        assert.equal(e.channel, 'history')
        assert.equal(e.args[0], 2)
        assert(webview.canGoBack())
        webview.clearHistory()
        assert(!webview.canGoBack())
        webview.removeEventListener('ipc-message', listener)
        done()
      }
      webview.addEventListener('ipc-message', listener)
      webview.setAttribute('nodeintegration', 'on')
      webview.src = 'file://' + fixtures + '/pages/history.html'
      document.body.appendChild(webview)
    })
  })

  describe('basic auth', function () {
    var auth = require('basic-auth')

    it('should authenticate with correct credentials', function (done) {
      var message = 'Authenticated'
      var server = http.createServer(function (req, res) {
        var credentials = auth(req)
        if (credentials.name === 'test' && credentials.pass === 'test') {
          res.end(message)
        } else {
          res.end('failed')
        }
        server.close()
      })
      server.listen(0, '127.0.0.1', function () {
        var port = server.address().port
        webview.addEventListener('ipc-message', function (e) {
          assert.equal(e.channel, message)
          done()
        })
        webview.src = 'file://' + fixtures + '/pages/basic-auth.html?port=' + port
        webview.setAttribute('nodeintegration', 'on')
        document.body.appendChild(webview)
      })
    })
  })

  describe('dom-ready event', function () {
    it('emits when document is loaded', function (done) {
      var server = http.createServer(function () {})
      server.listen(0, '127.0.0.1', function () {
        var port = server.address().port
        webview.addEventListener('dom-ready', function () {
          done()
        })
        webview.src = 'file://' + fixtures + '/pages/dom-ready.html?port=' + port
        document.body.appendChild(webview)
      })
    })

    it('throws a custom error when an API method is called before the event is emitted', function () {
      assert.throws(function () {
        webview.stop()
      }, 'Cannot call stop because the webContents is unavailable. The WebView must be attached to the DOM and the dom-ready event emitted before this method can be called.')
    })
  })

  describe('executeJavaScript', function () {
    it('should support user gesture', function (done) {
      if (process.env.TRAVIS !== 'true' || process.platform === 'darwin') return done()

      var listener = function () {
        webview.removeEventListener('enter-html-full-screen', listener)
        done()
      }
      var listener2 = function () {
        var jsScript = "document.querySelector('video').webkitRequestFullscreen()"
        webview.executeJavaScript(jsScript, true)
        webview.removeEventListener('did-finish-load', listener2)
      }
      webview.addEventListener('enter-html-full-screen', listener)
      webview.addEventListener('did-finish-load', listener2)
      webview.src = 'file://' + fixtures + '/pages/fullscreen.html'
      document.body.appendChild(webview)
    })

    it('can return the result of the executed script', function (done) {
      if (process.env.TRAVIS === 'true' && process.platform === 'darwin') return done()

      var listener = function () {
        var jsScript = "'4'+2"
        webview.executeJavaScript(jsScript, false, function (result) {
          assert.equal(result, '42')
          done()
        })
        webview.removeEventListener('did-finish-load', listener)
      }
      webview.addEventListener('did-finish-load', listener)
      webview.src = 'about:blank'
      document.body.appendChild(webview)
    })
  })

  describe('sendInputEvent', function () {
    it('can send keyboard event', function (done) {
      webview.addEventListener('ipc-message', function (e) {
        assert.equal(e.channel, 'keyup')
        assert.deepEqual(e.args, ['C', 'KeyC', 67, true, false])
        done()
      })
      webview.addEventListener('dom-ready', function () {
        webview.sendInputEvent({
          type: 'keyup',
          keyCode: 'c',
          modifiers: ['shift']
        })
      })
      webview.src = 'file://' + fixtures + '/pages/onkeyup.html'
      webview.setAttribute('nodeintegration', 'on')
      document.body.appendChild(webview)
    })

    it('can send mouse event', function (done) {
      webview.addEventListener('ipc-message', function (e) {
        assert.equal(e.channel, 'mouseup')
        assert.deepEqual(e.args, [10, 20, false, true])
        done()
      })
      webview.addEventListener('dom-ready', function () {
        webview.sendInputEvent({
          type: 'mouseup',
          modifiers: ['ctrl'],
          x: 10,
          y: 20
        })
      })
      webview.src = 'file://' + fixtures + '/pages/onmouseup.html'
      webview.setAttribute('nodeintegration', 'on')
      document.body.appendChild(webview)
    })
  })

  describe('media-started-playing media-paused events', function () {
    it('emits when audio starts and stops playing', function (done) {
      var audioPlayed = false
      webview.addEventListener('media-started-playing', function () {
        audioPlayed = true
      })
      webview.addEventListener('media-paused', function () {
        assert(audioPlayed)
        done()
      })
      webview.src = 'file://' + fixtures + '/pages/audio.html'
      document.body.appendChild(webview)
    })
  })

  describe('found-in-page event', function () {
    it('emits when a request is made', function (done) {
      let requestId = null
      let activeMatchOrdinal = []
      const listener = function (e) {
        assert.equal(e.result.requestId, requestId)
        assert.equal(e.result.matches, 3)
        activeMatchOrdinal.push(e.result.activeMatchOrdinal)
        if (e.result.activeMatchOrdinal === e.result.matches) {
          assert.deepEqual(activeMatchOrdinal, [1, 2, 3])
          webview.stopFindInPage('clearSelection')
          done()
        } else {
          listener2()
        }
      }
      const listener2 = function () {
        requestId = webview.findInPage('virtual')
      }
      webview.addEventListener('found-in-page', listener)
      webview.addEventListener('did-finish-load', listener2)
      webview.src = 'file://' + fixtures + '/pages/content.html'
      document.body.appendChild(webview)
    })
  })

  xdescribe('did-change-theme-color event', function () {
    it('emits when theme color changes', function (done) {
      webview.addEventListener('did-change-theme-color', function () {
        done()
      })
      webview.src = 'file://' + fixtures + '/pages/theme-color.html'
      document.body.appendChild(webview)
    })
  })

  describe('permission-request event', function () {
    function setUpRequestHandler (webview, requestedPermission, completed) {
      var listener = function (webContents, permission, callback) {
        if (webContents.getId() === webview.getId()) {
          assert.equal(permission, requestedPermission)
          callback(false)
          if (completed) completed()
        }
      }
      session.fromPartition(webview.partition).setPermissionRequestHandler(listener)
    }

    it('emits when using navigator.getUserMedia api', function (done) {
      webview.addEventListener('ipc-message', function (e) {
        assert.equal(e.channel, 'message')
        assert.deepEqual(e.args, ['PermissionDeniedError'])
        done()
      })
      webview.src = 'file://' + fixtures + '/pages/permissions/media.html'
      webview.partition = 'permissionTest'
      webview.setAttribute('nodeintegration', 'on')
      setUpRequestHandler(webview, 'media')
      document.body.appendChild(webview)
    })

    it('emits when using navigator.geolocation api', function (done) {
      webview.addEventListener('ipc-message', function (e) {
        assert.equal(e.channel, 'message')
        assert.deepEqual(e.args, ['User denied Geolocation'])
        done()
      })
      webview.src = 'file://' + fixtures + '/pages/permissions/geolocation.html'
      webview.partition = 'permissionTest'
      webview.setAttribute('nodeintegration', 'on')
      setUpRequestHandler(webview, 'geolocation')
      document.body.appendChild(webview)
    })

    it('emits when using navigator.requestMIDIAccess api', function (done) {
      webview.addEventListener('ipc-message', function (e) {
        assert.equal(e.channel, 'message')
        assert.deepEqual(e.args, ['SecurityError'])
        done()
      })
      webview.src = 'file://' + fixtures + '/pages/permissions/midi.html'
      webview.partition = 'permissionTest'
      webview.setAttribute('nodeintegration', 'on')
      setUpRequestHandler(webview, 'midiSysex')
      document.body.appendChild(webview)
    })

    it('emits when accessing external protocol', function (done) {
      webview.src = 'magnet:test'
      webview.partition = 'permissionTest'
      setUpRequestHandler(webview, 'openExternal', done)
      document.body.appendChild(webview)
    })

    it('emits when using Notification.requestPermission', function (done) {
      webview.addEventListener('ipc-message', function (e) {
        assert.equal(e.channel, 'message')
        assert.deepEqual(e.args, ['granted'])
        done()
      })
      webview.src = 'file://' + fixtures + '/pages/permissions/notification.html'
      webview.partition = 'permissionTest'
      webview.setAttribute('nodeintegration', 'on')
      session.fromPartition(webview.partition).setPermissionRequestHandler(function (webContents, permission, callback) {
        if (webContents.getId() === webview.getId()) {
          assert.equal(permission, 'notifications')
          callback(true)
        }
      })
      document.body.appendChild(webview)
    })
  })

  describe('<webview>.getWebContents', function () {
    it('can return the webcontents associated', function (done) {
      webview.addEventListener('did-finish-load', function () {
        const webviewContents = webview.getWebContents()
        assert(webviewContents)
        assert.equal(webviewContents.getURL(), 'about:blank')
        done()
      })
      webview.src = 'about:blank'
      document.body.appendChild(webview)
    })
  })

  describe('did-get-response-details event', function () {
    it('emits for the page and its resources', function (done) {
      // expected {fileName: resourceType} pairs
      var expectedResources = {
        'did-get-response-details.html': 'mainFrame',
        'logo.png': 'image'
      }
      var responses = 0
      webview.addEventListener('did-get-response-details', function (event) {
        responses++
        var fileName = event.newURL.slice(event.newURL.lastIndexOf('/') + 1)
        var expectedType = expectedResources[fileName]
        assert(!!expectedType, `Unexpected response details for ${event.newURL}`)
        assert(typeof event.status === 'boolean', 'status should be boolean')
        assert.equal(event.httpResponseCode, 200)
        assert.equal(event.requestMethod, 'GET')
        assert(typeof event.referrer === 'string', 'referrer should be string')
        assert(!!event.headers, 'headers should be present')
        assert(typeof event.headers === 'object', 'headers should be object')
        assert.equal(event.resourceType, expectedType, 'Incorrect resourceType')
        if (responses === Object.keys(expectedResources).length) {
          done()
        }
      })
      webview.src = 'file://' + path.join(fixtures, 'pages', 'did-get-response-details.html')
      document.body.appendChild(webview)
    })
  })

  it('inherits the zoomFactor of the parent window', function (done) {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        zoomFactor: 1.2
      }
    })
    ipcMain.once('pong', function (event, zoomFactor, zoomLevel) {
      assert.equal(zoomFactor, 1.2)
      assert.equal(zoomLevel, 1)
      done()
    })
    w.loadURL('file://' + fixtures + '/pages/webview-zoom-factor.html')
  })

  it('inherits the parent window visibility state and receives visibilitychange events', function (done) {
    w = new BrowserWindow({
      show: false
    })

    ipcMain.once('pong', function (event, visibilityState, hidden) {
      assert.equal(visibilityState, 'hidden')
      assert.equal(hidden, true)

      w.webContents.send('ELECTRON_RENDERER_WINDOW_VISIBILITY_CHANGE', 'visible')

      ipcMain.once('pong', function (event, visibilityState, hidden) {
        assert.equal(visibilityState, 'visible')
        assert.equal(hidden, false)
        done()
      })
    })

    w.loadURL('file://' + fixtures + '/pages/webview-visibilitychange.html')
  })

  it('loads devtools extensions registered on the parent window', function (done) {
    this.timeout(10000)

    w = new BrowserWindow({
      show: false
    })

    BrowserWindow.removeDevToolsExtension('foo')

    var extensionPath = path.join(__dirname, 'fixtures', 'devtools-extensions', 'foo')
    BrowserWindow.addDevToolsExtension(extensionPath)

    w.loadURL('file://' + fixtures + '/pages/webview-devtools.html')

    ipcMain.once('answer', function (event, message) {
      assert.equal(message.runtimeId, 'foo')
      assert.notEqual(message.tabId, w.webContents.id)
      done()
    })
  })

  describe('guestinstance attribute', function () {
    it('before loading there is no attribute', function () {
      document.body.appendChild(webview)
      assert(!webview.hasAttribute('guestinstance'))
    })

    it('loading a page sets the guest view', function (done) {
      var loadListener = function () {
        webview.removeEventListener('did-finish-load', loadListener, false)
        var instance = webview.getAttribute('guestinstance')
        assert.equal(instance, parseInt(instance))

        var guest = getGuestWebContents(parseInt(instance))
        assert.equal(guest, webview.getWebContents())
        done()
      }
      webview.addEventListener('did-finish-load', loadListener, false)
      webview.src = 'file://' + fixtures + '/api/blank.html'
      document.body.appendChild(webview)
    })

    it('deleting the attribute destroys the webview', function (done) {
      var loadListener = function () {
        webview.removeEventListener('did-finish-load', loadListener, false)
        var destroyListener = function () {
          webview.removeEventListener('destroyed', destroyListener, false)
          assert.equal(getGuestWebContents(instance), null)
          done()
        }
        webview.addEventListener('destroyed', destroyListener, false)

        var instance = parseInt(webview.getAttribute('guestinstance'))
        webview.removeAttribute('guestinstance')
      }
      webview.addEventListener('did-finish-load', loadListener, false)
      webview.src = 'file://' + fixtures + '/api/blank.html'
      document.body.appendChild(webview)
    })

    it('setting the attribute on a new webview moves the contents', function (done) {
      var loadListener = function () {
        webview.removeEventListener('did-finish-load', loadListener, false)
        var webContents = webview.getWebContents()
        var instance = webview.getAttribute('guestinstance')

        var destroyListener = function () {
          webview.removeEventListener('destroyed', destroyListener, false)
          assert.equal(webContents, webview2.getWebContents())
          // Make sure that events are hooked up to the right webview now
          webview2.addEventListener('console-message', function (e) {
            assert.equal(e.message, 'a')
            document.body.removeChild(webview2)
            done()
          })

          webview2.src = 'file://' + fixtures + '/pages/a.html'
        }
        webview.addEventListener('destroyed', destroyListener, false)

        var webview2 = new WebView()
        webview2.setAttribute('guestinstance', instance)
        document.body.appendChild(webview2)
      }
      webview.addEventListener('did-finish-load', loadListener, false)
      webview.src = 'file://' + fixtures + '/api/blank.html'
      document.body.appendChild(webview)
    })

    it('setting the attribute to an invalid guestinstance does nothing', function (done) {
      var loadListener = function () {
        webview.removeEventListener('did-finish-load', loadListener, false)
        webview.setAttribute('guestinstance', 55)

        // Make sure that events are still hooked up to the webview
        webview.addEventListener('console-message', function (e) {
          assert.equal(e.message, 'a')
          done()
        })

        webview.src = 'file://' + fixtures + '/pages/a.html'
      }
      webview.addEventListener('did-finish-load', loadListener, false)

      webview.src = 'file://' + fixtures + '/api/blank.html'
      document.body.appendChild(webview)
    })

    it('setting the attribute on an existing webview moves the contents', function (done) {
      var load1Listener = function () {
        webview.removeEventListener('did-finish-load', load1Listener, false)
        var webContents = webview.getWebContents()
        var instance = webview.getAttribute('guestinstance')
        var destroyedInstance

        var destroyListener = function () {
          webview.removeEventListener('destroyed', destroyListener, false)
          assert.equal(webContents, webview2.getWebContents())
          assert.equal(null, getGuestWebContents(parseInt(destroyedInstance)))

          // Make sure that events are hooked up to the right webview now
          webview2.addEventListener('console-message', function (e) {
            assert.equal(e.message, 'a')
            document.body.removeChild(webview2)
            done()
          })

          webview2.src = 'file://' + fixtures + '/pages/a.html'
        }
        webview.addEventListener('destroyed', destroyListener, false)

        var webview2 = new WebView()
        var load2Listener = function () {
          webview2.removeEventListener('did-finish-load', load2Listener, false)
          destroyedInstance = webview2.getAttribute('guestinstance')
          assert.notEqual(instance, destroyedInstance)

          webview2.setAttribute('guestinstance', instance)
        }
        webview2.addEventListener('did-finish-load', load2Listener, false)
        webview2.src = 'file://' + fixtures + '/api/blank.html'
        document.body.appendChild(webview2)
      }
      webview.addEventListener('did-finish-load', load1Listener, false)
      webview.src = 'file://' + fixtures + '/api/blank.html'
      document.body.appendChild(webview)
    })

    it('moving a guest back to its original webview should work', function (done) {
      var loadListener = function () {
        webview.removeEventListener('did-finish-load', loadListener, false)
        var webContents = webview.getWebContents()
        var instance = webview.getAttribute('guestinstance')

        var destroy1Listener = function () {
          webview.removeEventListener('destroyed', destroy1Listener, false)
          assert.equal(webContents, webview2.getWebContents())
          assert.equal(null, webview.getWebContents())

          var destroy2Listener = function () {
            webview2.removeEventListener('destroyed', destroy2Listener, false)
            assert.equal(webContents, webview.getWebContents())
            assert.equal(null, webview2.getWebContents())

            // Make sure that events are hooked up to the right webview now
            webview.addEventListener('console-message', function (e) {
              assert.equal(e.message, 'a')
              document.body.removeChild(webview2)
              done()
            })

            webview.src = 'file://' + fixtures + '/pages/a.html'
          }
          webview2.addEventListener('destroyed', destroy2Listener, false)

          webview.setAttribute('guestinstance', instance)
        }
        webview.addEventListener('destroyed', destroy1Listener, false)

        var webview2 = new WebView()
        webview2.setAttribute('guestinstance', instance)
        document.body.appendChild(webview2)
      }
      webview.addEventListener('did-finish-load', loadListener, false)
      webview.src = 'file://' + fixtures + '/api/blank.html'
      document.body.appendChild(webview)
    })

    it('setting the attribute on a webview in a different window moves the contents', function (done) {
      var loadListener = function () {
        webview.removeEventListener('did-finish-load', loadListener, false)
        var instance = webview.getAttribute('guestinstance')

        w = new BrowserWindow({ show: false })
        w.webContents.once('did-finish-load', function () {
          ipcMain.once('pong', function () {
            assert(!webview.hasAttribute('guestinstance'))

            done()
          })

          w.webContents.send('guestinstance', instance)
        })
        w.loadURL('file://' + fixtures + '/pages/webview-move-to-window.html')
      }
      webview.addEventListener('did-finish-load', loadListener, false)
      webview.src = 'file://' + fixtures + '/api/blank.html'
      document.body.appendChild(webview)
    })
  })
})

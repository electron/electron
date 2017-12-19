const assert = require('assert')
const path = require('path')
const http = require('http')
const url = require('url')
const {ipcRenderer, remote} = require('electron')
const {app, session, getGuestWebContents, ipcMain, BrowserWindow, webContents} = remote
const {closeWindow} = require('./window-helpers')

const isCI = remote.getGlobal('isCi')
const nativeModulesEnabled = remote.getGlobal('nativeModulesEnabled')

/* Most of the APIs here don't use standard callbacks */
/* eslint-disable standard/no-callback-literal */

describe('<webview> tag', function () {
  this.timeout(3 * 60 * 1000)

  const fixtures = path.join(__dirname, 'fixtures')
  let webview = null
  let w = null

  beforeEach(() => {
    webview = new WebView()
  })

  afterEach(() => {
    if (!document.body.contains(webview)) {
      document.body.appendChild(webview)
    }
    webview.remove()
    return closeWindow(w).then(() => { w = null })
  })

  it('works without script tag in page', (done) => {
    w = new BrowserWindow({show: false})
    ipcMain.once('pong', () => done())
    w.loadURL('file://' + fixtures + '/pages/webview-no-script.html')
  })

  it('is disabled when nodeIntegration is disabled', (done) => {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        nodeIntegration: false,
        preload: path.join(fixtures, 'module', 'preload-webview.js')
      }
    })
    ipcMain.once('webview', (event, type) => {
      if (type === 'undefined') {
        done()
      } else {
        done('WebView still exists')
      }
    })
    w.loadURL(`file://${fixtures}/pages/webview-no-script.html`)
  })

  it('is enabled when the webviewTag option is enabled and the nodeIntegration option is disabled', (done) => {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        nodeIntegration: false,
        preload: path.join(fixtures, 'module', 'preload-webview.js'),
        webviewTag: true
      }
    })
    ipcMain.once('webview', (event, type) => {
      if (type !== 'undefined') {
        done()
      } else {
        done('WebView is not created')
      }
    })
    w.loadURL(`file://${fixtures}/pages/webview-no-script.html`)
  })

  describe('src attribute', () => {
    it('specifies the page to load', (done) => {
      webview.addEventListener('console-message', (e) => {
        assert.equal(e.message, 'a')
        done()
      })
      webview.src = `file://${fixtures}/pages/a.html`
      document.body.appendChild(webview)
    })

    it('navigates to new page when changed', (done) => {
      const listener = () => {
        webview.src = `file://${fixtures}/pages/b.html`
        webview.addEventListener('console-message', (e) => {
          assert.equal(e.message, 'b')
          done()
        })
        webview.removeEventListener('did-finish-load', listener)
      }
      webview.addEventListener('did-finish-load', listener)
      webview.src = `file://${fixtures}/pages/a.html`
      document.body.appendChild(webview)
    })

    it('resolves relative URLs', (done) => {
      const listener = (e) => {
        assert.equal(e.message, 'Window script is loaded before preload script')
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.src = '../fixtures/pages/e.html'
      document.body.appendChild(webview)
    })

    it('ignores empty values', () => {
      assert.equal(webview.src, '')
      webview.src = ''
      assert.equal(webview.src, '')
      webview.src = null
      assert.equal(webview.src, '')
      webview.src = undefined
      assert.equal(webview.src, '')
    })
  })

  describe('nodeintegration attribute', () => {
    it('inserts no node symbols when not set', (done) => {
      webview.addEventListener('console-message', (e) => {
        assert.equal(e.message, 'undefined undefined undefined undefined')
        done()
      })
      webview.src = `file://${fixtures}/pages/c.html`
      document.body.appendChild(webview)
    })

    it('inserts node symbols when set', (done) => {
      webview.addEventListener('console-message', (e) => {
        assert.equal(e.message, 'function object object')
        done()
      })
      webview.setAttribute('nodeintegration', 'on')
      webview.src = `file://${fixtures}/pages/d.html`
      document.body.appendChild(webview)
    })

    it('loads node symbols after POST navigation when set', function (done) {
      // FIXME Figure out why this is timing out on AppVeyor
      if (process.env.APPVEYOR === 'True') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return done()
      }

      webview.addEventListener('console-message', (e) => {
        assert.equal(e.message, 'function object object')
        done()
      })
      webview.setAttribute('nodeintegration', 'on')
      webview.src = `file://${fixtures}/pages/post.html`
      document.body.appendChild(webview)
    })

    it('disables node integration on child windows when it is disabled on the webview', (done) => {
      app.once('browser-window-created', (event, window) => {
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

    it('loads native modules when navigation happens', (done) => {
      if (!nativeModulesEnabled) return done()

      const listener = () => {
        webview.removeEventListener('did-finish-load', listener)
        const listener2 = (e) => {
          assert.equal(e.message, 'function')
          done()
        }
        webview.addEventListener('console-message', listener2)
        webview.reload()
      }
      webview.addEventListener('did-finish-load', listener)
      webview.setAttribute('nodeintegration', 'on')
      webview.src = `file://${fixtures}/pages/native-module.html`
      document.body.appendChild(webview)
    })
  })

  describe('preload attribute', () => {
    it('loads the script before other scripts in window', (done) => {
      const listener = (e) => {
        assert.equal(e.message, 'function object object function')
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.setAttribute('preload', `${fixtures}/module/preload.js`)
      webview.src = `file://${fixtures}/pages/e.html`
      document.body.appendChild(webview)
    })

    it('preload script can still use "process" and "Buffer" when nodeintegration is off', (done) => {
      webview.addEventListener('console-message', (e) => {
        assert.equal(e.message, 'object undefined object function')
        done()
      })
      webview.setAttribute('preload', `${fixtures}/module/preload-node-off.js`)
      webview.src = `file://${fixtures}/api/blank.html`
      document.body.appendChild(webview)
    })

    it('preload script can require modules that still use "process" and "Buffer" when nodeintegration is off', (done) => {
      webview.addEventListener('console-message', (e) => {
        assert.equal(e.message, 'object undefined object function undefined')
        done()
      })
      webview.setAttribute('preload', `${fixtures}/module/preload-node-off-wrapper.js`)
      webview.src = `file://${fixtures}/api/blank.html`
      document.body.appendChild(webview)
    })

    it('receives ipc message in preload script', (done) => {
      const message = 'boom!'
      const listener = (e) => {
        assert.equal(e.channel, 'pong')
        assert.deepEqual(e.args, [message])
        webview.removeEventListener('ipc-message', listener)
        done()
      }
      const listener2 = () => {
        webview.send('ping', message)
        webview.removeEventListener('did-finish-load', listener2)
      }
      webview.addEventListener('ipc-message', listener)
      webview.addEventListener('did-finish-load', listener2)
      webview.setAttribute('preload', `${fixtures}/module/preload-ipc.js`)
      webview.src = `file://${fixtures}/pages/e.html`
      document.body.appendChild(webview)
    })

    it('works without script tag in page', (done) => {
      const listener = (e) => {
        assert.equal(e.message, 'function object object function')
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.setAttribute('preload', `${fixtures}/module/preload.js`)
      webview.src = `file://${fixtures}pages/base-page.html`
      document.body.appendChild(webview)
    })

    it('resolves relative URLs', (done) => {
      const listener = (e) => {
        assert.equal(e.message, 'function object object function')
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.src = `file://${fixtures}/pages/e.html`
      webview.preload = '../fixtures/module/preload.js'
      document.body.appendChild(webview)
    })

    it('ignores empty values', () => {
      assert.equal(webview.preload, '')
      webview.preload = ''
      assert.equal(webview.preload, '')
      webview.preload = null
      assert.equal(webview.preload, '')
      webview.preload = undefined
      assert.equal(webview.preload, '')
    })
  })

  describe('httpreferrer attribute', () => {
    it('sets the referrer url', (done) => {
      const referrer = 'http://github.com/'
      const listener = (e) => {
        assert.equal(e.message, referrer)
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.setAttribute('httpreferrer', referrer)
      webview.src = `file://${fixtures}/pages/referrer.html`
      document.body.appendChild(webview)
    })
  })

  describe('useragent attribute', () => {
    it('sets the user agent', (done) => {
      const referrer = 'Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; AS; rv:11.0) like Gecko'
      const listener = (e) => {
        assert.equal(e.message, referrer)
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.setAttribute('useragent', referrer)
      webview.src = `file://${fixtures}/pages/useragent.html`
      document.body.appendChild(webview)
    })
  })

  describe('disablewebsecurity attribute', () => {
    it('does not disable web security when not set', (done) => {
      const jqueryPath = path.join(__dirname, '/static/jquery-2.0.3.min.js')
      const src = `<script src='file://${jqueryPath}'></script> <script>console.log('ok');</script>`
      const encoded = btoa(unescape(encodeURIComponent(src)))
      const listener = (e) => {
        assert(/Not allowed to load local resource/.test(e.message))
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.src = `data:text/html;base64,${encoded}`
      document.body.appendChild(webview)
    })

    it('disables web security when set', (done) => {
      const jqueryPath = path.join(__dirname, '/static/jquery-2.0.3.min.js')
      const src = `<script src='file://${jqueryPath}'></script> <script>console.log('ok');</script>`
      const encoded = btoa(unescape(encodeURIComponent(src)))
      const listener = (e) => {
        assert.equal(e.message, 'ok')
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.setAttribute('disablewebsecurity', '')
      webview.src = `data:text/html;base64,${encoded}`
      document.body.appendChild(webview)
    })

    it('does not break node integration', (done) => {
      webview.addEventListener('console-message', (e) => {
        assert.equal(e.message, 'function object object')
        done()
      })
      webview.setAttribute('nodeintegration', 'on')
      webview.setAttribute('disablewebsecurity', '')
      webview.src = `file://${fixtures}/pages/d.html`
      document.body.appendChild(webview)
    })

    it('does not break preload script', (done) => {
      const listener = (e) => {
        assert.equal(e.message, 'function object object function')
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.setAttribute('disablewebsecurity', '')
      webview.setAttribute('preload', `${fixtures}/module/preload.js`)
      webview.src = `file://${fixtures}/pages/e.html`
      document.body.appendChild(webview)
    })
  })

  describe('partition attribute', () => {
    it('inserts no node symbols when not set', (done) => {
      webview.addEventListener('console-message', (e) => {
        assert.equal(e.message, 'undefined undefined undefined undefined')
        done()
      })
      webview.src = `file://${fixtures}/pages/c.html`
      webview.partition = 'test1'
      document.body.appendChild(webview)
    })

    it('inserts node symbols when set', (done) => {
      webview.addEventListener('console-message', (e) => {
        assert.equal(e.message, 'function object object')
        done()
      })
      webview.setAttribute('nodeintegration', 'on')
      webview.src = `file://${fixtures}/pages/d.html`
      webview.partition = 'test2'
      document.body.appendChild(webview)
    })

    it('isolates storage for different id', (done) => {
      const listener = (e) => {
        assert.equal(e.message, ' 0')
        webview.removeEventListener('console-message', listener)
        done()
      }
      window.localStorage.setItem('test', 'one')
      webview.addEventListener('console-message', listener)
      webview.src = `file://${fixtures}/pages/partition/one.html`
      webview.partition = 'test3'
      document.body.appendChild(webview)
    })

    it('uses current session storage when no id is provided', (done) => {
      const listener = (e) => {
        assert.equal(e.message, 'one 1')
        webview.removeEventListener('console-message', listener)
        done()
      }
      window.localStorage.setItem('test', 'one')
      webview.addEventListener('console-message', listener)
      webview.src = `file://${fixtures}/pages/partition/one.html`
      document.body.appendChild(webview)
    })
  })

  describe('allowpopups attribute', () => {
    it('can not open new window when not set', (done) => {
      const listener = (e) => {
        assert.equal(e.message, 'null')
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.src = `file://${fixtures}/pages/window-open-hide.html`
      document.body.appendChild(webview)
    })

    it('can open new window when set', (done) => {
      const listener = (e) => {
        assert.equal(e.message, 'window')
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.setAttribute('allowpopups', 'on')
      webview.src = `file://${fixtures}/pages/window-open-hide.html`
      document.body.appendChild(webview)
    })
  })

  describe('webpreferences attribute', () => {
    it('can enable nodeintegration', (done) => {
      webview.addEventListener('console-message', (e) => {
        assert.equal(e.message, 'function object object')
        done()
      })
      webview.setAttribute('webpreferences', 'nodeIntegration')
      webview.src = `file://${fixtures}/pages/d.html`
      document.body.appendChild(webview)
    })

    it('can disables web security and enable nodeintegration', (done) => {
      const jqueryPath = path.join(__dirname, '/static/jquery-2.0.3.min.js')
      const src = `<script src='file://${jqueryPath}'></script> <script>console.log('ok '+(typeof require));</script>`
      const encoded = btoa(unescape(encodeURIComponent(src)))
      const listener = (e) => {
        assert.equal(e.message, 'ok function')
        webview.removeEventListener('console-message', listener)
        done()
      }
      webview.addEventListener('console-message', listener)
      webview.setAttribute('webpreferences', 'webSecurity=no, nodeIntegration=yes')
      webview.src = `data:text/html;base64,${encoded}`
      document.body.appendChild(webview)
    })

    it('can enable context isolation', (done) => {
      ipcMain.once('isolated-world', (event, data) => {
        assert.deepEqual(data, {
          preloadContext: {
            preloadProperty: 'number',
            pageProperty: 'undefined',
            typeofRequire: 'function',
            typeofProcess: 'object',
            typeofArrayPush: 'function',
            typeofFunctionApply: 'function'
          },
          pageContext: {
            preloadProperty: 'undefined',
            pageProperty: 'string',
            typeofRequire: 'undefined',
            typeofProcess: 'undefined',
            typeofArrayPush: 'number',
            typeofFunctionApply: 'boolean',
            typeofPreloadExecuteJavaScriptProperty: 'number',
            typeofOpenedWindow: 'object'
          }
        })
        done()
      })

      webview.setAttribute('preload', path.join(fixtures, 'api', 'isolated-preload.js'))
      webview.setAttribute('allowpopups', 'yes')
      webview.setAttribute('webpreferences', 'contextIsolation=yes')
      webview.src = 'file://' + fixtures + '/api/isolated.html'
      document.body.appendChild(webview)
    })
  })

  describe('new-window event', () => {
    it('emits when window.open is called', (done) => {
      webview.addEventListener('new-window', (e) => {
        assert.equal(e.url, 'http://host/')
        assert.equal(e.frameName, 'host')
        done()
      })
      webview.src = `file://${fixtures}/pages/window-open.html`
      document.body.appendChild(webview)
    })

    it('emits when link with target is called', (done) => {
      webview.addEventListener('new-window', (e) => {
        assert.equal(e.url, 'http://host/')
        assert.equal(e.frameName, 'target')
        done()
      })
      webview.src = `file://${fixtures}/pages/target-name.html`
      document.body.appendChild(webview)
    })
  })

  describe('ipc-message event', () => {
    it('emits when guest sends a ipc message to browser', (done) => {
      webview.addEventListener('ipc-message', (e) => {
        assert.equal(e.channel, 'channel')
        assert.deepEqual(e.args, ['arg1', 'arg2'])
        done()
      })
      webview.src = `file://${fixtures}/pages/ipc-message.html`
      webview.setAttribute('nodeintegration', 'on')
      document.body.appendChild(webview)
    })
  })

  describe('page-title-set event', () => {
    it('emits when title is set', (done) => {
      webview.addEventListener('page-title-set', (e) => {
        assert.equal(e.title, 'test')
        assert(e.explicitSet)
        done()
      })
      webview.src = `file://${fixtures}/pages/a.html`
      document.body.appendChild(webview)
    })
  })

  describe('page-favicon-updated event', () => {
    it('emits when favicon urls are received', (done) => {
      webview.addEventListener('page-favicon-updated', (e) => {
        assert.equal(e.favicons.length, 2)
        if (process.platform === 'win32') {
          assert(/^file:\/\/\/[A-Z]:\/favicon.png$/i.test(e.favicons[0]))
        } else {
          assert.equal(e.favicons[0], 'file:///favicon.png')
        }
        done()
      })
      webview.src = `file://${fixtures}/pages/a.html`
      document.body.appendChild(webview)
    })
  })

  describe('will-navigate event', () => {
    it('emits when a url that leads to oustide of the page is clicked', (done) => {
      webview.addEventListener('will-navigate', (e) => {
        assert.equal(e.url, 'http://host/')
        done()
      })
      webview.src = `file://${fixtures}/pages/webview-will-navigate.html`
      document.body.appendChild(webview)
    })
  })

  describe('did-navigate event', () => {
    let p = path.join(fixtures, 'pages', 'webview-will-navigate.html')
    p = p.replace(/\\/g, '/')
    const pageUrl = url.format({
      protocol: 'file',
      slashes: true,
      pathname: p
    })

    it('emits when a url that leads to outside of the page is clicked', (done) => {
      webview.addEventListener('did-navigate', (e) => {
        assert.equal(e.url, pageUrl)
        done()
      })
      webview.src = pageUrl
      document.body.appendChild(webview)
    })
  })

  describe('did-navigate-in-page event', () => {
    it('emits when an anchor link is clicked', (done) => {
      let p = path.join(fixtures, 'pages', 'webview-did-navigate-in-page.html')
      p = p.replace(/\\/g, '/')
      const pageUrl = url.format({
        protocol: 'file',
        slashes: true,
        pathname: p
      })
      webview.addEventListener('did-navigate-in-page', (e) => {
        assert.equal(e.url, `${pageUrl}#test_content`)
        done()
      })
      webview.src = pageUrl
      document.body.appendChild(webview)
    })

    it('emits when window.history.replaceState is called', (done) => {
      webview.addEventListener('did-navigate-in-page', (e) => {
        assert.equal(e.url, 'http://host/')
        done()
      })
      webview.src = `file://${fixtures}/pages/webview-did-navigate-in-page-with-history.html`
      document.body.appendChild(webview)
    })

    it('emits when window.location.hash is changed', (done) => {
      let p = path.join(fixtures, 'pages', 'webview-did-navigate-in-page-with-hash.html')
      p = p.replace(/\\/g, '/')
      const pageUrl = url.format({
        protocol: 'file',
        slashes: true,
        pathname: p
      })
      webview.addEventListener('did-navigate-in-page', (e) => {
        assert.equal(e.url, `${pageUrl}#test`)
        done()
      })
      webview.src = pageUrl
      document.body.appendChild(webview)
    })
  })

  describe('close event', () => {
    it('should fire when interior page calls window.close', (done) => {
      webview.addEventListener('close', () => done())
      webview.src = `file://${fixtures}/pages/close.html`
      document.body.appendChild(webview)
    })
  })

  describe('setDevToolsWebCotnents() API', () => {
    it('sets webContents of webview as devtools', (done) => {
      const webview2 = new WebView()
      webview2.addEventListener('did-attach', () => {
        webview2.addEventListener('dom-ready', () => {
          const devtools = webview2.getWebContents()
          assert.ok(devtools.getURL().startsWith('chrome-devtools://devtools'))
          devtools.executeJavaScript('InspectorFrontendHost.constructor.name', (name) => {
            assert.ok(name, 'InspectorFrontendHostImpl')
            document.body.removeChild(webview2)
            done()
          })
        })
        webview.addEventListener('dom-ready', () => {
          webview.getWebContents().setDevToolsWebContents(webview2.getWebContents())
          webview.getWebContents().openDevTools()
        })
        webview.src = 'about:blank'
        document.body.appendChild(webview)
      })
      document.body.appendChild(webview2)
    })
  })

  describe('devtools-opened event', () => {
    it('should fire when webview.openDevTools() is called', (done) => {
      const listener = () => {
        webview.removeEventListener('devtools-opened', listener)
        webview.closeDevTools()
        done()
      }
      webview.addEventListener('devtools-opened', listener)
      webview.addEventListener('dom-ready', () => {
        webview.openDevTools()
      })
      webview.src = `file://${fixtures}/pages/base-page.html`
      document.body.appendChild(webview)
    })
  })

  describe('devtools-closed event', () => {
    it('should fire when webview.closeDevTools() is called', (done) => {
      const listener2 = () => {
        webview.removeEventListener('devtools-closed', listener2)
        done()
      }
      const listener = () => {
        webview.removeEventListener('devtools-opened', listener)
        webview.closeDevTools()
      }
      webview.addEventListener('devtools-opened', listener)
      webview.addEventListener('devtools-closed', listener2)
      webview.addEventListener('dom-ready', () => {
        webview.openDevTools()
      })
      webview.src = `file://${fixtures}/pages/base-page.html`
      document.body.appendChild(webview)
    })
  })

  describe('devtools-focused event', () => {
    it('should fire when webview.openDevTools() is called', (done) => {
      const listener = () => {
        webview.removeEventListener('devtools-focused', listener)
        webview.closeDevTools()
        done()
      }
      webview.addEventListener('devtools-focused', listener)
      webview.addEventListener('dom-ready', () => {
        webview.openDevTools()
      })
      webview.src = `file://${fixtures}/pages/base-page.html`
      document.body.appendChild(webview)
    })
  })

  describe('<webview>.reload()', () => {
    it('should emit beforeunload handler', (done) => {
      const listener = (e) => {
        assert.equal(e.channel, 'onbeforeunload')
        webview.removeEventListener('ipc-message', listener)
        done()
      }
      const listener2 = () => {
        webview.reload()
        webview.removeEventListener('did-finish-load', listener2)
      }
      webview.addEventListener('ipc-message', listener)
      webview.addEventListener('did-finish-load', listener2)
      webview.setAttribute('nodeintegration', 'on')
      webview.src = `file://${fixtures}/pages/beforeunload-false.html`
      document.body.appendChild(webview)
    })
  })

  describe('<webview>.goForward()', () => {
    it('should work after a replaced history entry', (done) => {
      let loadCount = 1
      const listener = (e) => {
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

      const loadListener = (e) => {
        if (loadCount === 1) {
          webview.src = `file://${fixtures}/pages/base-page.html`
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

        loadCount += 1
      }

      webview.addEventListener('ipc-message', listener)
      webview.addEventListener('did-finish-load', loadListener)
      webview.setAttribute('nodeintegration', 'on')
      webview.src = `file://${fixtures}/pages/history-replace.html`
      document.body.appendChild(webview)
    })
  })

  describe('<webview>.clearHistory()', () => {
    it('should clear the navigation history', (done) => {
      const listener = (e) => {
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
      webview.src = `file://${fixtures}/pages/history.html`
      document.body.appendChild(webview)
    })
  })

  describe('basic auth', () => {
    const auth = require('basic-auth')

    it('should authenticate with correct credentials', (done) => {
      const message = 'Authenticated'
      const server = http.createServer((req, res) => {
        const credentials = auth(req)
        if (credentials.name === 'test' && credentials.pass === 'test') {
          res.end(message)
        } else {
          res.end('failed')
        }
        server.close()
      })
      server.listen(0, '127.0.0.1', () => {
        const port = server.address().port
        webview.addEventListener('ipc-message', (e) => {
          assert.equal(e.channel, message)
          done()
        })
        webview.src = `file://${fixtures}/pages/basic-auth.html?port=${port}`
        webview.setAttribute('nodeintegration', 'on')
        document.body.appendChild(webview)
      })
    })
  })

  describe('dom-ready event', () => {
    it('emits when document is loaded', (done) => {
      const server = http.createServer(() => {})
      server.listen(0, '127.0.0.1', () => {
        const port = server.address().port
        webview.addEventListener('dom-ready', () => {
          done()
        })
        webview.src = `file://${fixtures}/pages/dom-ready.html?port=${port}`
        document.body.appendChild(webview)
      })
    })

    it('throws a custom error when an API method is called before the event is emitted', () => {
      assert.throws(() => {
        webview.stop()
      }, 'Cannot call stop because the webContents is unavailable. The WebView must be attached to the DOM and the dom-ready event emitted before this method can be called.')
    })
  })

  describe('executeJavaScript', () => {
    it('should support user gesture', function (done) {
      const listener = () => {
        webview.removeEventListener('enter-html-full-screen', listener)
        done()
      }
      const listener2 = () => {
        const jsScript = "document.querySelector('video').webkitRequestFullscreen()"
        webview.executeJavaScript(jsScript, true)
        webview.removeEventListener('did-finish-load', listener2)
      }
      webview.addEventListener('enter-html-full-screen', listener)
      webview.addEventListener('did-finish-load', listener2)
      webview.src = `file://${fixtures}/pages/fullscreen.html`
      document.body.appendChild(webview)
    })

    it('can return the result of the executed script', function (done) {
      const listener = () => {
        const jsScript = "'4'+2"
        webview.executeJavaScript(jsScript, false, (result) => {
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

  describe('sendInputEvent', () => {
    it('can send keyboard event', (done) => {
      webview.addEventListener('ipc-message', (e) => {
        assert.equal(e.channel, 'keyup')
        assert.deepEqual(e.args, ['C', 'KeyC', 67, true, false])
        done()
      })
      webview.addEventListener('dom-ready', () => {
        webview.sendInputEvent({
          type: 'keyup',
          keyCode: 'c',
          modifiers: ['shift']
        })
      })
      webview.src = `file://${fixtures}/pages/onkeyup.html`
      webview.setAttribute('nodeintegration', 'on')
      document.body.appendChild(webview)
    })

    it('can send mouse event', (done) => {
      webview.addEventListener('ipc-message', (e) => {
        assert.equal(e.channel, 'mouseup')
        assert.deepEqual(e.args, [10, 20, false, true])
        done()
      })
      webview.addEventListener('dom-ready', () => {
        webview.sendInputEvent({
          type: 'mouseup',
          modifiers: ['ctrl'],
          x: 10,
          y: 20
        })
      })
      webview.src = `file://${fixtures}/pages/onmouseup.html`
      webview.setAttribute('nodeintegration', 'on')
      document.body.appendChild(webview)
    })
  })

  describe('media-started-playing media-paused events', () => {
    it('emits when audio starts and stops playing', (done) => {
      let audioPlayed = false
      webview.addEventListener('media-started-playing', () => {
        audioPlayed = true
      })
      webview.addEventListener('media-paused', () => {
        assert(audioPlayed)
        done()
      })
      webview.src = `file://${fixtures}/pages/audio.html`
      document.body.appendChild(webview)
    })
  })

  describe('found-in-page event', () => {
    it('emits when a request is made', (done) => {
      let requestId = null
      let activeMatchOrdinal = []
      const listener = (e) => {
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
      const listener2 = () => {
        requestId = webview.findInPage('virtual')
      }
      webview.addEventListener('found-in-page', listener)
      webview.addEventListener('did-finish-load', listener2)
      webview.src = `file://${fixtures}/pages/content.html`
      document.body.appendChild(webview)
      // TODO(deepak1556): With https://codereview.chromium.org/2836973002
      // focus of the webContents is required when triggering the api.
      // Remove this workaround after determining the cause for
      // incorrect focus.
      webview.focus()
    })
  })

  describe('did-change-theme-color event', () => {
    it('emits when theme color changes', (done) => {
      webview.addEventListener('did-change-theme-color', () => done())
      webview.src = `file://${fixtures}/pages/theme-color.html`
      document.body.appendChild(webview)
    })
  })

  describe('permission-request event', () => {
    function setUpRequestHandler (webview, requestedPermission, completed) {
      const listener = function (webContents, permission, callback) {
        if (webContents.getId() === webview.getId()) {
          // requestMIDIAccess with sysex requests both midi and midiSysex so
          // grant the first midi one and then reject the midiSysex one
          if (requestedPermission === 'midiSysex' && permission === 'midi') {
            return callback(true)
          }

          assert.equal(permission, requestedPermission)
          callback(false)
          if (completed) completed()
        }
      }
      session.fromPartition(webview.partition).setPermissionRequestHandler(listener)
    }

    it('emits when using navigator.getUserMedia api', (done) => {
      if (isCI) return done()

      webview.addEventListener('ipc-message', (e) => {
        assert.equal(e.channel, 'message')
        assert.deepEqual(e.args, ['PermissionDeniedError'])
        done()
      })
      webview.src = `file://${fixtures}/pages/permissions/media.html`
      webview.partition = 'permissionTest'
      webview.setAttribute('nodeintegration', 'on')
      setUpRequestHandler(webview, 'media')
      document.body.appendChild(webview)
    })

    it('emits when using navigator.geolocation api', (done) => {
      webview.addEventListener('ipc-message', (e) => {
        assert.equal(e.channel, 'message')
        assert.deepEqual(e.args, ['User denied Geolocation'])
        done()
      })
      webview.src = `file://${fixtures}/pages/permissions/geolocation.html`
      webview.partition = 'permissionTest'
      webview.setAttribute('nodeintegration', 'on')
      setUpRequestHandler(webview, 'geolocation')
      document.body.appendChild(webview)
    })

    it('emits when using navigator.requestMIDIAccess without sysex api', (done) => {
      webview.addEventListener('ipc-message', (e) => {
        assert.equal(e.channel, 'message')
        assert.deepEqual(e.args, ['SecurityError'])
        done()
      })
      webview.src = `file://${fixtures}/pages/permissions/midi.html`
      webview.partition = 'permissionTest'
      webview.setAttribute('nodeintegration', 'on')
      setUpRequestHandler(webview, 'midi')
      document.body.appendChild(webview)
    })

    it('emits when using navigator.requestMIDIAccess with sysex api', (done) => {
      webview.addEventListener('ipc-message', (e) => {
        assert.equal(e.channel, 'message')
        assert.deepEqual(e.args, ['SecurityError'])
        done()
      })
      webview.src = `file://${fixtures}/pages/permissions/midi-sysex.html`
      webview.partition = 'permissionTest'
      webview.setAttribute('nodeintegration', 'on')
      setUpRequestHandler(webview, 'midiSysex')
      document.body.appendChild(webview)
    })

    it('emits when accessing external protocol', (done) => {
      webview.src = 'magnet:test'
      webview.partition = 'permissionTest'
      setUpRequestHandler(webview, 'openExternal', done)
      document.body.appendChild(webview)
    })

    it('emits when using Notification.requestPermission', (done) => {
      webview.addEventListener('ipc-message', (e) => {
        assert.equal(e.channel, 'message')
        assert.deepEqual(e.args, ['granted'])
        done()
      })
      webview.src = `file://${fixtures}/pages/permissions/notification.html`
      webview.partition = 'permissionTest'
      webview.setAttribute('nodeintegration', 'on')
      session.fromPartition(webview.partition).setPermissionRequestHandler((webContents, permission, callback) => {
        if (webContents.getId() === webview.getId()) {
          assert.equal(permission, 'notifications')
          setTimeout(() => { callback(true) }, 10)
        }
      })
      document.body.appendChild(webview)
    })
  })

  describe('<webview>.getWebContents', () => {
    it('can return the webcontents associated', (done) => {
      webview.addEventListener('did-finish-load', () => {
        const webviewContents = webview.getWebContents()
        assert(webviewContents)
        assert.equal(webviewContents.getURL(), 'about:blank')
        done()
      })
      webview.src = 'about:blank'
      document.body.appendChild(webview)
    })
  })

  describe('did-get-response-details event', () => {
    it('emits for the page and its resources', (done) => {
      // expected {fileName: resourceType} pairs
      const expectedResources = {
        'did-get-response-details.html': 'mainFrame',
        'logo.png': 'image'
      }
      let responses = 0
      webview.addEventListener('did-get-response-details', (event) => {
        responses += 1
        const fileName = event.newURL.slice(event.newURL.lastIndexOf('/') + 1)
        const expectedType = expectedResources[fileName]
        assert(!!expectedType, `Unexpected response details for ${event.newURL}`)
        assert(typeof event.status === 'boolean', 'status should be boolean')
        assert.equal(event.httpResponseCode, 200)
        assert.equal(event.requestMethod, 'GET')
        assert(typeof event.referrer === 'string', 'referrer should be string')
        assert(!!event.headers, 'headers should be present')
        assert(typeof event.headers === 'object', 'headers should be object')
        assert.equal(event.resourceType, expectedType, 'Incorrect resourceType')
        if (responses === Object.keys(expectedResources).length) done()
      })
      webview.src = `file://${path.join(fixtures, 'pages', 'did-get-response-details.html')}`
      document.body.appendChild(webview)
    })
  })

  describe('document.visibilityState/hidden', () => {
    afterEach(() => {
      ipcMain.removeAllListeners('pong')
    })

    it('updates when the window is shown after the ready-to-show event', (done) => {
      w = new BrowserWindow({ show: false })
      w.once('ready-to-show', () => {
        w.show()
      })

      ipcMain.on('pong', (event, visibilityState, hidden) => {
        if (!hidden) {
          assert.equal(visibilityState, 'visible')
          done()
        }
      })

      w.loadURL(`file://${fixtures}/pages/webview-visibilitychange.html`)
    })

    it('inherits the parent window visibility state and receives visibilitychange events', (done) => {
      w = new BrowserWindow({ show: false })

      ipcMain.once('pong', (event, visibilityState, hidden) => {
        assert.equal(visibilityState, 'hidden')
        assert.equal(hidden, true)

        ipcMain.once('pong', function (event, visibilityState, hidden) {
          assert.equal(visibilityState, 'visible')
          assert.equal(hidden, false)
          done()
        })
        w.webContents.emit('-window-visibility-change', 'visible')
      })
      w.loadURL(`file://${fixtures}/pages/webview-visibilitychange.html`)
    })
  })

  describe('will-attach-webview event', () => {
    it('supports changing the web preferences', (done) => {
      ipcRenderer.send('disable-node-on-next-will-attach-webview')
      webview.addEventListener('console-message', (event) => {
        assert.equal(event.message, 'undefined undefined undefined undefined')
        done()
      })
      webview.setAttribute('nodeintegration', 'yes')
      webview.src = `file://${fixtures}/pages/a.html`
      document.body.appendChild(webview)
    })

    it('supports preventing a webview from being created', (done) => {
      ipcRenderer.send('prevent-next-will-attach-webview')
      webview.addEventListener('destroyed', () => {
        done()
      })
      webview.src = `file://${fixtures}/pages/c.html`
      document.body.appendChild(webview)
    })

    it('supports removing the preload script', (done) => {
      ipcRenderer.send('disable-preload-on-next-will-attach-webview')
      webview.addEventListener('console-message', (event) => {
        assert.equal(event.message, 'undefined')
        done()
      })
      webview.setAttribute('nodeintegration', 'yes')
      webview.setAttribute('preload', path.join(fixtures, 'module', 'preload-set-global.js'))
      webview.src = `file://${fixtures}/pages/a.html`
      document.body.appendChild(webview)
    })
  })

  describe('did-attach-webview event', () => {
    it('is emitted when a webview has been attached', (done) => {
      w = new BrowserWindow({ show: false })
      w.webContents.on('did-attach-webview', (event, webContents) => {
        ipcMain.once('webview-dom-ready', (event, id) => {
          assert.equal(webContents.id, id)
          done()
        })
      })
      w.loadURL(`file://${fixtures}/pages/webview-did-attach-event.html`)
    })
  })

  it('loads devtools extensions registered on the parent window', (done) => {
    w = new BrowserWindow({ show: false })
    BrowserWindow.removeDevToolsExtension('foo')

    const extensionPath = path.join(__dirname, 'fixtures', 'devtools-extensions', 'foo')
    BrowserWindow.addDevToolsExtension(extensionPath)

    w.loadURL(`file://${fixtures}/pages/webview-devtools.html`)

    ipcMain.once('answer', (event, message) => {
      assert.equal(message.runtimeId, 'foo')
      assert.notEqual(message.tabId, w.webContents.id)
      done()
    })
  })

  describe('guestinstance attribute', () => {
    it('before loading there is no attribute', () => {
      document.body.appendChild(webview)
      assert(!webview.hasAttribute('guestinstance'))
    })

    it('loading a page sets the guest view', (done) => {
      const loadListener = () => {
        webview.removeEventListener('did-finish-load', loadListener, false)
        const instance = webview.getAttribute('guestinstance')
        assert.equal(instance, parseInt(instance))

        const guest = getGuestWebContents(parseInt(instance))
        assert.equal(guest, webview.getWebContents())
        done()
      }
      webview.addEventListener('did-finish-load', loadListener, false)
      webview.src = `file://${fixtures}/api/blank.html`
      document.body.appendChild(webview)
    })

    it('deleting the attribute destroys the webview', (done) => {
      const loadListener = () => {
        webview.removeEventListener('did-finish-load', loadListener, false)
        const destroyListener = () => {
          webview.removeEventListener('destroyed', destroyListener, false)
          assert.equal(getGuestWebContents(instance), null)
          done()
        }
        webview.addEventListener('destroyed', destroyListener, false)

        const instance = parseInt(webview.getAttribute('guestinstance'))
        webview.removeAttribute('guestinstance')
      }
      webview.addEventListener('did-finish-load', loadListener, false)
      webview.src = `file://${fixtures}/api/blank.html`
      document.body.appendChild(webview)
    })

    it('setting the attribute on a new webview moves the contents', (done) => {
      const loadListener = () => {
        webview.removeEventListener('did-finish-load', loadListener, false)
        const webContents = webview.getWebContents()
        const instance = webview.getAttribute('guestinstance')

        const destroyListener = () => {
          webview.removeEventListener('destroyed', destroyListener, false)
          assert.equal(webContents, webview2.getWebContents())
          // Make sure that events are hooked up to the right webview now
          webview2.addEventListener('console-message', (e) => {
            assert.equal(e.message, 'a')
            document.body.removeChild(webview2)
            done()
          })

          webview2.src = `file://${fixtures}/pages/a.html`
        }
        webview.addEventListener('destroyed', destroyListener, false)

        const webview2 = new WebView()
        webview2.setAttribute('guestinstance', instance)
        document.body.appendChild(webview2)
      }
      webview.addEventListener('did-finish-load', loadListener, false)
      webview.src = `file://${fixtures}/api/blank.html`
      document.body.appendChild(webview)
    })

    it('setting the attribute to an invalid guestinstance does nothing', (done) => {
      const loadListener = () => {
        webview.removeEventListener('did-finish-load', loadListener, false)
        webview.setAttribute('guestinstance', 55)

        // Make sure that events are still hooked up to the webview
        webview.addEventListener('console-message', (e) => {
          assert.equal(e.message, 'a')
          done()
        })
        webview.src = `file://${fixtures}/pages/a.html`
      }
      webview.addEventListener('did-finish-load', loadListener, false)

      webview.src = `file://${fixtures}/api/blank.html`
      document.body.appendChild(webview)
    })

    it('setting the attribute on an existing webview moves the contents', (done) => {
      const load1Listener = () => {
        webview.removeEventListener('did-finish-load', load1Listener, false)
        const webContents = webview.getWebContents()
        const instance = webview.getAttribute('guestinstance')
        let destroyedInstance

        const destroyListener = () => {
          webview.removeEventListener('destroyed', destroyListener, false)
          assert.equal(webContents, webview2.getWebContents())
          assert.equal(null, getGuestWebContents(parseInt(destroyedInstance)))

          // Make sure that events are hooked up to the right webview now
          webview2.addEventListener('console-message', (e) => {
            assert.equal(e.message, 'a')
            document.body.removeChild(webview2)
            done()
          })

          webview2.src = 'file://' + fixtures + '/pages/a.html'
        }
        webview.addEventListener('destroyed', destroyListener, false)

        const webview2 = new WebView()
        const load2Listener = () => {
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

    it('moving a guest back to its original webview should work', (done) => {
      const loadListener = () => {
        webview.removeEventListener('did-finish-load', loadListener, false)
        const webContents = webview.getWebContents()
        const instance = webview.getAttribute('guestinstance')

        const destroy1Listener = () => {
          webview.removeEventListener('destroyed', destroy1Listener, false)
          assert.equal(webContents, webview2.getWebContents())
          assert.equal(null, webview.getWebContents())

          const destroy2Listener = () => {
            webview2.removeEventListener('destroyed', destroy2Listener, false)
            assert.equal(webContents, webview.getWebContents())
            assert.equal(null, webview2.getWebContents())

            // Make sure that events are hooked up to the right webview now
            webview.addEventListener('console-message', (e) => {
              assert.equal(e.message, 'a')
              document.body.removeChild(webview2)
              done()
            })

            webview.src = `file://${fixtures}/pages/a.html`
          }
          webview2.addEventListener('destroyed', destroy2Listener, false)
          webview.setAttribute('guestinstance', instance)
        }
        webview.addEventListener('destroyed', destroy1Listener, false)

        const webview2 = new WebView()
        webview2.setAttribute('guestinstance', instance)
        document.body.appendChild(webview2)
      }
      webview.addEventListener('did-finish-load', loadListener, false)
      webview.src = `file://${fixtures}/api/blank.html`
      document.body.appendChild(webview)
    })

    it('setting the attribute on a webview in a different window moves the contents', (done) => {
      const loadListener = () => {
        webview.removeEventListener('did-finish-load', loadListener, false)
        const instance = webview.getAttribute('guestinstance')

        w = new BrowserWindow({ show: false })
        w.webContents.once('did-finish-load', () => {
          ipcMain.once('pong', () => {
            assert(!webview.hasAttribute('guestinstance'))
            done()
          })

          w.webContents.send('guestinstance', instance)
        })
        w.loadURL(`file://${fixtures}/pages/webview-move-to-window.html`)
      }
      webview.addEventListener('did-finish-load', loadListener, false)
      webview.src = `file://${fixtures}/api/blank.html`
      document.body.appendChild(webview)
    })

    it('does not delete the guestinstance attribute when moving the webview to another parent node', (done) => {
      webview.addEventListener('dom-ready', function domReadyListener () {
        webview.addEventListener('did-attach', () => {
          assert(webview.guestinstance != null)
          assert(webview.getWebContents() != null)
          done()
        })

        document.body.replaceChild(webview, div)
      })
      webview.src = `file://${fixtures}/pages/a.html`

      const div = document.createElement('div')
      div.appendChild(webview)
      document.body.appendChild(div)
    })

    it('does not destroy the webContents when hiding/showing the webview (regression)', (done) => {
      webview.addEventListener('dom-ready', function domReadyListener () {
        const instance = webview.getAttribute('guestinstance')
        assert(instance != null)

        // Wait for event directly since attach happens asynchronously over IPC
        ipcMain.once('ELECTRON_GUEST_VIEW_MANAGER_ATTACH_GUEST', () => {
          assert(webview.getWebContents() != null)
          assert.equal(instance, webview.getAttribute('guestinstance'))
          done()
        })

        webview.style.display = 'none'
        webview.offsetHeight // eslint-disable-line
        webview.style.display = 'block'
      })
      webview.src = `file://${fixtures}/pages/a.html`
      document.body.appendChild(webview)
    })

    it('does not reload the webContents when hiding/showing the webview (regression)', (done) => {
      webview.addEventListener('dom-ready', function domReadyListener () {
        webview.addEventListener('did-start-loading', () => {
          done(new Error('webview started loading unexpectedly'))
        })

        // Wait for event directly since attach happens asynchronously over IPC
        webview.addEventListener('did-attach', () => {
          done()
        })

        webview.style.display = 'none'
        webview.offsetHeight  // eslint-disable-line
        webview.style.display = 'block'
      })
      webview.src = `file://${fixtures}/pages/a.html`
      document.body.appendChild(webview)
    })
  })

  describe('DOM events', () => {
    let div

    beforeEach(() => {
      div = document.createElement('div')
      div.style.width = '100px'
      div.style.height = '10px'
      div.style.overflow = 'hidden'
      webview.style.height = '100%'
      webview.style.width = '100%'
    })

    afterEach(() => {
      if (div != null) div.remove()
    })

    it('emits resize events', (done) => {
      webview.addEventListener('dom-ready', () => {
        div.style.width = '1234px'
        div.style.height = '789px'
      })

      webview.addEventListener('resize', function onResize (event) {
        webview.removeEventListener('resize', onResize)
        assert.equal(event.newWidth, 1234)
        assert.equal(event.newHeight, 789)
        assert.equal(event.target, webview)
        done()
      })

      webview.src = `file://${fixtures}/pages/a.html`
      div.appendChild(webview)
      document.body.appendChild(div)
    })
  })

  describe('disableguestresize attribute', () => {
    it('does not have attribute by default', () => {
      document.body.appendChild(webview)
      assert(!webview.hasAttribute('disableguestresize'))
    })

    it('resizes guest when attribute is not present', (done) => {
      w = new BrowserWindow({show: false, width: 200, height: 200})
      w.loadURL(`file://${fixtures}/pages/webview-guest-resize.html`)

      w.webContents.once('did-finish-load', () => {
        const CONTENT_SIZE = 300

        const elementResizePromise = new Promise(resolve => {
          ipcMain.once('webview-element-resize', (event, width, height) => {
            assert.equal(width, CONTENT_SIZE)
            assert.equal(height, CONTENT_SIZE)
            resolve()
          })
        })

        const guestResizePromise = new Promise(resolve => {
          ipcMain.once('webview-guest-resize', (event, width, height) => {
            assert.equal(width, CONTENT_SIZE)
            assert.equal(height, CONTENT_SIZE)
            resolve()
          })
        })

        Promise.all([elementResizePromise, guestResizePromise]).then(() => done())

        w.setContentSize(CONTENT_SIZE, CONTENT_SIZE)
      })
    })

    it('does not resize guest when attribute is present', done => {
      w = new BrowserWindow({show: false, width: 200, height: 200})
      w.loadURL(`file://${fixtures}/pages/webview-no-guest-resize.html`)

      w.webContents.once('did-finish-load', () => {
        const CONTENT_SIZE = 300

        const elementResizePromise = new Promise(resolve => {
          ipcMain.once('webview-element-resize', (event, width, height) => {
            assert.equal(width, CONTENT_SIZE)
            assert.equal(height, CONTENT_SIZE)
            resolve()
          })
        })

        const noGuestResizePromise = new Promise(resolve => {
          const onGuestResize = (event, width, height) => {
            done(new Error('Unexpected guest resize message'))
          }
          ipcMain.once('webview-guest-resize', onGuestResize)

          setTimeout(() => {
            ipcMain.removeListener('webview-guest-resize', onGuestResize)
            resolve()
          }, 500)
        })

        Promise.all([elementResizePromise, noGuestResizePromise]).then(() => done())

        w.setContentSize(CONTENT_SIZE, CONTENT_SIZE)
      })
    })

    it('dispatches element resize event even when attribute is present', done => {
      w = new BrowserWindow({show: false, width: 200, height: 200})
      w.loadURL(`file://${fixtures}/pages/webview-no-guest-resize.html`)

      w.webContents.once('did-finish-load', () => {
        const CONTENT_SIZE = 300

        ipcMain.once('webview-element-resize', (event, width, height) => {
          assert.equal(width, CONTENT_SIZE)
          done()
        })

        w.setContentSize(CONTENT_SIZE, CONTENT_SIZE)
      })
    })

    it('can be manually resized with setSize even when attribute is present', function (done) {
      w = new BrowserWindow({show: false, width: 200, height: 200})
      w.loadURL(`file://${fixtures}/pages/webview-no-guest-resize.html`)

      w.webContents.once('did-finish-load', () => {
        const GUEST_WIDTH = 10
        const GUEST_HEIGHT = 20

        ipcMain.once('webview-guest-resize', (event, width, height) => {
          assert.equal(width, GUEST_WIDTH)
          assert.equal(height, GUEST_HEIGHT)
          done()
        })

        for (const wc of webContents.getAllWebContents()) {
          if (wc.hostWebContents &&
              wc.hostWebContents.id === w.webContents.id) {
            wc.setSize({
              normal: {
                width: GUEST_WIDTH,
                height: GUEST_HEIGHT
              }
            })
          }
        }
      })
    })
  })

  describe('zoom behavior', () => {
    const zoomScheme = remote.getGlobal('zoomScheme')
    const webviewSession = session.fromPartition('webview-temp')

    before((done) => {
      const protocol = webviewSession.protocol
      protocol.registerStringProtocol(zoomScheme, (request, callback) => {
        callback('hello')
      }, (error) => done(error))
    })

    after((done) => {
      const protocol = webviewSession.protocol
      protocol.unregisterProtocol(zoomScheme, (error) => done(error))
    })

    it('inherits the zoomFactor of the parent window', (done) => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          zoomFactor: 1.2
        }
      })
      ipcMain.once('webview-parent-zoom-level', (event, zoomFactor, zoomLevel) => {
        assert.equal(zoomFactor, 1.2)
        assert.equal(zoomLevel, 1)
        done()
      })
      w.loadURL(`file://${fixtures}/pages/webview-zoom-factor.html`)
    })

    it('maintains zoom level on navigation', (done) => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          zoomFactor: 1.2
        }
      })
      ipcMain.on('webview-zoom-level', (event, zoomLevel, zoomFactor, newHost, final) => {
        if (!newHost) {
          assert.equal(zoomFactor, 1.44)
          assert.equal(zoomLevel, 2.0)
        } else {
          assert.equal(zoomFactor, 1.2)
          assert.equal(zoomLevel, 1)
        }
        if (final) done()
      })
      w.loadURL(`file://${fixtures}/pages/webview-custom-zoom-level.html`)
    })

    it('maintains zoom level when navigating within same page', (done) => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          zoomFactor: 1.2
        }
      })
      ipcMain.on('webview-zoom-in-page', (event, zoomLevel, zoomFactor, final) => {
        assert.equal(zoomFactor, 1.44)
        assert.equal(zoomLevel, 2.0)
        if (final) done()
      })
      w.loadURL(`file://${fixtures}/pages/webview-in-page-navigate.html`)
    })

    it('inherits zoom level for the origin when available', (done) => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          zoomFactor: 1.2
        }
      })
      ipcMain.once('webview-origin-zoom-level', (event, zoomLevel) => {
        assert.equal(zoomLevel, 2.0)
        done()
      })
      w.loadURL(`file://${fixtures}/pages/webview-origin-zoom-level.html`)
    })
  })

  describe('nativeWindowOpen option', () => {
    beforeEach(() => {
      webview.setAttribute('allowpopups', 'on')
      webview.setAttribute('nodeintegration', 'on')
      webview.setAttribute('webpreferences', 'nativeWindowOpen=1')
    })

    it('opens window of about:blank with cross-scripting enabled', (done) => {
      ipcMain.once('answer', (event, content) => {
        assert.equal(content, 'Hello')
        done()
      })
      webview.src = `file://${path.join(fixtures, 'api', 'native-window-open-blank.html')}`
      document.body.appendChild(webview)
    })

    it('opens window of same domain with cross-scripting enabled', (done) => {
      ipcMain.once('answer', (event, content) => {
        assert.equal(content, 'Hello')
        done()
      })
      webview.src = `file://${path.join(fixtures, 'api', 'native-window-open-file.html')}`
      document.body.appendChild(webview)
    })

    it('returns null from window.open when allowpopups is not set', (done) => {
      webview.removeAttribute('allowpopups')
      ipcMain.once('answer', (event, {windowOpenReturnedNull}) => {
        assert.equal(windowOpenReturnedNull, true)
        done()
      })
      webview.src = `file://${path.join(fixtures, 'api', 'native-window-open-no-allowpopups.html')}`
      document.body.appendChild(webview)
    })

    it('blocks accessing cross-origin frames', (done) => {
      ipcMain.once('answer', (event, content) => {
        assert.equal(content, 'Blocked a frame with origin "file://" from accessing a cross-origin frame.')
        done()
      })
      webview.src = `file://${path.join(fixtures, 'api', 'native-window-open-cross-origin.html')}`
      document.body.appendChild(webview)
    })

    it('emits a new-window event', (done) => {
      webview.addEventListener('new-window', (e) => {
        assert.equal(e.url, 'http://host/')
        assert.equal(e.frameName, 'host')
        done()
      })
      webview.src = `file://${fixtures}/pages/window-open.html`
      document.body.appendChild(webview)
    })

    it('emits a browser-window-created event', (done) => {
      app.once('browser-window-created', () => done())
      webview.src = `file://${fixtures}/pages/window-open.html`
      document.body.appendChild(webview)
    })

    it('emits a web-contents-created event', (done) => {
      app.on('web-contents-created', function listener (event, contents) {
        if (contents.getType() === 'window') {
          app.removeListener('web-contents-created', listener)
          done()
        }
      })
      webview.src = `file://${fixtures}/pages/window-open.html`
      document.body.appendChild(webview)
    })
  })
})

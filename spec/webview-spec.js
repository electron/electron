const assert = require('assert')
const chai = require('chai')
const dirtyChai = require('dirty-chai')
const path = require('path')
const http = require('http')
const url = require('url')
const { ipcRenderer, remote } = require('electron')
const { app, session, ipcMain, BrowserWindow } = remote
const { closeWindow } = require('./window-helpers')
const { emittedOnce, waitForEvent } = require('./events-helpers')

const { expect } = chai
chai.use(dirtyChai)

const isCI = remote.getGlobal('isCi')
const nativeModulesEnabled = remote.getGlobal('nativeModulesEnabled')

/* Most of the APIs here don't use standard callbacks */
/* eslint-disable standard/no-callback-literal */

describe('<webview> tag', function () {
  this.timeout(3 * 60 * 1000)

  const fixtures = path.join(__dirname, 'fixtures')
  let webview = null
  let w = null

  const openTheWindow = async (...args) => {
    await closeTheWindow()
    w = new BrowserWindow(...args)
    return w
  }

  const closeTheWindow = async () => {
    await closeWindow(w)
    w = null
  }

  const loadWebView = async (webview, attributes = {}) => {
    for (const [name, value] of Object.entries(attributes)) {
      webview.setAttribute(name, value)
    }
    document.body.appendChild(webview)
    await waitForEvent(webview, 'did-finish-load')
    return webview
  }

  const startLoadingWebViewAndWaitForMessage = async (webview, attributes = {}) => {
    loadWebView(webview, attributes) // Don't wait for load to be finished.
    const event = await waitForEvent(webview, 'console-message')
    return event.message
  }

  beforeEach(() => {
    webview = new WebView()
  })

  afterEach(() => {
    if (!document.body.contains(webview)) {
      document.body.appendChild(webview)
    }
    webview.remove()

    return closeTheWindow()
  })

  it('works without script tag in page', async () => {
    const w = await openTheWindow({
      show: false,
      webPreferences: {
        webviewTag: true,
        nodeIntegration: true
      }
    })
    w.loadFile(path.join(fixtures, 'pages', 'webview-no-script.html'))
    await emittedOnce(ipcMain, 'pong')
  })

  it('works with contextIsolation', async () => {
    const w = await openTheWindow({
      show: false,
      webPreferences: {
        webviewTag: true,
        nodeIntegration: true,
        contextIsolation: true
      }
    })
    w.loadFile(path.join(fixtures, 'pages', 'webview-isolated.html'))
    await emittedOnce(ipcMain, 'pong')
  })

  it('is disabled by default', async () => {
    const w = await openTheWindow({
      show: false,
      webPreferences: {
        preload: path.join(fixtures, 'module', 'preload-webview.js'),
        nodeIntegration: true
      }
    })

    w.loadFile(path.join(fixtures, 'pages', 'webview-no-script.html'))
    const [, type] = await emittedOnce(ipcMain, 'webview')

    expect(type).to.equal('undefined', 'WebView still exists')
  })

  describe('src attribute', () => {
    it('specifies the page to load', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        src: `file://${fixtures}/pages/a.html`
      })
      expect(message).to.equal('a')
    })

    it('navigates to new page when changed', async () => {
      await loadWebView(webview, {
        src: `file://${fixtures}/pages/a.html`
      })

      webview.src = `file://${fixtures}/pages/b.html`

      const { message } = await waitForEvent(webview, 'console-message')
      expect(message).to.equal('b')
    })

    it('resolves relative URLs', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        src: '../fixtures/pages/e.html'
      })
      assert.strictEqual(message, 'Window script is loaded before preload script')
    })

    it('ignores empty values', () => {
      assert.strictEqual(webview.src, '')

      for (const emptyValue of ['', null, undefined]) {
        webview.src = emptyValue
        expect(webview.src).to.equal('')
      }
    })
  })

  describe('nodeintegration attribute', () => {
    it('inserts no node symbols when not set', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        src: `file://${fixtures}/pages/c.html`
      })

      const types = JSON.parse(message)
      expect(types).to.include({
        require: 'undefined',
        module: 'undefined',
        process: 'undefined',
        global: 'undefined'
      })
    })

    it('inserts node symbols when set', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        nodeintegration: 'on',
        src: `file://${fixtures}/pages/d.html`
      })

      const types = JSON.parse(message)
      expect(types).to.include({
        require: 'function',
        module: 'object',
        process: 'object'
      })
    })

    it('loads node symbols after POST navigation when set', async function () {
      // FIXME Figure out why this is timing out on AppVeyor
      if (process.env.APPVEYOR === 'True') {
        this.skip()
        return
      }

      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        nodeintegration: 'on',
        src: `file://${fixtures}/pages/post.html`
      })

      const types = JSON.parse(message)
      expect(types).to.include({
        require: 'function',
        module: 'object',
        process: 'object'
      })
    })

    it('disables node integration on child windows when it is disabled on the webview', (done) => {
      app.once('browser-window-created', (event, window) => {
        assert.strictEqual(window.webContents.getWebPreferences().nodeIntegration, false)
        done()
      })

      const src = url.format({
        pathname: `${fixtures}/pages/webview-opener-no-node-integration.html`,
        protocol: 'file',
        query: {
          p: `${fixtures}/pages/window-opener-node.html`
        },
        slashes: true
      })
      loadWebView(webview, {
        allowpopups: 'on',
        src
      })
    });

    (nativeModulesEnabled ? it : it.skip)('loads native modules when navigation happens', async function () {
      await loadWebView(webview, {
        nodeintegration: 'on',
        src: `file://${fixtures}/pages/native-module.html`
      })

      webview.reload()

      const { message } = await waitForEvent(webview, 'console-message')
      assert.strictEqual(message, 'function')
    })
  })

  describe('enableremotemodule attribute', () => {
    const generateSpecs = (description, sandbox) => {
      describe(description, () => {
        const preload = `${fixtures}/module/preload-disable-remote.js`
        const src = `file://${fixtures}/api/blank.html`

        it('enables the remote module by default', async () => {
          const message = await startLoadingWebViewAndWaitForMessage(webview, {
            preload,
            src,
            sandbox
          })

          const typeOfRemote = JSON.parse(message)
          expect(typeOfRemote).to.equal('object')
        })

        it('disables the remote module when false', async () => {
          const message = await startLoadingWebViewAndWaitForMessage(webview, {
            preload,
            src,
            sandbox,
            enableremotemodule: false
          })

          const typeOfRemote = JSON.parse(message)
          expect(typeOfRemote).to.equal('undefined')
        })
      })
    }

    generateSpecs('without sandbox', false)
    generateSpecs('with sandbox', true)
  })

  describe('preload attribute', () => {
    it('loads the script before other scripts in window', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        preload: `${fixtures}/module/preload.js`,
        src: `file://${fixtures}/pages/e.html`
      })

      expect(message).to.be.a('string')
      expect(message).to.be.not.equal('Window script is loaded before preload script')
    })

    it('preload script can still use "process" and "Buffer" when nodeintegration is off', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        preload: `${fixtures}/module/preload-node-off.js`,
        src: `file://${fixtures}/api/blank.html`
      })

      const types = JSON.parse(message)
      expect(types).to.include({
        process: 'object',
        Buffer: 'function'
      })
    })

    it('runs in the correct scope when sandboxed', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        preload: `${fixtures}/module/preload-context.js`,
        src: `file://${fixtures}/api/blank.html`,
        webpreferences: 'sandbox=yes'
      })

      const types = JSON.parse(message)
      expect(types).to.include({
        require: 'function', // arguments passed to it should be availale
        electron: 'undefined', // objects from the scope it is called from should not be available
        window: 'object', // the window object should be available
        localVar: 'undefined' // but local variables should not be exposed to the window
      })
    })

    it('preload script can require modules that still use "process" and "Buffer" when nodeintegration is off', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        preload: `${fixtures}/module/preload-node-off-wrapper.js`,
        src: `file://${fixtures}/api/blank.html`
      })

      const types = JSON.parse(message)
      expect(types).to.include({
        process: 'object',
        Buffer: 'function'
      })
    })

    it('receives ipc message in preload script', async () => {
      await loadWebView(webview, {
        preload: `${fixtures}/module/preload-ipc.js`,
        src: `file://${fixtures}/pages/e.html`
      })

      const message = 'boom!'
      webview.send('ping', message)

      const { channel, args } = await waitForEvent(webview, 'ipc-message')
      assert.strictEqual(channel, 'pong')
      assert.deepStrictEqual(args, [message])
    })

    it('works without script tag in page', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        preload: `${fixtures}/module/preload.js`,
        src: `file://${fixtures}pages/base-page.html`
      })

      const types = JSON.parse(message)
      expect(types).to.include({
        require: 'function',
        module: 'object',
        process: 'object',
        Buffer: 'function'
      })
    })

    it('resolves relative URLs', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        preload: '../fixtures/module/preload.js',
        src: `file://${fixtures}/pages/e.html`
      })

      const types = JSON.parse(message)
      expect(types).to.include({
        require: 'function',
        module: 'object',
        process: 'object',
        Buffer: 'function'
      })
    })

    it('ignores empty values', () => {
      assert.strictEqual(webview.preload, '')

      for (const emptyValue of ['', null, undefined]) {
        webview.preload = emptyValue
        assert.strictEqual(webview.preload, '')
      }
    })
  })

  describe('httpreferrer attribute', () => {
    it('sets the referrer url', (done) => {
      const referrer = 'http://github.com/'
      const server = http.createServer((req, res) => {
        res.end()
        server.close()
        assert.strictEqual(req.headers.referer, referrer)
        done()
      }).listen(0, '127.0.0.1', () => {
        const port = server.address().port
        loadWebView(webview, {
          httpreferrer: referrer,
          src: `http://127.0.0.1:${port}`
        })
      })
    })
  })

  describe('useragent attribute', () => {
    it('sets the user agent', async () => {
      const referrer = 'Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; AS; rv:11.0) like Gecko'
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        src: `file://${fixtures}/pages/useragent.html`,
        useragent: referrer
      })
      expect(message).to.equal(referrer)
    })
  })

  describe('disablewebsecurity attribute', () => {
    it('does not disable web security when not set', async () => {
      const jqueryPath = path.join(__dirname, '/static/jquery-2.0.3.min.js')
      const src = `<script src='file://${jqueryPath}'></script> <script>console.log('ok');</script>`
      const encoded = btoa(unescape(encodeURIComponent(src)))

      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        src: `data:text/html;base64,${encoded}`
      })
      expect(message).to.be.a('string')
      expect(message).to.contain('Not allowed to load local resource')
    })

    it('disables web security when set', async () => {
      const jqueryPath = path.join(__dirname, '/static/jquery-2.0.3.min.js')
      const src = `<script src='file://${jqueryPath}'></script> <script>console.log('ok');</script>`
      const encoded = btoa(unescape(encodeURIComponent(src)))

      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        disablewebsecurity: '',
        src: `data:text/html;base64,${encoded}`
      })
      expect(message).to.equal('ok')
    })

    it('does not break node integration', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        disablewebsecurity: '',
        nodeintegration: 'on',
        src: `file://${fixtures}/pages/d.html`
      })

      const types = JSON.parse(message)
      expect(types).to.include({
        require: 'function',
        module: 'object',
        process: 'object'
      })
    })

    it('does not break preload script', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        disablewebsecurity: '',
        preload: `${fixtures}/module/preload.js`,
        src: `file://${fixtures}/pages/e.html`
      })

      const types = JSON.parse(message)
      expect(types).to.include({
        require: 'function',
        module: 'object',
        process: 'object',
        Buffer: 'function'
      })
    })
  })

  describe('partition attribute', () => {
    it('inserts no node symbols when not set', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        partition: 'test1',
        src: `file://${fixtures}/pages/c.html`
      })

      const types = JSON.parse(message)
      expect(types).to.include({
        require: 'undefined',
        module: 'undefined',
        process: 'undefined',
        global: 'undefined'
      })
    })

    it('inserts node symbols when set', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        nodeintegration: 'on',
        partition: 'test2',
        src: `file://${fixtures}/pages/d.html`
      })

      const types = JSON.parse(message)
      expect(types).to.include({
        require: 'function',
        module: 'object',
        process: 'object'
      })
    })

    it('isolates storage for different id', async () => {
      window.localStorage.setItem('test', 'one')

      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        partition: 'test3',
        src: `file://${fixtures}/pages/partition/one.html`
      })

      const parsedMessage = JSON.parse(message)
      expect(parsedMessage).to.include({
        numberOfEntries: 0,
        testValue: null
      })
    })

    it('uses current session storage when no id is provided', async () => {
      const testValue = 'one'
      window.localStorage.setItem('test', testValue)

      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        src: `file://${fixtures}/pages/partition/one.html`
      })

      const parsedMessage = JSON.parse(message)
      expect(parsedMessage).to.include({
        numberOfEntries: 1,
        testValue
      })
    })
  })

  describe('allowpopups attribute', () => {
    it('can not open new window when not set', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        src: `file://${fixtures}/pages/window-open-hide.html`
      })
      expect(message).to.equal('null')
    })

    it('can open new window when set', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        allowpopups: 'on',
        src: `file://${fixtures}/pages/window-open-hide.html`
      })
      expect(message).to.equal('window')
    })
  })

  describe('webpreferences attribute', () => {
    it('can enable nodeintegration', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        src: `file://${fixtures}/pages/d.html`,
        webpreferences: 'nodeIntegration'
      })

      const types = JSON.parse(message)
      expect(types).to.include({
        require: 'function',
        module: 'object',
        process: 'object'
      })
    })

    it('can disable the remote module', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        preload: `${fixtures}/module/preload-disable-remote.js`,
        src: `file://${fixtures}/api/blank.html`,
        webpreferences: 'enableRemoteModule=no'
      })

      const typeOfRemote = JSON.parse(message)
      expect(typeOfRemote).to.equal('undefined')
    })

    it('can disables web security and enable nodeintegration', async () => {
      const jqueryPath = path.join(__dirname, '/static/jquery-2.0.3.min.js')
      const src = `<script src='file://${jqueryPath}'></script> <script>console.log(typeof require);</script>`
      const encoded = btoa(unescape(encodeURIComponent(src)))

      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        src: `data:text/html;base64,${encoded}`,
        webpreferences: 'webSecurity=no, nodeIntegration=yes'
      })

      expect(message).to.equal('function')
    })

    it('can enable context isolation', async () => {
      loadWebView(webview, {
        allowpopups: 'yes',
        preload: path.join(fixtures, 'api', 'isolated-preload.js'),
        src: `file://${fixtures}/api/isolated.html`,
        webpreferences: 'contextIsolation=yes'
      })

      const [, data] = await emittedOnce(ipcMain, 'isolated-world')
      assert.deepStrictEqual(data, {
        preloadContext: {
          preloadProperty: 'number',
          pageProperty: 'undefined',
          typeofRequire: 'function',
          typeofProcess: 'object',
          typeofArrayPush: 'function',
          typeofFunctionApply: 'function',
          typeofPreloadExecuteJavaScriptProperty: 'undefined'
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
    })
  })

  describe('new-window event', () => {
    it('emits when window.open is called', async () => {
      loadWebView(webview, {
        src: `file://${fixtures}/pages/window-open.html`
      })
      const { url, frameName } = await waitForEvent(webview, 'new-window')

      assert.strictEqual(url, 'http://host/')
      assert.strictEqual(frameName, 'host')
    })

    it('emits when link with target is called', async () => {
      loadWebView(webview, {
        src: `file://${fixtures}/pages/target-name.html`
      })
      const { url, frameName } = await waitForEvent(webview, 'new-window')

      assert.strictEqual(url, 'http://host/')
      assert.strictEqual(frameName, 'target')
    })
  })

  describe('ipc-message event', () => {
    it('emits when guest sends a ipc message to browser', async () => {
      loadWebView(webview, {
        nodeintegration: 'on',
        src: `file://${fixtures}/pages/ipc-message.html`
      })
      const { channel, args } = await waitForEvent(webview, 'ipc-message')

      assert.strictEqual(channel, 'channel')
      assert.deepStrictEqual(args, ['arg1', 'arg2'])
    })
  })

  describe('page-title-set event', () => {
    it('emits when title is set', async () => {
      loadWebView(webview, {
        src: `file://${fixtures}/pages/a.html`
      })
      const { title, explicitSet } = await waitForEvent(webview, 'page-title-set')

      assert.strictEqual(title, 'test')
      assert(explicitSet)
    })
  })

  describe('page-favicon-updated event', () => {
    it('emits when favicon urls are received', async () => {
      loadWebView(webview, {
        src: `file://${fixtures}/pages/a.html`
      })
      const { favicons } = await waitForEvent(webview, 'page-favicon-updated')

      assert(favicons)
      assert.strictEqual(favicons.length, 2)
      if (process.platform === 'win32') {
        assert(/^file:\/\/\/[A-Z]:\/favicon.png$/i.test(favicons[0]))
      } else {
        assert.strictEqual(favicons[0], 'file:///favicon.png')
      }
    })
  })

  describe('will-navigate event', () => {
    it('emits when a url that leads to oustide of the page is clicked', async () => {
      loadWebView(webview, {
        src: `file://${fixtures}/pages/webview-will-navigate.html`
      })
      const { url } = await waitForEvent(webview, 'will-navigate')

      assert.strictEqual(url, 'http://host/')
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

    it('emits when a url that leads to outside of the page is clicked', async () => {
      loadWebView(webview, { src: pageUrl })
      const { url } = await waitForEvent(webview, 'did-navigate')

      assert.strictEqual(url, pageUrl)
    })
  })

  describe('did-navigate-in-page event', () => {
    it('emits when an anchor link is clicked', async () => {
      let p = path.join(fixtures, 'pages', 'webview-did-navigate-in-page.html')
      p = p.replace(/\\/g, '/')
      const pageUrl = url.format({
        protocol: 'file',
        slashes: true,
        pathname: p
      })
      loadWebView(webview, { src: pageUrl })
      const event = await waitForEvent(webview, 'did-navigate-in-page')
      assert.strictEqual(event.url, `${pageUrl}#test_content`)
    })

    it('emits when window.history.replaceState is called', async () => {
      loadWebView(webview, {
        src: `file://${fixtures}/pages/webview-did-navigate-in-page-with-history.html`
      })
      const { url } = await waitForEvent(webview, 'did-navigate-in-page')
      assert.strictEqual(url, 'http://host/')
    })

    it('emits when window.location.hash is changed', async () => {
      let p = path.join(fixtures, 'pages', 'webview-did-navigate-in-page-with-hash.html')
      p = p.replace(/\\/g, '/')
      const pageUrl = url.format({
        protocol: 'file',
        slashes: true,
        pathname: p
      })
      loadWebView(webview, { src: pageUrl })
      const event = await waitForEvent(webview, 'did-navigate-in-page')
      assert.strictEqual(event.url, `${pageUrl}#test`)
    })
  })

  describe('close event', () => {
    it('should fire when interior page calls window.close', async () => {
      loadWebView(webview, { src: `file://${fixtures}/pages/close.html` })
      await waitForEvent(webview, 'close')
    })
  })

  // FIXME(zcbenz): Disabled because of moving to OOPIF webview.
  xdescribe('setDevToolsWebContents() API', () => {
    it('sets webContents of webview as devtools', async () => {
      const webview2 = new WebView()
      loadWebView(webview2)

      // Setup an event handler for further usage.
      const waitForDomReady = waitForEvent(webview2, 'dom-ready')

      loadWebView(webview, { src: 'about:blank' })
      await waitForEvent(webview, 'dom-ready')
      webview.getWebContents().setDevToolsWebContents(webview2.getWebContents())
      webview.getWebContents().openDevTools()

      await waitForDomReady

      // Its WebContents should be a DevTools.
      const devtools = webview2.getWebContents()
      assert.ok(devtools.getURL().startsWith('chrome-devtools://devtools'))

      const name = await new Promise((resolve) => {
        devtools.executeJavaScript('InspectorFrontendHost.constructor.name', (name) => {
          resolve(name)
        })
      })
      document.body.removeChild(webview2)

      expect(name).to.be.equal('InspectorFrontendHostImpl')
    })
  })

  describe('devtools-opened event', () => {
    it('should fire when webview.openDevTools() is called', async () => {
      loadWebView(webview, {
        src: `file://${fixtures}/pages/base-page.html`
      })
      await waitForEvent(webview, 'dom-ready')

      webview.openDevTools()
      await waitForEvent(webview, 'devtools-opened')

      webview.closeDevTools()
    })
  })

  describe('devtools-closed event', () => {
    it('should fire when webview.closeDevTools() is called', async () => {
      loadWebView(webview, {
        src: `file://${fixtures}/pages/base-page.html`
      })
      await waitForEvent(webview, 'dom-ready')

      webview.openDevTools()
      await waitForEvent(webview, 'devtools-opened')

      webview.closeDevTools()
      await waitForEvent(webview, 'devtools-closed')
    })
  })

  describe('devtools-focused event', () => {
    it('should fire when webview.openDevTools() is called', async () => {
      loadWebView(webview, {
        src: `file://${fixtures}/pages/base-page.html`
      })

      const waitForDevToolsFocused = waitForEvent(webview, 'devtools-focused')

      await waitForEvent(webview, 'dom-ready')
      webview.openDevTools()

      await waitForDevToolsFocused
      webview.closeDevTools()
    })
  })

  describe('<webview>.reload()', () => {
    it('should emit beforeunload handler', async () => {
      await loadWebView(webview, {
        nodeintegration: 'on',
        src: `file://${fixtures}/pages/beforeunload-false.html`
      })

      // Event handler has to be added before reload.
      const waitForOnbeforeunload = waitForEvent(webview, 'ipc-message')

      webview.reload()

      const { channel } = await waitForOnbeforeunload
      assert.strictEqual(channel, 'onbeforeunload')
    })
  })

  describe('<webview>.goForward()', () => {
    it('should work after a replaced history entry', (done) => {
      let loadCount = 1
      const listener = (e) => {
        if (loadCount === 1) {
          assert.strictEqual(e.channel, 'history')
          assert.strictEqual(e.args[0], 1)
          assert(!webview.canGoBack())
          assert(!webview.canGoForward())
        } else if (loadCount === 2) {
          assert.strictEqual(e.channel, 'history')
          assert.strictEqual(e.args[0], 2)
          assert(!webview.canGoBack())
          assert(webview.canGoForward())
          webview.removeEventListener('ipc-message', listener)
        }
      }

      const loadListener = () => {
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

      loadWebView(webview, {
        nodeintegration: 'on',
        src: `file://${fixtures}/pages/history-replace.html`
      })
    })
  })

  describe('<webview>.clearHistory()', () => {
    it('should clear the navigation history', async () => {
      loadWebView(webview, {
        nodeintegration: 'on',
        src: `file://${fixtures}/pages/history.html`
      })
      const event = await waitForEvent(webview, 'ipc-message')

      assert.strictEqual(event.channel, 'history')
      assert.strictEqual(event.args[0], 2)
      assert(webview.canGoBack())

      webview.clearHistory()
      assert(!webview.canGoBack())
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
          assert.strictEqual(e.channel, message)
          done()
        })
        loadWebView(webview, {
          nodeintegration: 'on',
          src: `file://${fixtures}/pages/basic-auth.html?port=${port}`
        })
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
        loadWebView(webview, {
          src: `file://${fixtures}/pages/dom-ready.html?port=${port}`
        })
      })
    })

    it('throws a custom error when an API method is called before the event is emitted', () => {
      const expectedErrorMessage =
          'The WebView must be attached to the DOM ' +
          'and the dom-ready event emitted before this method can be called.'
      expect(() => { webview.stop() }).to.throw(expectedErrorMessage)
    })
  })

  describe('executeJavaScript', () => {
    it('should support user gesture', async () => {
      await loadWebView(webview, {
        src: `file://${fixtures}/pages/fullscreen.html`
      })

      // Event handler has to be added before js execution.
      const waitForEnterHtmlFullScreen = waitForEvent(webview, 'enter-html-full-screen')

      const jsScript = "document.querySelector('video').webkitRequestFullscreen()"
      webview.executeJavaScript(jsScript, true)

      return waitForEnterHtmlFullScreen
    })

    it('can return the result of the executed script', async () => {
      await loadWebView(webview, {
        src: 'about:blank'
      })

      const jsScript = "'4'+2"
      const expectedResult = '42'

      const result = await new Promise((resolve) => {
        webview.executeJavaScript(jsScript, false, (result) => {
          resolve(result)
        })
      })
      assert.strictEqual(result, expectedResult)
    })
  })

  describe('sendInputEvent', () => {
    it('can send keyboard event', async () => {
      loadWebView(webview, {
        nodeintegration: 'on',
        src: `file://${fixtures}/pages/onkeyup.html`
      })
      await waitForEvent(webview, 'dom-ready')

      const waitForIpcMessage = waitForEvent(webview, 'ipc-message')
      webview.sendInputEvent({
        type: 'keyup',
        keyCode: 'c',
        modifiers: ['shift']
      })

      const { channel, args } = await waitForIpcMessage
      assert.strictEqual(channel, 'keyup')
      assert.deepStrictEqual(args, ['C', 'KeyC', 67, true, false])
    })

    it('can send mouse event', async () => {
      loadWebView(webview, {
        nodeintegration: 'on',
        src: `file://${fixtures}/pages/onmouseup.html`
      })
      await waitForEvent(webview, 'dom-ready')

      const waitForIpcMessage = waitForEvent(webview, 'ipc-message')
      webview.sendInputEvent({
        type: 'mouseup',
        modifiers: ['ctrl'],
        x: 10,
        y: 20
      })

      const { channel, args } = await waitForIpcMessage
      assert.strictEqual(channel, 'mouseup')
      assert.deepStrictEqual(args, [10, 20, false, true])
    })
  })

  describe('media-started-playing media-paused events', () => {
    it('emits when audio starts and stops playing', async () => {
      await loadWebView(webview, { src: `file://${fixtures}/pages/base-page.html` })

      // With the new autoplay policy, audio elements must be unmuted
      // see https://goo.gl/xX8pDD.
      const source = `
        const audio = document.createElement("audio")
        audio.src = "../assets/tone.wav"
        document.body.appendChild(audio);
        audio.play()
      `
      webview.executeJavaScript(source, true)
      await waitForEvent(webview, 'media-started-playing')

      webview.executeJavaScript('document.querySelector("audio").pause()', true)
      await waitForEvent(webview, 'media-paused')
    })
  })

  describe('found-in-page event', () => {
    it('emits when a request is made', (done) => {
      let requestId = null
      const activeMatchOrdinal = []
      const listener = (e) => {
        assert.strictEqual(e.result.requestId, requestId)
        assert.strictEqual(e.result.matches, 3)
        activeMatchOrdinal.push(e.result.activeMatchOrdinal)
        if (e.result.activeMatchOrdinal === e.result.matches) {
          assert.deepStrictEqual(activeMatchOrdinal, [1, 2, 3])
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
      loadWebView(webview, { src: `file://${fixtures}/pages/content.html` })
      // TODO(deepak1556): With https://codereview.chromium.org/2836973002
      // focus of the webContents is required when triggering the api.
      // Remove this workaround after determining the cause for
      // incorrect focus.
      webview.focus()
    })
  })

  describe('did-change-theme-color event', () => {
    it('emits when theme color changes', async () => {
      loadWebView(webview, {
        src: `file://${fixtures}/pages/theme-color.html`
      })
      await waitForEvent(webview, 'did-change-theme-color')
    })
  })

  describe('permission-request event', () => {
    function setUpRequestHandler (webview, requestedPermission, completed) {
      assert.ok(webview.partition)

      const listener = function (webContents, permission, callback) {
        if (webContents.id === webview.getWebContents().id) {
          // requestMIDIAccess with sysex requests both midi and midiSysex so
          // grant the first midi one and then reject the midiSysex one
          if (requestedPermission === 'midiSysex' && permission === 'midi') {
            return callback(true)
          }

          assert.strictEqual(permission, requestedPermission)
          callback(false)
          if (completed) completed()
        }
      }
      session.fromPartition(webview.partition).setPermissionRequestHandler(listener)
    }

    it('emits when using navigator.getUserMedia api', (done) => {
      if (isCI) return done()

      webview.addEventListener('ipc-message', (e) => {
        assert.strictEqual(e.channel, 'message')
        assert.deepStrictEqual(e.args, ['PermissionDeniedError'])
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
        assert.strictEqual(e.channel, 'message')
        assert.deepStrictEqual(e.args, ['User denied Geolocation'])
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
        assert.strictEqual(e.channel, 'message')
        assert.deepStrictEqual(e.args, ['SecurityError'])
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
        assert.strictEqual(e.channel, 'message')
        assert.deepStrictEqual(e.args, ['SecurityError'])
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
        assert.strictEqual(e.channel, 'message')
        assert.deepStrictEqual(e.args, ['granted'])
        done()
      })
      webview.src = `file://${fixtures}/pages/permissions/notification.html`
      webview.partition = 'permissionTest'
      webview.setAttribute('nodeintegration', 'on')
      session.fromPartition(webview.partition).setPermissionRequestHandler((webContents, permission, callback) => {
        if (webContents.id === webview.getWebContents().id) {
          assert.strictEqual(permission, 'notifications')
          setTimeout(() => { callback(true) }, 10)
        }
      })
      document.body.appendChild(webview)
    })
  })

  describe('<webview>.getWebContents', () => {
    it('can return the webcontents associated', async () => {
      const src = 'about:blank'
      await loadWebView(webview, { src })

      const webviewContents = webview.getWebContents()
      assert(webviewContents)
      expect(webviewContents.getURL()).to.equal(src)
    })
  })

  // FIXME(deepak1556): Ch69 follow up.
  xdescribe('document.visibilityState/hidden', () => {
    afterEach(() => {
      ipcMain.removeAllListeners('pong')
    })

    it('updates when the window is shown after the ready-to-show event', async () => {
      const w = await openTheWindow({ show: false })
      const readyToShowSignal = emittedOnce(w, 'ready-to-show')
      const pongSignal1 = emittedOnce(ipcMain, 'pong')
      w.loadFile(path.join(fixtures, 'pages', 'webview-visibilitychange.html'))
      await pongSignal1
      const pongSignal2 = emittedOnce(ipcMain, 'pong')
      await readyToShowSignal
      w.show()

      const [, visibilityState, hidden] = await pongSignal2
      assert(!hidden)
      assert.strictEqual(visibilityState, 'visible')
    })

    it('inherits the parent window visibility state and receives visibilitychange events', async () => {
      const w = await openTheWindow({ show: false })
      w.loadFile(path.join(fixtures, 'pages', 'webview-visibilitychange.html'))
      const [, visibilityState, hidden] = await emittedOnce(ipcMain, 'pong')
      assert.strictEqual(visibilityState, 'hidden')
      assert.strictEqual(hidden, true)

      // We have to start waiting for the event
      // before we ask the webContents to resize.
      const getResponse = emittedOnce(ipcMain, 'pong')
      w.webContents.emit('-window-visibility-change', 'visible')

      return getResponse.then(([, visibilityState, hidden]) => {
        assert.strictEqual(visibilityState, 'visible')
        assert.strictEqual(hidden, false)
      })
    })
  })

  describe('will-attach-webview event', () => {
    it('does not emit when src is not changed', (done) => {
      loadWebView(webview)
      setTimeout(() => {
        const expectedErrorMessage =
            'The WebView must be attached to the DOM ' +
            'and the dom-ready event emitted before this method can be called.'
        expect(() => { webview.stop() }).to.throw(expectedErrorMessage)
        done()
      })
    })

    it('supports changing the web preferences', async () => {
      ipcRenderer.send('disable-node-on-next-will-attach-webview')
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        nodeintegration: 'yes',
        src: `file://${fixtures}/pages/a.html`
      })

      const types = JSON.parse(message)
      expect(types).to.include({
        require: 'undefined',
        module: 'undefined',
        process: 'undefined',
        global: 'undefined'
      })
    })

    it('supports preventing a webview from being created', async () => {
      ipcRenderer.send('prevent-next-will-attach-webview')

      loadWebView(webview, {
        src: `file://${fixtures}/pages/c.html`
      })
      await waitForEvent(webview, 'destroyed')
    })

    it('supports removing the preload script', async () => {
      ipcRenderer.send('disable-preload-on-next-will-attach-webview')

      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        nodeintegration: 'yes',
        preload: path.join(fixtures, 'module', 'preload-set-global.js'),
        src: `file://${fixtures}/pages/a.html`
      })

      assert.strictEqual(message, 'undefined')
    })
  })

  describe('did-attach-webview event', () => {
    it('is emitted when a webview has been attached', async () => {
      const w = await openTheWindow({
        show: false,
        webPreferences: {
          webviewTag: true,
          nodeIntegration: true
        }
      })
      w.loadFile(path.join(fixtures, 'pages', 'webview-did-attach-event.html'))

      const [, webContents] = await emittedOnce(w.webContents, 'did-attach-webview')
      const [, id] = await emittedOnce(ipcMain, 'webview-dom-ready')
      expect(webContents.id).to.equal(id)
    })
  })

  it('loads devtools extensions registered on the parent window', async () => {
    const w = await openTheWindow({
      show: false,
      webPreferences: {
        webviewTag: true,
        nodeIntegration: true
      }
    })
    BrowserWindow.removeDevToolsExtension('foo')

    const extensionPath = path.join(__dirname, 'fixtures', 'devtools-extensions', 'foo')
    BrowserWindow.addDevToolsExtension(extensionPath)

    w.loadFile(path.join(fixtures, 'pages', 'webview-devtools.html'))

    const [, { runtimeId, tabId }] = await emittedOnce(ipcMain, 'answer')
    expect(runtimeId).to.equal('foo')
    expect(tabId).to.be.not.equal(w.webContents.id)
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

    it('emits resize events', async () => {
      const firstResizeSignal = waitForEvent(webview, 'resize')
      const domReadySignal = waitForEvent(webview, 'dom-ready')

      webview.src = `file://${fixtures}/pages/a.html`
      div.appendChild(webview)
      document.body.appendChild(div)

      const firstResizeEvent = await firstResizeSignal
      expect(firstResizeEvent.target).to.equal(webview)
      expect(firstResizeEvent.newWidth).to.equal(100)
      expect(firstResizeEvent.newHeight).to.equal(10)

      await domReadySignal

      const secondResizeSignal = waitForEvent(webview, 'resize')

      const newWidth = 1234
      const newHeight = 789
      div.style.width = `${newWidth}px`
      div.style.height = `${newHeight}px`

      const secondResizeEvent = await secondResizeSignal
      expect(secondResizeEvent.target).to.equal(webview)
      expect(secondResizeEvent.newWidth).to.equal(newWidth)
      expect(secondResizeEvent.newHeight).to.equal(newHeight)
    })

    it('emits focus event', async () => {
      const domReadySignal = waitForEvent(webview, 'dom-ready')
      webview.src = `file://${fixtures}/pages/a.html`
      document.body.appendChild(webview)

      await domReadySignal

      // If this test fails, check if webview.focus() still works.
      const focusSignal = waitForEvent(webview, 'focus')
      webview.focus()

      await focusSignal
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

    it('inherits the zoomFactor of the parent window', async () => {
      const w = await openTheWindow({
        show: false,
        webPreferences: {
          webviewTag: true,
          nodeIntegration: true,
          zoomFactor: 1.2
        }
      })
      w.loadFile(path.join(fixtures, 'pages', 'webview-zoom-factor.html'))

      const [, zoomFactor, zoomLevel] = await emittedOnce(ipcMain, 'webview-parent-zoom-level')
      expect(zoomFactor).to.equal(1.2)
      expect(zoomLevel).to.equal(1)
    })

    it('maintains zoom level on navigation', async () => {
      return openTheWindow({
        show: false,
        webPreferences: {
          webviewTag: true,
          nodeIntegration: true,
          zoomFactor: 1.2
        }
      }).then((w) => {
        const promise = new Promise((resolve) => {
          ipcMain.on('webview-zoom-level', (event, zoomLevel, zoomFactor, newHost, final) => {
            if (!newHost) {
              expect(zoomFactor).to.equal(1.44)
              expect(zoomLevel).to.equal(2.0)
            } else {
              expect(zoomFactor).to.equal(1.2)
              expect(zoomLevel).to.equal(1)
            }

            if (final) {
              resolve()
            }
          })
        })

        w.loadFile(path.join(fixtures, 'pages', 'webview-custom-zoom-level.html'))

        return promise
      })
    })

    it('maintains zoom level when navigating within same page', async () => {
      return openTheWindow({
        show: false,
        webPreferences: {
          webviewTag: true,
          nodeIntegration: true,
          zoomFactor: 1.2
        }
      }).then((w) => {
        const promise = new Promise((resolve) => {
          ipcMain.on('webview-zoom-in-page', (event, zoomLevel, zoomFactor, final) => {
            expect(zoomFactor).to.equal(1.44)
            expect(zoomLevel).to.equal(2.0)

            if (final) {
              resolve()
            }
          })
        })

        w.loadFile(path.join(fixtures, 'pages', 'webview-in-page-navigate.html'))

        return promise
      })
    })

    it('inherits zoom level for the origin when available', async () => {
      const w = await openTheWindow({
        show: false,
        webPreferences: {
          webviewTag: true,
          nodeIntegration: true,
          zoomFactor: 1.2
        }
      })
      w.loadFile(path.join(fixtures, 'pages', 'webview-origin-zoom-level.html'))

      const [, zoomLevel] = await emittedOnce(ipcMain, 'webview-origin-zoom-level')
      expect(zoomLevel).to.equal(2.0)
    })
  })

  describe('nativeWindowOpen option', () => {
    beforeEach(() => {
      webview.setAttribute('allowpopups', 'on')
      webview.setAttribute('nodeintegration', 'on')
      webview.setAttribute('webpreferences', 'nativeWindowOpen=1')
    })

    it('opens window of about:blank with cross-scripting enabled', async () => {
      // Don't wait for loading to finish.
      loadWebView(webview, {
        src: `file://${path.join(fixtures, 'api', 'native-window-open-blank.html')}`
      })

      const [, content] = await emittedOnce(ipcMain, 'answer')
      expect(content).to.equal('Hello')
    })

    it('opens window of same domain with cross-scripting enabled', async () => {
      // Don't wait for loading to finish.
      loadWebView(webview, {
        src: `file://${path.join(fixtures, 'api', 'native-window-open-file.html')}`
      })

      const [, content] = await emittedOnce(ipcMain, 'answer')
      expect(content).to.equal('Hello')
    })

    it('returns null from window.open when allowpopups is not set', async () => {
      webview.removeAttribute('allowpopups')

      // Don't wait for loading to finish.
      loadWebView(webview, {
        src: `file://${path.join(fixtures, 'api', 'native-window-open-no-allowpopups.html')}`
      })

      const [, { windowOpenReturnedNull }] = await emittedOnce(ipcMain, 'answer')
      expect(windowOpenReturnedNull).to.be.true()
    })

    it('blocks accessing cross-origin frames', async () => {
      // Don't wait for loading to finish.
      loadWebView(webview, {
        src: `file://${path.join(fixtures, 'api', 'native-window-open-cross-origin.html')}`
      })

      const [, content] = await emittedOnce(ipcMain, 'answer')
      const expectedContent =
          'Blocked a frame with origin "file://" from accessing a cross-origin frame.'

      expect(content).to.equal(expectedContent)
    })

    it('emits a new-window event', async () => {
      // Don't wait for loading to finish.
      loadWebView(webview, {
        src: `file://${fixtures}/pages/window-open.html`
      })
      const { url, frameName } = await waitForEvent(webview, 'new-window')

      expect(url).to.equal('http://host/')
      expect(frameName).to.equal('host')
    })

    it('emits a browser-window-created event', async () => {
      // Don't wait for loading to finish.
      loadWebView(webview, {
        src: `file://${fixtures}/pages/window-open.html`
      })

      await emittedOnce(app, 'browser-window-created')
    })

    it('emits a web-contents-created event', (done) => {
      app.on('web-contents-created', function listener (event, contents) {
        if (contents.getType() === 'window') {
          app.removeListener('web-contents-created', listener)
          done()
        }
      })
      loadWebView(webview, {
        src: `file://${fixtures}/pages/window-open.html`
      })
    })
  })
})

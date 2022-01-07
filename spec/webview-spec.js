const { expect } = require('chai');
const path = require('path');
const http = require('http');
const url = require('url');
const { ipcRenderer } = require('electron');
const { emittedOnce, waitForEvent } = require('./events-helpers');
const { ifdescribe, ifit, delay } = require('./spec-helpers');

const features = process._linkedBinding('electron_common_features');
const nativeModulesEnabled = process.env.ELECTRON_SKIP_NATIVE_MODULE_TESTS;

/* Most of the APIs here don't use standard callbacks */
/* eslint-disable standard/no-callback-literal */

describe('<webview> tag', function () {
  this.timeout(3 * 60 * 1000);

  const fixtures = path.join(__dirname, 'fixtures');
  let webview = null;

  const loadWebView = async (webview, attributes = {}) => {
    for (const [name, value] of Object.entries(attributes)) {
      webview.setAttribute(name, value);
    }
    document.body.appendChild(webview);
    await waitForEvent(webview, 'did-finish-load');
    return webview;
  };

  const startLoadingWebViewAndWaitForMessage = async (webview, attributes = {}) => {
    loadWebView(webview, attributes); // Don't wait for load to be finished.
    const event = await waitForEvent(webview, 'console-message');
    return event.message;
  };

  async function loadFileInWebView (webview, attributes = {}) {
    const thisFile = url.format({
      pathname: __filename.replace(/\\/g, '/'),
      protocol: 'file',
      slashes: true
    });
    const src = `<script>
      function loadFile() {
        return new Promise((resolve) => {
          fetch('${thisFile}').then(
            () => resolve('loaded'),
            () => resolve('failed')
          )
        });
      }
      console.log('ok');
    </script>`;
    attributes.src = `data:text/html;base64,${btoa(unescape(encodeURIComponent(src)))}`;
    await startLoadingWebViewAndWaitForMessage(webview, attributes);
    return await webview.executeJavaScript('loadFile()');
  }

  beforeEach(() => {
    webview = new WebView();
  });

  afterEach(() => {
    if (!document.body.contains(webview)) {
      document.body.appendChild(webview);
    }
    webview.remove();
  });

  describe('src attribute', () => {
    it('specifies the page to load', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        src: `file://${fixtures}/pages/a.html`
      });
      expect(message).to.equal('a');
    });

    it('navigates to new page when changed', async () => {
      await loadWebView(webview, {
        src: `file://${fixtures}/pages/a.html`
      });

      webview.src = `file://${fixtures}/pages/b.html`;

      const { message } = await waitForEvent(webview, 'console-message');
      expect(message).to.equal('b');
    });

    it('resolves relative URLs', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        src: '../fixtures/pages/e.html'
      });
      expect(message).to.equal('Window script is loaded before preload script');
    });

    it('ignores empty values', () => {
      expect(webview.src).to.equal('');

      for (const emptyValue of ['', null, undefined]) {
        webview.src = emptyValue;
        expect(webview.src).to.equal('');
      }
    });

    it('does not wait until loadURL is resolved', async () => {
      await loadWebView(webview, { src: 'about:blank' });

      const before = Date.now();
      webview.src = 'https://github.com';
      const now = Date.now();

      // Setting src is essentially sending a sync IPC message, which should
      // not exceed more than a few ms.
      //
      // This is for testing #18638.
      expect(now - before).to.be.below(100);
    });
  });

  describe('nodeintegration attribute', () => {
    it('inserts no node symbols when not set', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        src: `file://${fixtures}/pages/c.html`
      });

      const types = JSON.parse(message);
      expect(types).to.include({
        require: 'undefined',
        module: 'undefined',
        process: 'undefined',
        global: 'undefined'
      });
    });

    it('inserts node symbols when set', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        nodeintegration: 'on',
        webpreferences: 'contextIsolation=no',
        src: `file://${fixtures}/pages/d.html`
      });

      const types = JSON.parse(message);
      expect(types).to.include({
        require: 'function',
        module: 'object',
        process: 'object'
      });
    });

    it('loads node symbols after POST navigation when set', async function () {
      // FIXME Figure out why this is timing out on AppVeyor
      if (process.env.APPVEYOR === 'True') {
        this.skip();
        return;
      }

      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        nodeintegration: 'on',
        webpreferences: 'contextIsolation=no',
        src: `file://${fixtures}/pages/post.html`
      });

      const types = JSON.parse(message);
      expect(types).to.include({
        require: 'function',
        module: 'object',
        process: 'object'
      });
    });

    it('disables node integration on child windows when it is disabled on the webview', async () => {
      const src = url.format({
        pathname: `${fixtures}/pages/webview-opener-no-node-integration.html`,
        protocol: 'file',
        query: {
          p: `${fixtures}/pages/window-opener-node.html`
        },
        slashes: true
      });
      loadWebView(webview, {
        allowpopups: 'on',
        webpreferences: 'contextIsolation=no',
        src
      });
      const { message } = await waitForEvent(webview, 'console-message');
      expect(JSON.parse(message).isProcessGlobalUndefined).to.be.true();
    });

    (nativeModulesEnabled ? it : it.skip)('loads native modules when navigation happens', async function () {
      await loadWebView(webview, {
        nodeintegration: 'on',
        webpreferences: 'contextIsolation=no',
        src: `file://${fixtures}/pages/native-module.html`
      });

      webview.reload();

      const { message } = await waitForEvent(webview, 'console-message');
      expect(message).to.equal('function');
    });
  });

  describe('preload attribute', () => {
    it('loads the script before other scripts in window', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        preload: `${fixtures}/module/preload.js`,
        src: `file://${fixtures}/pages/e.html`,
        contextIsolation: false
      });

      expect(message).to.be.a('string');
      expect(message).to.be.not.equal('Window script is loaded before preload script');
    });

    it('preload script can still use "process" and "Buffer" when nodeintegration is off', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        preload: `${fixtures}/module/preload-node-off.js`,
        src: `file://${fixtures}/api/blank.html`
      });

      const types = JSON.parse(message);
      expect(types).to.include({
        process: 'object',
        Buffer: 'function'
      });
    });

    it('runs in the correct scope when sandboxed', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        preload: `${fixtures}/module/preload-context.js`,
        src: `file://${fixtures}/api/blank.html`,
        webpreferences: 'sandbox=yes'
      });

      const types = JSON.parse(message);
      expect(types).to.include({
        require: 'function', // arguments passed to it should be availale
        electron: 'undefined', // objects from the scope it is called from should not be available
        window: 'object', // the window object should be available
        localVar: 'undefined' // but local variables should not be exposed to the window
      });
    });

    it('preload script can require modules that still use "process" and "Buffer" when nodeintegration is off', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        preload: `${fixtures}/module/preload-node-off-wrapper.js`,
        src: `file://${fixtures}/api/blank.html`
      });

      const types = JSON.parse(message);
      expect(types).to.include({
        process: 'object',
        Buffer: 'function'
      });
    });

    it('receives ipc message in preload script', async () => {
      await loadWebView(webview, {
        preload: `${fixtures}/module/preload-ipc.js`,
        src: `file://${fixtures}/pages/e.html`
      });

      const message = 'boom!';
      webview.send('ping', message);

      const { channel, args } = await waitForEvent(webview, 'ipc-message');
      expect(channel).to.equal('pong');
      expect(args).to.deep.equal([message]);
    });

    it('<webview>.sendToFrame()', async () => {
      loadWebView(webview, {
        nodeintegration: 'on',
        webpreferences: 'contextIsolation=no',
        preload: `${fixtures}/module/preload-ipc.js`,
        src: `file://${fixtures}/pages/ipc-message.html`
      });

      const { frameId } = await waitForEvent(webview, 'ipc-message');

      const message = 'boom!';
      webview.sendToFrame(frameId, 'ping', message);

      const { channel, args } = await waitForEvent(webview, 'ipc-message');
      expect(channel).to.equal('pong');
      expect(args).to.deep.equal([message]);
    });

    it('works without script tag in page', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        preload: `${fixtures}/module/preload.js`,
        src: `file://${fixtures}pages/base-page.html`
      });

      const types = JSON.parse(message);
      expect(types).to.include({
        require: 'function',
        module: 'object',
        process: 'object',
        Buffer: 'function'
      });
    });

    it('resolves relative URLs', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        preload: '../fixtures/module/preload.js',
        src: `file://${fixtures}/pages/e.html`
      });

      const types = JSON.parse(message);
      expect(types).to.include({
        require: 'function',
        module: 'object',
        process: 'object',
        Buffer: 'function'
      });
    });

    it('ignores empty values', () => {
      expect(webview.preload).to.equal('');

      for (const emptyValue of ['', null, undefined]) {
        webview.preload = emptyValue;
        expect(webview.preload).to.equal('');
      }
    });
  });

  describe('httpreferrer attribute', () => {
    it('sets the referrer url', (done) => {
      const referrer = 'http://github.com/';
      const server = http.createServer((req, res) => {
        try {
          expect(req.headers.referer).to.equal(referrer);
          done();
        } catch (e) {
          done(e);
        } finally {
          res.end();
          server.close();
        }
      }).listen(0, '127.0.0.1', () => {
        const port = server.address().port;
        loadWebView(webview, {
          httpreferrer: referrer,
          src: `http://127.0.0.1:${port}`
        });
      });
    });
  });

  describe('useragent attribute', () => {
    it('sets the user agent', async () => {
      const referrer = 'Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; AS; rv:11.0) like Gecko';
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        src: `file://${fixtures}/pages/useragent.html`,
        useragent: referrer
      });
      expect(message).to.equal(referrer);
    });
  });

  describe('disablewebsecurity attribute', () => {
    it('does not disable web security when not set', async () => {
      const result = await loadFileInWebView(webview);
      expect(result).to.equal('failed');
    });

    it('disables web security when set', async () => {
      const result = await loadFileInWebView(webview, { disablewebsecurity: '' });
      expect(result).to.equal('loaded');
    });

    it('does not break node integration', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        disablewebsecurity: '',
        nodeintegration: 'on',
        webpreferences: 'contextIsolation=no',
        src: `file://${fixtures}/pages/d.html`
      });

      const types = JSON.parse(message);
      expect(types).to.include({
        require: 'function',
        module: 'object',
        process: 'object'
      });
    });

    it('does not break preload script', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        disablewebsecurity: '',
        preload: `${fixtures}/module/preload.js`,
        src: `file://${fixtures}/pages/e.html`
      });

      const types = JSON.parse(message);
      expect(types).to.include({
        require: 'function',
        module: 'object',
        process: 'object',
        Buffer: 'function'
      });
    });
  });

  describe('partition attribute', () => {
    it('inserts no node symbols when not set', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        partition: 'test1',
        src: `file://${fixtures}/pages/c.html`
      });

      const types = JSON.parse(message);
      expect(types).to.include({
        require: 'undefined',
        module: 'undefined',
        process: 'undefined',
        global: 'undefined'
      });
    });

    it('inserts node symbols when set', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        nodeintegration: 'on',
        partition: 'test2',
        webpreferences: 'contextIsolation=no',
        src: `file://${fixtures}/pages/d.html`
      });

      const types = JSON.parse(message);
      expect(types).to.include({
        require: 'function',
        module: 'object',
        process: 'object'
      });
    });

    it('isolates storage for different id', async () => {
      window.localStorage.setItem('test', 'one');

      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        partition: 'test3',
        src: `file://${fixtures}/pages/partition/one.html`
      });

      const parsedMessage = JSON.parse(message);
      expect(parsedMessage).to.include({
        numberOfEntries: 0,
        testValue: null
      });
    });

    it('uses current session storage when no id is provided', async () => {
      const testValue = 'one';
      window.localStorage.setItem('test', testValue);

      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        src: `file://${fixtures}/pages/partition/one.html`
      });

      const parsedMessage = JSON.parse(message);
      expect(parsedMessage).to.include({
        numberOfEntries: 1,
        testValue
      });
    });
  });

  describe('allowpopups attribute', () => {
    const generateSpecs = (description, webpreferences = '') => {
      describe(description, () => {
        it('can not open new window when not set', async () => {
          const message = await startLoadingWebViewAndWaitForMessage(webview, {
            webpreferences,
            src: `file://${fixtures}/pages/window-open-hide.html`
          });
          expect(message).to.equal('null');
        });

        it('can open new window when set', async () => {
          const message = await startLoadingWebViewAndWaitForMessage(webview, {
            webpreferences,
            allowpopups: 'on',
            src: `file://${fixtures}/pages/window-open-hide.html`
          });
          expect(message).to.equal('window');
        });
      });
    };

    generateSpecs('without sandbox');
    generateSpecs('with sandbox', 'sandbox=yes');
  });

  describe('webpreferences attribute', () => {
    it('can enable nodeintegration', async () => {
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        src: `file://${fixtures}/pages/d.html`,
        webpreferences: 'nodeIntegration,contextIsolation=no'
      });

      const types = JSON.parse(message);
      expect(types).to.include({
        require: 'function',
        module: 'object',
        process: 'object'
      });
    });

    it('can disables web security and enable nodeintegration', async () => {
      const result = await loadFileInWebView(webview, { webpreferences: 'webSecurity=no, nodeIntegration=yes, contextIsolation=no' });
      expect(result).to.equal('loaded');
      const type = await webview.executeJavaScript('typeof require');
      expect(type).to.equal('function');
    });
  });

  describe('new-window event', () => {
    it('emits when window.open is called', async () => {
      loadWebView(webview, {
        src: `file://${fixtures}/pages/window-open.html`,
        allowpopups: true
      });
      const { url, frameName } = await waitForEvent(webview, 'new-window');

      expect(url).to.equal('http://host/');
      expect(frameName).to.equal('host');
    });

    it('emits when link with target is called', async () => {
      loadWebView(webview, {
        src: `file://${fixtures}/pages/target-name.html`,
        allowpopups: true
      });
      const { url, frameName } = await waitForEvent(webview, 'new-window');

      expect(url).to.equal('http://host/');
      expect(frameName).to.equal('target');
    });
  });

  describe('ipc-message event', () => {
    it('emits when guest sends an ipc message to browser', async () => {
      loadWebView(webview, {
        nodeintegration: 'on',
        webpreferences: 'contextIsolation=no',
        src: `file://${fixtures}/pages/ipc-message.html`
      });
      const { frameId, channel, args } = await waitForEvent(webview, 'ipc-message');

      expect(frameId).to.be.an('array').that.has.lengthOf(2);
      expect(channel).to.equal('channel');
      expect(args).to.deep.equal(['arg1', 'arg2']);
    });
  });

  describe('page-title-set event', () => {
    it('emits when title is set', async () => {
      loadWebView(webview, {
        src: `file://${fixtures}/pages/a.html`
      });
      const { title, explicitSet } = await waitForEvent(webview, 'page-title-set');

      expect(title).to.equal('test');
      expect(explicitSet).to.be.true();
    });
  });

  describe('page-favicon-updated event', () => {
    it('emits when favicon urls are received', async () => {
      loadWebView(webview, {
        src: `file://${fixtures}/pages/a.html`
      });
      const { favicons } = await waitForEvent(webview, 'page-favicon-updated');

      expect(favicons).to.be.an('array').of.length(2);
      if (process.platform === 'win32') {
        expect(favicons[0]).to.match(/^file:\/\/\/[A-Z]:\/favicon.png$/i);
      } else {
        expect(favicons[0]).to.equal('file:///favicon.png');
      }
    });
  });

  describe('did-redirect-navigation event', () => {
    let server = null;
    let uri = null;

    before((done) => {
      server = http.createServer((req, res) => {
        if (req.url === '/302') {
          res.setHeader('Location', '/200');
          res.statusCode = 302;
          res.end();
        } else {
          res.end();
        }
      });
      server.listen(0, '127.0.0.1', () => {
        uri = `http://127.0.0.1:${(server.address()).port}`;
        done();
      });
    });

    after(() => {
      server.close();
    });

    it('is emitted on redirects', async () => {
      loadWebView(webview, {
        src: `${uri}/302`
      });

      const event = await waitForEvent(webview, 'did-redirect-navigation');

      expect(event.url).to.equal(`${uri}/200`);
      expect(event.isInPlace).to.be.false();
      expect(event.isMainFrame).to.be.true();
      expect(event.frameProcessId).to.be.a('number');
      expect(event.frameRoutingId).to.be.a('number');
    });
  });

  describe('will-navigate event', () => {
    it('emits when a url that leads to oustide of the page is clicked', async () => {
      loadWebView(webview, {
        src: `file://${fixtures}/pages/webview-will-navigate.html`
      });
      const { url } = await waitForEvent(webview, 'will-navigate');

      expect(url).to.equal('http://host/');
    });
  });

  describe('did-navigate event', () => {
    let p = path.join(fixtures, 'pages', 'webview-will-navigate.html');
    p = p.replace(/\\/g, '/');
    const pageUrl = url.format({
      protocol: 'file',
      slashes: true,
      pathname: p
    });

    it('emits when a url that leads to outside of the page is clicked', async () => {
      loadWebView(webview, { src: pageUrl });
      const { url } = await waitForEvent(webview, 'did-navigate');

      expect(url).to.equal(pageUrl);
    });
  });

  describe('did-navigate-in-page event', () => {
    it('emits when an anchor link is clicked', async () => {
      let p = path.join(fixtures, 'pages', 'webview-did-navigate-in-page.html');
      p = p.replace(/\\/g, '/');
      const pageUrl = url.format({
        protocol: 'file',
        slashes: true,
        pathname: p
      });
      loadWebView(webview, { src: pageUrl });
      const event = await waitForEvent(webview, 'did-navigate-in-page');
      expect(event.url).to.equal(`${pageUrl}#test_content`);
    });

    it('emits when window.history.replaceState is called', async () => {
      loadWebView(webview, {
        src: `file://${fixtures}/pages/webview-did-navigate-in-page-with-history.html`
      });
      const { url } = await waitForEvent(webview, 'did-navigate-in-page');
      expect(url).to.equal('http://host/');
    });

    it('emits when window.location.hash is changed', async () => {
      let p = path.join(fixtures, 'pages', 'webview-did-navigate-in-page-with-hash.html');
      p = p.replace(/\\/g, '/');
      const pageUrl = url.format({
        protocol: 'file',
        slashes: true,
        pathname: p
      });
      loadWebView(webview, { src: pageUrl });
      const event = await waitForEvent(webview, 'did-navigate-in-page');
      expect(event.url).to.equal(`${pageUrl}#test`);
    });
  });

  describe('close event', () => {
    it('should fire when interior page calls window.close', async () => {
      loadWebView(webview, { src: `file://${fixtures}/pages/close.html` });
      await waitForEvent(webview, 'close');
    });
  });

  // FIXME(zcbenz): Disabled because of moving to OOPIF webview.
  xdescribe('setDevToolsWebContents() API', () => {
    it('sets webContents of webview as devtools', async () => {
      const webview2 = new WebView();
      loadWebView(webview2);

      // Setup an event handler for further usage.
      const waitForDomReady = waitForEvent(webview2, 'dom-ready');

      loadWebView(webview, { src: 'about:blank' });
      await waitForEvent(webview, 'dom-ready');
      webview.getWebContents().setDevToolsWebContents(webview2.getWebContents());
      webview.getWebContents().openDevTools();

      await waitForDomReady;

      // Its WebContents should be a DevTools.
      const devtools = webview2.getWebContents();
      expect(devtools.getURL().startsWith('devtools://devtools')).to.be.true();

      const name = await devtools.executeJavaScript('InspectorFrontendHost.constructor.name');
      document.body.removeChild(webview2);

      expect(name).to.be.equal('InspectorFrontendHostImpl');
    });
  });

  describe('devtools-opened event', () => {
    it('should fire when webview.openDevTools() is called', async () => {
      loadWebView(webview, {
        src: `file://${fixtures}/pages/base-page.html`
      });
      await waitForEvent(webview, 'dom-ready');

      webview.openDevTools();
      await waitForEvent(webview, 'devtools-opened');

      webview.closeDevTools();
    });
  });

  describe('devtools-closed event', () => {
    it('should fire when webview.closeDevTools() is called', async () => {
      loadWebView(webview, {
        src: `file://${fixtures}/pages/base-page.html`
      });
      await waitForEvent(webview, 'dom-ready');

      webview.openDevTools();
      await waitForEvent(webview, 'devtools-opened');

      webview.closeDevTools();
      await waitForEvent(webview, 'devtools-closed');
    });
  });

  describe('devtools-focused event', () => {
    it('should fire when webview.openDevTools() is called', async () => {
      loadWebView(webview, {
        src: `file://${fixtures}/pages/base-page.html`
      });

      const waitForDevToolsFocused = waitForEvent(webview, 'devtools-focused');

      await waitForEvent(webview, 'dom-ready');
      webview.openDevTools();

      await waitForDevToolsFocused;
      webview.closeDevTools();
    });
  });

  describe('<webview>.reload()', () => {
    it('should emit beforeunload handler', async () => {
      await loadWebView(webview, {
        nodeintegration: 'on',
        webpreferences: 'contextIsolation=no',
        src: `file://${fixtures}/pages/beforeunload-false.html`
      });

      // Event handler has to be added before reload.
      const waitForOnbeforeunload = waitForEvent(webview, 'ipc-message');

      webview.reload();

      const { channel } = await waitForOnbeforeunload;
      expect(channel).to.equal('onbeforeunload');
    });
  });

  describe('<webview>.goForward()', () => {
    it('should work after a replaced history entry', (done) => {
      let loadCount = 1;
      const listener = (e) => {
        if (loadCount === 1) {
          expect(e.channel).to.equal('history');
          expect(e.args[0]).to.equal(1);
          expect(webview.canGoBack()).to.be.false();
          expect(webview.canGoForward()).to.be.false();
        } else if (loadCount === 2) {
          expect(e.channel).to.equal('history');
          expect(e.args[0]).to.equal(2);
          expect(webview.canGoBack()).to.be.false();
          expect(webview.canGoForward()).to.be.true();
          webview.removeEventListener('ipc-message', listener);
        }
      };

      const loadListener = () => {
        try {
          if (loadCount === 1) {
            webview.src = `file://${fixtures}/pages/base-page.html`;
          } else if (loadCount === 2) {
            expect(webview.canGoBack()).to.be.true();
            expect(webview.canGoForward()).to.be.false();

            webview.goBack();
          } else if (loadCount === 3) {
            webview.goForward();
          } else if (loadCount === 4) {
            expect(webview.canGoBack()).to.be.true();
            expect(webview.canGoForward()).to.be.false();

            webview.removeEventListener('did-finish-load', loadListener);
            done();
          }

          loadCount += 1;
        } catch (e) {
          done(e);
        }
      };

      webview.addEventListener('ipc-message', listener);
      webview.addEventListener('did-finish-load', loadListener);

      loadWebView(webview, {
        nodeintegration: 'on',
        src: `file://${fixtures}/pages/history-replace.html`
      });
    });
  });

  // FIXME: https://github.com/electron/electron/issues/19397
  xdescribe('<webview>.clearHistory()', () => {
    it('should clear the navigation history', async () => {
      const message = waitForEvent(webview, 'ipc-message');
      await loadWebView(webview, {
        nodeintegration: 'on',
        src: `file://${fixtures}/pages/history.html`
      });
      const event = await message;

      expect(event.channel).to.equal('history');
      expect(event.args[0]).to.equal(2);
      expect(webview.canGoBack()).to.be.true();

      webview.clearHistory();
      expect(webview.canGoBack()).to.be.false();
    });
  });

  describe('basic auth', () => {
    const auth = require('basic-auth');

    it('should authenticate with correct credentials', (done) => {
      const message = 'Authenticated';
      const server = http.createServer((req, res) => {
        const credentials = auth(req);
        if (credentials.name === 'test' && credentials.pass === 'test') {
          res.end(message);
        } else {
          res.end('failed');
        }
        server.close();
      });
      server.listen(0, '127.0.0.1', () => {
        const port = server.address().port;
        webview.addEventListener('ipc-message', (e) => {
          try {
            expect(e.channel).to.equal(message);
            done();
          } catch (e) {
            done(e);
          }
        });
        loadWebView(webview, {
          nodeintegration: 'on',
          webpreferences: 'contextIsolation=no',
          src: `file://${fixtures}/pages/basic-auth.html?port=${port}`
        });
      });
    });
  });

  describe('dom-ready event', () => {
    it('emits when document is loaded', (done) => {
      const server = http.createServer(() => {});
      server.listen(0, '127.0.0.1', () => {
        const port = server.address().port;
        webview.addEventListener('dom-ready', () => {
          done();
        });
        loadWebView(webview, {
          src: `file://${fixtures}/pages/dom-ready.html?port=${port}`
        });
      });
    });

    it('throws a custom error when an API method is called before the event is emitted', () => {
      const expectedErrorMessage =
          'The WebView must be attached to the DOM ' +
          'and the dom-ready event emitted before this method can be called.';
      expect(() => { webview.stop(); }).to.throw(expectedErrorMessage);
    });
  });

  describe('executeJavaScript', () => {
    it('should support user gesture', async () => {
      await loadWebView(webview, {
        src: `file://${fixtures}/pages/fullscreen.html`
      });

      // Event handler has to be added before js execution.
      const waitForEnterHtmlFullScreen = waitForEvent(webview, 'enter-html-full-screen');

      const jsScript = "document.querySelector('video').webkitRequestFullscreen()";
      webview.executeJavaScript(jsScript, true);

      return waitForEnterHtmlFullScreen;
    });

    it('can return the result of the executed script', async () => {
      await loadWebView(webview, {
        src: 'about:blank'
      });

      const jsScript = "'4'+2";
      const expectedResult = '42';

      const result = await webview.executeJavaScript(jsScript);
      expect(result).to.equal(expectedResult);
    });
  });

  it('supports inserting CSS', async () => {
    await loadWebView(webview, { src: `file://${fixtures}/pages/base-page.html` });
    await webview.insertCSS('body { background-repeat: round; }');
    const result = await webview.executeJavaScript('window.getComputedStyle(document.body).getPropertyValue("background-repeat")');
    expect(result).to.equal('round');
  });

  it('supports removing inserted CSS', async () => {
    await loadWebView(webview, { src: `file://${fixtures}/pages/base-page.html` });
    const key = await webview.insertCSS('body { background-repeat: round; }');
    await webview.removeInsertedCSS(key);
    const result = await webview.executeJavaScript('window.getComputedStyle(document.body).getPropertyValue("background-repeat")');
    expect(result).to.equal('repeat');
  });

  describe('sendInputEvent', () => {
    it('can send keyboard event', async () => {
      loadWebView(webview, {
        nodeintegration: 'on',
        webpreferences: 'contextIsolation=no',
        src: `file://${fixtures}/pages/onkeyup.html`
      });
      await waitForEvent(webview, 'dom-ready');

      const waitForIpcMessage = waitForEvent(webview, 'ipc-message');
      webview.sendInputEvent({
        type: 'keyup',
        keyCode: 'c',
        modifiers: ['shift']
      });

      const { channel, args } = await waitForIpcMessage;
      expect(channel).to.equal('keyup');
      expect(args).to.deep.equal(['C', 'KeyC', 67, true, false]);
    });

    it('can send mouse event', async () => {
      loadWebView(webview, {
        nodeintegration: 'on',
        webpreferences: 'contextIsolation=no',
        src: `file://${fixtures}/pages/onmouseup.html`
      });
      await waitForEvent(webview, 'dom-ready');

      const waitForIpcMessage = waitForEvent(webview, 'ipc-message');
      webview.sendInputEvent({
        type: 'mouseup',
        modifiers: ['ctrl'],
        x: 10,
        y: 20
      });

      const { channel, args } = await waitForIpcMessage;
      expect(channel).to.equal('mouseup');
      expect(args).to.deep.equal([10, 20, false, true]);
    });
  });

  describe('context-menu event', () => {
    it('emits when right-clicked in page', async () => {
      await loadWebView(webview, { src: 'about:blank' });

      const promise = waitForEvent(webview, 'context-menu');

      // Simulate right-click to create context-menu event.
      const opts = { x: 0, y: 0, button: 'right' };
      webview.sendInputEvent({ ...opts, type: 'mouseDown' });
      webview.sendInputEvent({ ...opts, type: 'mouseUp' });

      const { params } = await promise;

      expect(params.pageURL).to.equal(webview.getURL());
      expect(params.frame).to.be.undefined();
      expect(params.x).to.be.a('number');
      expect(params.y).to.be.a('number');
    });
  });

  describe('media-started-playing media-paused events', () => {
    beforeEach(function () {
      if (!document.createElement('audio').canPlayType('audio/wav')) {
        this.skip();
      }
    });

    it('emits when audio starts and stops playing', async () => {
      await loadWebView(webview, { src: `file://${fixtures}/pages/base-page.html` });

      // With the new autoplay policy, audio elements must be unmuted
      // see https://goo.gl/xX8pDD.
      const source = `
        const audio = document.createElement("audio")
        audio.src = "../assets/tone.wav"
        document.body.appendChild(audio);
        audio.play()
      `;
      webview.executeJavaScript(source, true);
      await waitForEvent(webview, 'media-started-playing');

      webview.executeJavaScript('document.querySelector("audio").pause()', true);
      await waitForEvent(webview, 'media-paused');
    });
  });

  describe('found-in-page event', () => {
    it('emits when a request is made', async () => {
      const didFinishLoad = waitForEvent(webview, 'did-finish-load');
      loadWebView(webview, { src: `file://${fixtures}/pages/content.html` });
      // TODO(deepak1556): With https://codereview.chromium.org/2836973002
      // focus of the webContents is required when triggering the api.
      // Remove this workaround after determining the cause for
      // incorrect focus.
      webview.focus();
      await didFinishLoad;

      const activeMatchOrdinal = [];

      for (;;) {
        const foundInPage = waitForEvent(webview, 'found-in-page');
        const requestId = webview.findInPage('virtual');
        const event = await foundInPage;

        expect(event.result.requestId).to.equal(requestId);
        expect(event.result.matches).to.equal(3);

        activeMatchOrdinal.push(event.result.activeMatchOrdinal);

        if (event.result.activeMatchOrdinal === event.result.matches) {
          break;
        }
      }

      expect(activeMatchOrdinal).to.deep.equal([1, 2, 3]);
      webview.stopFindInPage('clearSelection');
    });
  });

  describe('<webview>.getWebContentsId', () => {
    it('can return the WebContents ID', async () => {
      const src = 'about:blank';
      await loadWebView(webview, { src });

      expect(webview.getWebContentsId()).to.be.a('number');
    });
  });

  describe('<webview>.capturePage()', () => {
    before(function () {
      // TODO(miniak): figure out why this is failing on windows
      if (process.platform === 'win32') {
        this.skip();
      }
    });

    it('returns a Promise with a NativeImage', async () => {
      const src = 'data:text/html,%3Ch1%3EHello%2C%20World!%3C%2Fh1%3E';
      await loadWebView(webview, { src });

      const image = await webview.capturePage();
      const imgBuffer = image.toPNG();

      // Check the 25th byte in the PNG.
      // Values can be 0,2,3,4, or 6. We want 6, which is RGB + Alpha
      expect(imgBuffer[25]).to.equal(6);
    });
  });

  ifdescribe(features.isPrintingEnabled())('<webview>.printToPDF()', () => {
    it('rejects on incorrectly typed parameters', async () => {
      const badTypes = {
        marginsType: 'terrible',
        scaleFactor: 'not-a-number',
        landscape: [],
        pageRanges: { oops: 'im-not-the-right-key' },
        headerFooter: '123',
        printSelectionOnly: 1,
        printBackground: 2,
        pageSize: 'IAmAPageSize'
      };

      // These will hard crash in Chromium unless we type-check
      for (const [key, value] of Object.entries(badTypes)) {
        const param = { [key]: value };

        const src = 'data:text/html,%3Ch1%3EHello%2C%20World!%3C%2Fh1%3E';
        await loadWebView(webview, { src });
        await expect(webview.printToPDF(param)).to.eventually.be.rejected();
      }
    });

    it('can print to PDF', async () => {
      const src = 'data:text/html,%3Ch1%3EHello%2C%20World!%3C%2Fh1%3E';
      await loadWebView(webview, { src });

      const data = await webview.printToPDF({});
      expect(data).to.be.an.instanceof(Uint8Array).that.is.not.empty();
    });
  });

  describe('will-attach-webview event', () => {
    it('does not emit when src is not changed', async () => {
      console.log('loadWebView(webview)');
      loadWebView(webview);
      await delay();
      const expectedErrorMessage =
          'The WebView must be attached to the DOM ' +
          'and the dom-ready event emitted before this method can be called.';
      expect(() => { webview.stop(); }).to.throw(expectedErrorMessage);
    });

    it('supports changing the web preferences', async () => {
      ipcRenderer.send('disable-node-on-next-will-attach-webview');
      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        nodeintegration: 'yes',
        src: `file://${fixtures}/pages/a.html`
      });

      const types = JSON.parse(message);
      expect(types).to.include({
        require: 'undefined',
        module: 'undefined',
        process: 'undefined',
        global: 'undefined'
      });
    });

    it('handler modifying params.instanceId does not break <webview>', async () => {
      ipcRenderer.send('break-next-will-attach-webview');

      await startLoadingWebViewAndWaitForMessage(webview, {
        src: `file://${fixtures}/pages/a.html`
      });
    });

    it('supports preventing a webview from being created', async () => {
      ipcRenderer.send('prevent-next-will-attach-webview');

      loadWebView(webview, {
        src: `file://${fixtures}/pages/c.html`
      });
      await waitForEvent(webview, 'destroyed');
    });

    it('supports removing the preload script', async () => {
      ipcRenderer.send('disable-preload-on-next-will-attach-webview');

      const message = await startLoadingWebViewAndWaitForMessage(webview, {
        nodeintegration: 'yes',
        preload: path.join(fixtures, 'module', 'preload-set-global.js'),
        src: `file://${fixtures}/pages/a.html`
      });

      expect(message).to.equal('undefined');
    });
  });

  describe('DOM events', () => {
    let div;

    beforeEach(() => {
      div = document.createElement('div');
      div.style.width = '100px';
      div.style.height = '10px';
      div.style.overflow = 'hidden';
      webview.style.height = '100%';
      webview.style.width = '100%';
    });

    afterEach(() => {
      if (div != null) div.remove();
    });

    const generateSpecs = (description, sandbox) => {
      describe(description, () => {
        // TODO(nornagon): disabled during chromium roll 2019-06-11 due to a
        // 'ResizeObserver loop limit exceeded' error on Windows
        xit('emits resize events', async () => {
          const firstResizeSignal = waitForEvent(webview, 'resize');
          const domReadySignal = waitForEvent(webview, 'dom-ready');

          webview.src = `file://${fixtures}/pages/a.html`;
          webview.webpreferences = `sandbox=${sandbox ? 'yes' : 'no'}`;
          div.appendChild(webview);
          document.body.appendChild(div);

          const firstResizeEvent = await firstResizeSignal;
          expect(firstResizeEvent.target).to.equal(webview);
          expect(firstResizeEvent.newWidth).to.equal(100);
          expect(firstResizeEvent.newHeight).to.equal(10);

          await domReadySignal;

          const secondResizeSignal = waitForEvent(webview, 'resize');

          const newWidth = 1234;
          const newHeight = 789;
          div.style.width = `${newWidth}px`;
          div.style.height = `${newHeight}px`;

          const secondResizeEvent = await secondResizeSignal;
          expect(secondResizeEvent.target).to.equal(webview);
          expect(secondResizeEvent.newWidth).to.equal(newWidth);
          expect(secondResizeEvent.newHeight).to.equal(newHeight);
        });

        it('emits focus event', async () => {
          const domReadySignal = waitForEvent(webview, 'dom-ready');
          webview.src = `file://${fixtures}/pages/a.html`;
          webview.webpreferences = `sandbox=${sandbox ? 'yes' : 'no'}`;
          document.body.appendChild(webview);

          await domReadySignal;

          // If this test fails, check if webview.focus() still works.
          const focusSignal = waitForEvent(webview, 'focus');
          webview.focus();

          await focusSignal;
        });
      });
    };

    generateSpecs('without sandbox', false);
    generateSpecs('with sandbox', true);
  });
});

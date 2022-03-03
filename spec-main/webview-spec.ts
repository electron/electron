import * as path from 'path';
import * as url from 'url';
import { BrowserWindow, session, ipcMain, app, WebContents } from 'electron/main';
import { closeAllWindows } from './window-helpers';
import { emittedOnce, emittedUntil } from './events-helpers';
import { ifit, delay } from './spec-helpers';
import { expect } from 'chai';

async function loadWebView (w: WebContents, attributes: Record<string, string>, openDevTools: boolean = false): Promise<void> {
  await w.executeJavaScript(`
    new Promise((resolve, reject) => {
      const webview = new WebView()
      for (const [k, v] of Object.entries(${JSON.stringify(attributes)})) {
        webview.setAttribute(k, v)
      }
      document.body.appendChild(webview)
      webview.addEventListener('dom-ready', () => {
        if (${openDevTools}) {
          webview.openDevTools()
        }
      })
      webview.addEventListener('did-finish-load', () => {
        resolve()
      })
    })
  `);
}

describe('<webview> tag', function () {
  const fixtures = path.join(__dirname, '..', 'spec', 'fixtures');

  afterEach(closeAllWindows);

  function hideChildWindows (e: any, wc: WebContents) {
    wc.on('new-window', (event, url, frameName, disposition, options) => {
      options.show = false;
    });
  }

  before(() => {
    app.on('web-contents-created', hideChildWindows);
  });

  after(() => {
    app.off('web-contents-created', hideChildWindows);
  });

  it('works without script tag in page', async () => {
    const w = new BrowserWindow({
      show: false,
      webPreferences: {
        webviewTag: true,
        nodeIntegration: true,
        contextIsolation: false
      }
    });
    w.loadFile(path.join(fixtures, 'pages', 'webview-no-script.html'));
    await emittedOnce(ipcMain, 'pong');
  });

  it('works with sandbox', async () => {
    const w = new BrowserWindow({
      show: false,
      webPreferences: {
        webviewTag: true,
        sandbox: true
      }
    });
    w.loadFile(path.join(fixtures, 'pages', 'webview-isolated.html'));
    await emittedOnce(ipcMain, 'pong');
  });

  it('works with contextIsolation', async () => {
    const w = new BrowserWindow({
      show: false,
      webPreferences: {
        webviewTag: true,
        contextIsolation: true
      }
    });
    w.loadFile(path.join(fixtures, 'pages', 'webview-isolated.html'));
    await emittedOnce(ipcMain, 'pong');
  });

  it('works with contextIsolation + sandbox', async () => {
    const w = new BrowserWindow({
      show: false,
      webPreferences: {
        webviewTag: true,
        contextIsolation: true,
        sandbox: true
      }
    });
    w.loadFile(path.join(fixtures, 'pages', 'webview-isolated.html'));
    await emittedOnce(ipcMain, 'pong');
  });

  it('works with Trusted Types', async () => {
    const w = new BrowserWindow({
      show: false,
      webPreferences: {
        webviewTag: true
      }
    });
    w.loadFile(path.join(fixtures, 'pages', 'webview-trusted-types.html'));
    await emittedOnce(ipcMain, 'pong');
  });

  it('is disabled by default', async () => {
    const w = new BrowserWindow({
      show: false,
      webPreferences: {
        preload: path.join(fixtures, 'module', 'preload-webview.js'),
        nodeIntegration: true
      }
    });

    const webview = emittedOnce(ipcMain, 'webview');
    w.loadFile(path.join(fixtures, 'pages', 'webview-no-script.html'));
    const [, type] = await webview;

    expect(type).to.equal('undefined', 'WebView still exists');
  });

  // FIXME(deepak1556): Ch69 follow up.
  xdescribe('document.visibilityState/hidden', () => {
    afterEach(() => {
      ipcMain.removeAllListeners('pong');
    });

    it('updates when the window is shown after the ready-to-show event', async () => {
      const w = new BrowserWindow({ show: false });
      const readyToShowSignal = emittedOnce(w, 'ready-to-show');
      const pongSignal1 = emittedOnce(ipcMain, 'pong');
      w.loadFile(path.join(fixtures, 'pages', 'webview-visibilitychange.html'));
      await pongSignal1;
      const pongSignal2 = emittedOnce(ipcMain, 'pong');
      await readyToShowSignal;
      w.show();

      const [, visibilityState, hidden] = await pongSignal2;
      expect(visibilityState).to.equal('visible');
      expect(hidden).to.be.false();
    });

    it('inherits the parent window visibility state and receives visibilitychange events', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadFile(path.join(fixtures, 'pages', 'webview-visibilitychange.html'));
      const [, visibilityState, hidden] = await emittedOnce(ipcMain, 'pong');
      expect(visibilityState).to.equal('hidden');
      expect(hidden).to.be.true();

      // We have to start waiting for the event
      // before we ask the webContents to resize.
      const getResponse = emittedOnce(ipcMain, 'pong');
      w.webContents.emit('-window-visibility-change', 'visible');

      return getResponse.then(([, visibilityState, hidden]) => {
        expect(visibilityState).to.equal('visible');
        expect(hidden).to.be.false();
      });
    });
  });

  describe('did-attach-webview event', () => {
    it('is emitted when a webview has been attached', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          webviewTag: true,
          nodeIntegration: true,
          contextIsolation: false
        }
      });
      const didAttachWebview = emittedOnce(w.webContents, 'did-attach-webview');
      const webviewDomReady = emittedOnce(ipcMain, 'webview-dom-ready');
      w.loadFile(path.join(fixtures, 'pages', 'webview-did-attach-event.html'));

      const [, webContents] = await didAttachWebview;
      const [, id] = await webviewDomReady;
      expect(webContents.id).to.equal(id);
    });
  });

  describe('did-attach event', () => {
    it('is emitted when a webview has been attached', async () => {
      const w = new BrowserWindow({
        webPreferences: {
          webviewTag: true
        }
      });
      await w.loadURL('about:blank');
      const message = await w.webContents.executeJavaScript(`new Promise((resolve, reject) => {
        const webview = new WebView()
        webview.setAttribute('src', 'about:blank')
        webview.addEventListener('did-attach', (e) => {
          resolve('ok')
        })
        document.body.appendChild(webview)
      })`);
      expect(message).to.equal('ok');
    });
  });

  describe('did-change-theme-color event', () => {
    it('emits when theme color changes', async () => {
      const w = new BrowserWindow({
        webPreferences: {
          webviewTag: true
        }
      });
      await w.loadURL('about:blank');
      const src = url.format({
        pathname: `${fixtures.replace(/\\/g, '/')}/pages/theme-color.html`,
        protocol: 'file',
        slashes: true
      });
      const message = await w.webContents.executeJavaScript(`new Promise((resolve, reject) => {
        const webview = new WebView()
        webview.setAttribute('src', '${src}')
        webview.addEventListener('did-change-theme-color', (e) => {
          resolve('ok')
        })
        document.body.appendChild(webview)
      })`);
      expect(message).to.equal('ok');
    });
  });

  // This test is flaky on WOA, so skip it there.
  ifit(process.platform !== 'win32' || process.arch !== 'arm64')('loads devtools extensions registered on the parent window', async () => {
    const w = new BrowserWindow({
      show: false,
      webPreferences: {
        webviewTag: true,
        nodeIntegration: true,
        contextIsolation: false
      }
    });
    w.webContents.session.removeExtension('foo');

    const extensionPath = path.join(__dirname, 'fixtures', 'devtools-extensions', 'foo');
    await w.webContents.session.loadExtension(extensionPath);

    w.loadFile(path.join(__dirname, 'fixtures', 'pages', 'webview-devtools.html'));
    loadWebView(w.webContents, {
      nodeintegration: 'on',
      webpreferences: 'contextIsolation=no',
      src: `file://${path.join(__dirname, 'fixtures', 'blank.html')}`
    }, true);
    let childWebContentsId = 0;
    app.once('web-contents-created', (e, webContents) => {
      childWebContentsId = webContents.id;
      webContents.on('devtools-opened', function () {
        const showPanelIntervalId = setInterval(function () {
          if (!webContents.isDestroyed() && webContents.devToolsWebContents) {
            webContents.devToolsWebContents.executeJavaScript('(' + function () {
              const { UI } = (window as any);
              const tabs = UI.inspectorView.tabbedPane.tabs;
              const lastPanelId: any = tabs[tabs.length - 1].id;
              UI.inspectorView.showPanel(lastPanelId);
            }.toString() + ')()');
          } else {
            clearInterval(showPanelIntervalId);
          }
        }, 100);
      });
    });

    const [, { runtimeId, tabId }] = await emittedOnce(ipcMain, 'answer');
    expect(runtimeId).to.match(/^[a-z]{32}$/);
    expect(tabId).to.equal(childWebContentsId);
  });

  describe('zoom behavior', () => {
    const zoomScheme = standardScheme;
    const webviewSession = session.fromPartition('webview-temp');

    before(() => {
      const protocol = webviewSession.protocol;
      protocol.registerStringProtocol(zoomScheme, (request, callback) => {
        callback('hello');
      });
    });

    after(() => {
      const protocol = webviewSession.protocol;
      protocol.unregisterProtocol(zoomScheme);
    });

    it('inherits the zoomFactor of the parent window', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          webviewTag: true,
          nodeIntegration: true,
          zoomFactor: 1.2,
          contextIsolation: false
        }
      });
      const zoomEventPromise = emittedOnce(ipcMain, 'webview-parent-zoom-level');
      w.loadFile(path.join(fixtures, 'pages', 'webview-zoom-factor.html'));

      const [, zoomFactor, zoomLevel] = await zoomEventPromise;
      expect(zoomFactor).to.equal(1.2);
      expect(zoomLevel).to.equal(1);
    });

    it('maintains zoom level on navigation', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          webviewTag: true,
          nodeIntegration: true,
          zoomFactor: 1.2,
          contextIsolation: false
        }
      });
      const promise = new Promise<void>((resolve) => {
        ipcMain.on('webview-zoom-level', (event, zoomLevel, zoomFactor, newHost, final) => {
          if (!newHost) {
            expect(zoomFactor).to.equal(1.44);
            expect(zoomLevel).to.equal(2.0);
          } else {
            expect(zoomFactor).to.equal(1.2);
            expect(zoomLevel).to.equal(1);
          }

          if (final) {
            resolve();
          }
        });
      });

      w.loadFile(path.join(fixtures, 'pages', 'webview-custom-zoom-level.html'));

      await promise;
    });

    it('maintains zoom level when navigating within same page', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          webviewTag: true,
          nodeIntegration: true,
          zoomFactor: 1.2,
          contextIsolation: false
        }
      });
      const promise = new Promise<void>((resolve) => {
        ipcMain.on('webview-zoom-in-page', (event, zoomLevel, zoomFactor, final) => {
          expect(zoomFactor).to.equal(1.44);
          expect(zoomLevel).to.equal(2.0);

          if (final) {
            resolve();
          }
        });
      });

      w.loadFile(path.join(fixtures, 'pages', 'webview-in-page-navigate.html'));

      await promise;
    });

    it('inherits zoom level for the origin when available', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          webviewTag: true,
          nodeIntegration: true,
          zoomFactor: 1.2,
          contextIsolation: false
        }
      });
      w.loadFile(path.join(fixtures, 'pages', 'webview-origin-zoom-level.html'));

      const [, zoomLevel] = await emittedOnce(ipcMain, 'webview-origin-zoom-level');
      expect(zoomLevel).to.equal(2.0);
    });

    it('does not crash when navigating with zoom level inherited from parent', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          webviewTag: true,
          nodeIntegration: true,
          zoomFactor: 1.2,
          session: webviewSession,
          contextIsolation: false
        }
      });
      const attachPromise = emittedOnce(w.webContents, 'did-attach-webview');
      const readyPromise = emittedOnce(ipcMain, 'dom-ready');
      w.loadFile(path.join(fixtures, 'pages', 'webview-zoom-inherited.html'));
      const [, webview] = await attachPromise;
      await readyPromise;
      expect(webview.getZoomFactor()).to.equal(1.2);
      await w.loadURL(`${zoomScheme}://host1`);
    });

    it('does not crash when changing zoom level after webview is destroyed', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          webviewTag: true,
          nodeIntegration: true,
          session: webviewSession,
          contextIsolation: false
        }
      });
      const attachPromise = emittedOnce(w.webContents, 'did-attach-webview');
      await w.loadFile(path.join(fixtures, 'pages', 'webview-zoom-inherited.html'));
      await attachPromise;
      await w.webContents.executeJavaScript('view.remove()');
      w.webContents.setZoomLevel(0.5);
    });
  });

  describe('requestFullscreen from webview', () => {
    const loadWebViewWindow = async () => {
      const w = new BrowserWindow({
        webPreferences: {
          webviewTag: true,
          nodeIntegration: true,
          contextIsolation: false
        }
      });
      const attachPromise = emittedOnce(w.webContents, 'did-attach-webview');
      const readyPromise = emittedOnce(ipcMain, 'webview-ready');
      w.loadFile(path.join(__dirname, 'fixtures', 'webview', 'fullscreen', 'main.html'));
      const [, webview] = await attachPromise;
      await readyPromise;
      return [w, webview];
    };

    afterEach(closeAllWindows);

    // TODO(jkleinsc) fix this test on arm64 macOS.  It causes the tests following it to fail/be flaky
    ifit(process.platform !== 'darwin' || process.arch !== 'arm64')('should make parent frame element fullscreen too', async () => {
      const [w, webview] = await loadWebViewWindow();
      expect(await w.webContents.executeJavaScript('isIframeFullscreen()')).to.be.false();

      const parentFullscreen = emittedOnce(ipcMain, 'fullscreenchange');
      await webview.executeJavaScript('document.getElementById("div").requestFullscreen()', true);
      await parentFullscreen;
      expect(await w.webContents.executeJavaScript('isIframeFullscreen()')).to.be.true();
    });

    // FIXME(zcbenz): Fullscreen events do not work on Linux.
    // This test is flaky on arm64 macOS.
    ifit(process.platform !== 'linux' && process.arch !== 'arm64')('exiting fullscreen should unfullscreen window', async () => {
      const [w, webview] = await loadWebViewWindow();
      const enterFullScreen = emittedOnce(w, 'enter-full-screen');
      await webview.executeJavaScript('document.getElementById("div").requestFullscreen()', true);
      await enterFullScreen;

      const leaveFullScreen = emittedOnce(w, 'leave-full-screen');
      await webview.executeJavaScript('document.exitFullscreen()', true);
      await leaveFullScreen;
      await delay(0);
      expect(w.isFullScreen()).to.be.false();
    });

    // Sending ESC via sendInputEvent only works on Windows.
    ifit(process.platform === 'win32')('pressing ESC should unfullscreen window', async () => {
      const [w, webview] = await loadWebViewWindow();
      const enterFullScreen = emittedOnce(w, 'enter-full-screen');
      await webview.executeJavaScript('document.getElementById("div").requestFullscreen()', true);
      await enterFullScreen;

      const leaveFullScreen = emittedOnce(w, 'leave-full-screen');
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Escape' });
      await leaveFullScreen;
      await delay(0);
      expect(w.isFullScreen()).to.be.false();
    });

    it('pressing ESC should emit the leave-html-full-screen event', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          webviewTag: true,
          nodeIntegration: true,
          contextIsolation: false
        }
      });

      const didAttachWebview = emittedOnce(w.webContents, 'did-attach-webview');
      w.loadFile(path.join(fixtures, 'pages', 'webview-did-attach-event.html'));

      const [, webContents] = await didAttachWebview;

      const enterFSWindow = emittedOnce(w, 'enter-html-full-screen');
      const enterFSWebview = emittedOnce(webContents, 'enter-html-full-screen');
      await webContents.executeJavaScript('document.getElementById("div").requestFullscreen()', true);
      await enterFSWindow;
      await enterFSWebview;

      const leaveFSWindow = emittedOnce(w, 'leave-html-full-screen');
      const leaveFSWebview = emittedOnce(webContents, 'leave-html-full-screen');
      webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Escape' });
      await leaveFSWindow;
      await leaveFSWebview;
    });
  });

  describe('child windows', () => {
    let w: BrowserWindow;
    beforeEach(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, webviewTag: true, contextIsolation: false } });
      await w.loadURL('about:blank');
    });
    afterEach(closeAllWindows);

    it('opens window of about:blank with cross-scripting enabled', async () => {
      // Don't wait for loading to finish.
      loadWebView(w.webContents, {
        allowpopups: 'on',
        nodeintegration: 'on',
        webpreferences: 'contextIsolation=no',
        src: `file://${path.join(fixtures, 'api', 'native-window-open-blank.html')}`
      });

      const [, content] = await emittedOnce(ipcMain, 'answer');
      expect(content).to.equal('Hello');
    });

    it('opens window of same domain with cross-scripting enabled', async () => {
      // Don't wait for loading to finish.
      loadWebView(w.webContents, {
        allowpopups: 'on',
        nodeintegration: 'on',
        webpreferences: 'contextIsolation=no',
        src: `file://${path.join(fixtures, 'api', 'native-window-open-file.html')}`
      });

      const [, content] = await emittedOnce(ipcMain, 'answer');
      expect(content).to.equal('Hello');
    });

    it('returns null from window.open when allowpopups is not set', async () => {
      // Don't wait for loading to finish.
      loadWebView(w.webContents, {
        nodeintegration: 'on',
        webpreferences: 'contextIsolation=no',
        src: `file://${path.join(fixtures, 'api', 'native-window-open-no-allowpopups.html')}`
      });

      const [, { windowOpenReturnedNull }] = await emittedOnce(ipcMain, 'answer');
      expect(windowOpenReturnedNull).to.be.true();
    });

    it('blocks accessing cross-origin frames', async () => {
      // Don't wait for loading to finish.
      loadWebView(w.webContents, {
        allowpopups: 'on',
        nodeintegration: 'on',
        webpreferences: 'contextIsolation=no',
        src: `file://${path.join(fixtures, 'api', 'native-window-open-cross-origin.html')}`
      });

      const [, content] = await emittedOnce(ipcMain, 'answer');
      const expectedContent =
          'Blocked a frame with origin "file://" from accessing a cross-origin frame.';

      expect(content).to.equal(expectedContent);
    });

    it('emits a new-window event', async () => {
      // Don't wait for loading to finish.
      const attributes = {
        allowpopups: 'on',
        nodeintegration: 'on',
        webpreferences: 'contextIsolation=no',
        src: `file://${fixtures}/pages/window-open.html`
      };
      const { url, frameName } = await w.webContents.executeJavaScript(`
        new Promise((resolve, reject) => {
          const webview = document.createElement('webview')
          for (const [k, v] of Object.entries(${JSON.stringify(attributes)})) {
            webview.setAttribute(k, v)
          }
          document.body.appendChild(webview)
          webview.addEventListener('new-window', (e) => {
            resolve({url: e.url, frameName: e.frameName})
          })
        })
      `);

      expect(url).to.equal('http://host/');
      expect(frameName).to.equal('host');
    });

    it('emits a browser-window-created event', async () => {
      // Don't wait for loading to finish.
      loadWebView(w.webContents, {
        allowpopups: 'on',
        webpreferences: 'contextIsolation=no',
        src: `file://${fixtures}/pages/window-open.html`
      });

      await emittedOnce(app, 'browser-window-created');
    });

    it('emits a web-contents-created event', async () => {
      const webContentsCreated = emittedUntil(app, 'web-contents-created',
        (event: Electron.Event, contents: Electron.WebContents) => contents.getType() === 'window');

      loadWebView(w.webContents, {
        allowpopups: 'on',
        webpreferences: 'contextIsolation=no',
        src: `file://${fixtures}/pages/window-open.html`
      });

      await webContentsCreated;
    });

    it('does not crash when creating window with noopener', async () => {
      loadWebView(w.webContents, {
        allowpopups: 'on',
        src: `file://${path.join(fixtures, 'api', 'native-window-open-noopener.html')}`
      });
      await emittedOnce(app, 'browser-window-created');
    });
  });

  describe('webpreferences attribute', () => {
    let w: BrowserWindow;
    beforeEach(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, webviewTag: true } });
      await w.loadURL('about:blank');
    });
    afterEach(closeAllWindows);

    it('can enable context isolation', async () => {
      loadWebView(w.webContents, {
        allowpopups: 'yes',
        preload: `file://${fixtures}/api/isolated-preload.js`,
        src: `file://${fixtures}/api/isolated.html`,
        webpreferences: 'contextIsolation=yes'
      });

      const [, data] = await emittedOnce(ipcMain, 'isolated-world');
      expect(data).to.deep.equal({
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
      });
    });
  });

  describe('permission request handlers', () => {
    let w: BrowserWindow;
    beforeEach(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, webviewTag: true, contextIsolation: false } });
      await w.loadURL('about:blank');
    });
    afterEach(closeAllWindows);

    const partition = 'permissionTest';

    function setUpRequestHandler (webContentsId: number, requestedPermission: string) {
      return new Promise<void>((resolve, reject) => {
        session.fromPartition(partition).setPermissionRequestHandler(function (webContents, permission, callback) {
          if (webContents.id === webContentsId) {
            // requestMIDIAccess with sysex requests both midi and midiSysex so
            // grant the first midi one and then reject the midiSysex one
            if (requestedPermission === 'midiSysex' && permission === 'midi') {
              return callback(true);
            }

            try {
              expect(permission).to.equal(requestedPermission);
            } catch (e) {
              return reject(e);
            }
            callback(false);
            resolve();
          }
        });
      });
    }
    afterEach(() => {
      session.fromPartition(partition).setPermissionRequestHandler(null);
    });

    // This is disabled because CI machines don't have cameras or microphones,
    // so Chrome responds with "NotFoundError" instead of
    // "PermissionDeniedError". It should be re-enabled if we find a way to mock
    // the presence of a microphone & camera.
    xit('emits when using navigator.getUserMedia api', async () => {
      const errorFromRenderer = emittedOnce(ipcMain, 'message');
      loadWebView(w.webContents, {
        src: `file://${fixtures}/pages/permissions/media.html`,
        partition,
        nodeintegration: 'on'
      });
      const [, webViewContents] = await emittedOnce(app, 'web-contents-created');
      setUpRequestHandler(webViewContents.id, 'media');
      const [, errorName] = await errorFromRenderer;
      expect(errorName).to.equal('PermissionDeniedError');
    });

    it('emits when using navigator.geolocation api', async () => {
      const errorFromRenderer = emittedOnce(ipcMain, 'message');
      loadWebView(w.webContents, {
        src: `file://${fixtures}/pages/permissions/geolocation.html`,
        partition,
        nodeintegration: 'on',
        webpreferences: 'contextIsolation=no'
      });
      const [, webViewContents] = await emittedOnce(app, 'web-contents-created');
      setUpRequestHandler(webViewContents.id, 'geolocation');
      const [, error] = await errorFromRenderer;
      expect(error).to.equal('User denied Geolocation');
    });

    it('emits when using navigator.requestMIDIAccess without sysex api', async () => {
      const errorFromRenderer = emittedOnce(ipcMain, 'message');
      loadWebView(w.webContents, {
        src: `file://${fixtures}/pages/permissions/midi.html`,
        partition,
        nodeintegration: 'on',
        webpreferences: 'contextIsolation=no'
      });
      const [, webViewContents] = await emittedOnce(app, 'web-contents-created');
      setUpRequestHandler(webViewContents.id, 'midi');
      const [, error] = await errorFromRenderer;
      expect(error).to.equal('SecurityError');
    });

    it('emits when using navigator.requestMIDIAccess with sysex api', async () => {
      const errorFromRenderer = emittedOnce(ipcMain, 'message');
      loadWebView(w.webContents, {
        src: `file://${fixtures}/pages/permissions/midi-sysex.html`,
        partition,
        nodeintegration: 'on',
        webpreferences: 'contextIsolation=no'
      });
      const [, webViewContents] = await emittedOnce(app, 'web-contents-created');
      setUpRequestHandler(webViewContents.id, 'midiSysex');
      const [, error] = await errorFromRenderer;
      expect(error).to.equal('SecurityError');
    });

    it('emits when accessing external protocol', async () => {
      loadWebView(w.webContents, {
        src: 'magnet:test',
        partition
      });
      const [, webViewContents] = await emittedOnce(app, 'web-contents-created');
      await setUpRequestHandler(webViewContents.id, 'openExternal');
    });

    it('emits when using Notification.requestPermission', async () => {
      const errorFromRenderer = emittedOnce(ipcMain, 'message');
      loadWebView(w.webContents, {
        src: `file://${fixtures}/pages/permissions/notification.html`,
        partition,
        nodeintegration: 'on',
        webpreferences: 'contextIsolation=no'
      });
      const [, webViewContents] = await emittedOnce(app, 'web-contents-created');

      await setUpRequestHandler(webViewContents.id, 'notifications');

      const [, error] = await errorFromRenderer;
      expect(error).to.equal('denied');
    });
  });

  describe('DOM events', () => {
    afterEach(closeAllWindows);
    it('receives extra properties on DOM events when contextIsolation is enabled', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          webviewTag: true,
          contextIsolation: true
        }
      });
      await w.loadURL('about:blank');
      const message = await w.webContents.executeJavaScript(`new Promise((resolve, reject) => {
        const webview = new WebView()
        webview.setAttribute('src', 'data:text/html,<script>console.log("hi")</script>')
        webview.addEventListener('console-message', (e) => {
          resolve(e.message)
        })
        document.body.appendChild(webview)
      })`);
      expect(message).to.equal('hi');
    });

    it('emits focus event when contextIsolation is enabled', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          webviewTag: true,
          contextIsolation: true
        }
      });
      await w.loadURL('about:blank');
      await w.webContents.executeJavaScript(`new Promise((resolve, reject) => {
        const webview = new WebView()
        webview.setAttribute('src', 'about:blank')
        webview.addEventListener('dom-ready', () => {
          webview.focus()
        })
        webview.addEventListener('focus', () => {
          resolve();
        })
        document.body.appendChild(webview)
      })`);
    });
  });
});

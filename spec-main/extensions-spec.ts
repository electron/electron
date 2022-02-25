import { expect } from 'chai';
import { app, session, BrowserWindow, ipcMain, WebContents, Extension, Session } from 'electron/main';
import { closeAllWindows, closeWindow } from './window-helpers';
import * as http from 'http';
import { AddressInfo } from 'net';
import * as path from 'path';
import * as fs from 'fs';
import * as WebSocket from 'ws';
import { emittedOnce, emittedNTimes, emittedUntil } from './events-helpers';
import { ifit } from './spec-helpers';

const uuid = require('uuid');

const fixtures = path.join(__dirname, 'fixtures');

describe('chrome extensions', () => {
  const emptyPage = '<script>console.log("loaded")</script>';

  // NB. extensions are only allowed on http://, https:// and ftp:// (!) urls by default.
  let server: http.Server;
  let url: string;
  let port: string;
  before(async () => {
    server = http.createServer((req, res) => {
      if (req.url === '/cors') {
        res.setHeader('Access-Control-Allow-Origin', 'http://example.com');
      }
      res.end(emptyPage);
    });

    const wss = new WebSocket.Server({ noServer: true });
    wss.on('connection', function connection (ws) {
      ws.on('message', function incoming (message) {
        if (message === 'foo') {
          ws.send('bar');
        }
      });
    });

    await new Promise<void>(resolve => server.listen(0, '127.0.0.1', () => {
      port = String((server.address() as AddressInfo).port);
      url = `http://127.0.0.1:${port}`;
      resolve();
    }));
  });
  after(() => {
    server.close();
  });
  afterEach(closeAllWindows);
  afterEach(() => {
    session.defaultSession.getAllExtensions().forEach((e: any) => {
      session.defaultSession.removeExtension(e.id);
    });
  });

  it('does not crash when using chrome.management', async () => {
    const customSession = session.fromPartition(`persist:${require('uuid').v4()}`);
    const w = new BrowserWindow({ show: false, webPreferences: { session: customSession, sandbox: true } });
    await w.loadURL('about:blank');

    const promise = emittedOnce(app, 'web-contents-created');
    await customSession.loadExtension(path.join(fixtures, 'extensions', 'persistent-background-page'));
    const args: any = await promise;
    const wc: Electron.WebContents = args[1];
    await expect(wc.executeJavaScript(`
      (() => {
        return new Promise((resolve) => {
          chrome.management.getSelf((info) => {
            resolve(info);
          });
        })
      })();
    `)).to.eventually.have.property('id');
  });

  it('can open WebSQLDatabase in a background page', async () => {
    const customSession = session.fromPartition(`persist:${require('uuid').v4()}`);
    const w = new BrowserWindow({ show: false, webPreferences: { session: customSession, sandbox: true } });
    await w.loadURL('about:blank');

    const promise = emittedOnce(app, 'web-contents-created');
    await customSession.loadExtension(path.join(fixtures, 'extensions', 'persistent-background-page'));
    const args: any = await promise;
    const wc: Electron.WebContents = args[1];
    await expect(wc.executeJavaScript('(()=>{try{openDatabase("t", "1.0", "test", 2e5);return true;}catch(e){throw e}})()')).to.not.be.rejected();
  });

  function fetch (contents: WebContents, url: string) {
    return contents.executeJavaScript(`fetch(${JSON.stringify(url)})`);
  }

  it('bypasses CORS in requests made from extensions', async () => {
    const customSession = session.fromPartition(`persist:${require('uuid').v4()}`);
    const w = new BrowserWindow({ show: false, webPreferences: { session: customSession, sandbox: true } });
    const extension = await customSession.loadExtension(path.join(fixtures, 'extensions', 'ui-page'));
    await w.loadURL(`${extension.url}bare-page.html`);
    await expect(fetch(w.webContents, `${url}/cors`)).to.not.be.rejectedWith(TypeError);
  });

  it('loads an extension', async () => {
    // NB. we have to use a persist: session (i.e. non-OTR) because the
    // extension registry is redirected to the main session. so installing an
    // extension in an in-memory session results in it being installed in the
    // default session.
    const customSession = session.fromPartition(`persist:${uuid.v4()}`);
    await customSession.loadExtension(path.join(fixtures, 'extensions', 'red-bg'));
    const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
    await w.loadURL(url);
    const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor');
    expect(bg).to.equal('red');
  });

  it('does not crash when failing to load an extension', async () => {
    const customSession = session.fromPartition(`persist:${uuid.v4()}`);
    const promise = customSession.loadExtension(path.join(fixtures, 'extensions', 'load-error'));
    await expect(promise).to.eventually.be.rejected();
  });

  it('serializes a loaded extension', async () => {
    const extensionPath = path.join(fixtures, 'extensions', 'red-bg');
    const manifest = JSON.parse(fs.readFileSync(path.join(extensionPath, 'manifest.json'), 'utf-8'));
    const customSession = session.fromPartition(`persist:${uuid.v4()}`);
    const extension = await customSession.loadExtension(extensionPath);
    expect(extension.id).to.be.a('string');
    expect(extension.name).to.be.a('string');
    expect(extension.path).to.be.a('string');
    expect(extension.version).to.be.a('string');
    expect(extension.url).to.be.a('string');
    expect(extension.manifest).to.deep.equal(manifest);
  });

  it('removes an extension', async () => {
    const customSession = session.fromPartition(`persist:${uuid.v4()}`);
    const { id } = await customSession.loadExtension(path.join(fixtures, 'extensions', 'red-bg'));
    {
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
      await w.loadURL(url);
      const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor');
      expect(bg).to.equal('red');
    }
    customSession.removeExtension(id);
    {
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
      await w.loadURL(url);
      const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor');
      expect(bg).to.equal('');
    }
  });

  it('emits extension lifecycle events', async () => {
    const customSession = session.fromPartition(`persist:${require('uuid').v4()}`);

    const loadedPromise = emittedOnce(customSession, 'extension-loaded');
    const extension = await customSession.loadExtension(path.join(fixtures, 'extensions', 'red-bg'));
    const [, loadedExtension] = await loadedPromise;
    const [, readyExtension] = await emittedUntil(customSession, 'extension-ready', (event: Event, extension: Extension) => {
      return extension.name !== 'Chromium PDF Viewer' && extension.name !== 'CryptoTokenExtension';
    });

    expect(loadedExtension).to.deep.equal(extension);
    expect(readyExtension).to.deep.equal(extension);

    const unloadedPromise = emittedOnce(customSession, 'extension-unloaded');
    await customSession.removeExtension(extension.id);
    const [, unloadedExtension] = await unloadedPromise;
    expect(unloadedExtension).to.deep.equal(extension);
  });

  it('lists loaded extensions in getAllExtensions', async () => {
    const customSession = session.fromPartition(`persist:${uuid.v4()}`);
    const e = await customSession.loadExtension(path.join(fixtures, 'extensions', 'red-bg'));
    expect(customSession.getAllExtensions()).to.deep.equal([e]);
    customSession.removeExtension(e.id);
    expect(customSession.getAllExtensions()).to.deep.equal([]);
  });

  it('gets an extension by id', async () => {
    const customSession = session.fromPartition(`persist:${uuid.v4()}`);
    const e = await customSession.loadExtension(path.join(fixtures, 'extensions', 'red-bg'));
    expect(customSession.getExtension(e.id)).to.deep.equal(e);
  });

  it('confines an extension to the session it was loaded in', async () => {
    const customSession = session.fromPartition(`persist:${uuid.v4()}`);
    await customSession.loadExtension(path.join(fixtures, 'extensions', 'red-bg'));
    const w = new BrowserWindow({ show: false }); // not in the session
    await w.loadURL(url);
    const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor');
    expect(bg).to.equal('');
  });

  it('loading an extension in a temporary session throws an error', async () => {
    const customSession = session.fromPartition(uuid.v4());
    await expect(customSession.loadExtension(path.join(fixtures, 'extensions', 'red-bg'))).to.eventually.be.rejectedWith('Extensions cannot be loaded in a temporary session');
  });

  describe('chrome.i18n', () => {
    let w: BrowserWindow;
    let extension: Extension;
    const exec = async (name: string) => {
      const p = emittedOnce(ipcMain, 'success');
      await w.webContents.executeJavaScript(`exec('${name}')`);
      const [, result] = await p;
      return result;
    };
    beforeEach(async () => {
      const customSession = session.fromPartition(`persist:${uuid.v4()}`);
      extension = await customSession.loadExtension(path.join(fixtures, 'extensions', 'chrome-i18n'));
      w = new BrowserWindow({ show: false, webPreferences: { session: customSession, nodeIntegration: true, contextIsolation: false } });
      await w.loadURL(url);
    });
    it('getAcceptLanguages()', async () => {
      const result = await exec('getAcceptLanguages');
      expect(result).to.be.an('array').and.deep.equal(['en-US', 'en']);
    });
    it('getMessage()', async () => {
      const result = await exec('getMessage');
      expect(result.id).to.be.a('string').and.equal(extension.id);
      expect(result.name).to.be.a('string').and.equal('chrome-i18n');
    });
  });

  describe('chrome.runtime', () => {
    let w: BrowserWindow;
    const exec = async (name: string) => {
      const p = emittedOnce(ipcMain, 'success');
      await w.webContents.executeJavaScript(`exec('${name}')`);
      const [, result] = await p;
      return result;
    };
    beforeEach(async () => {
      const customSession = session.fromPartition(`persist:${uuid.v4()}`);
      await customSession.loadExtension(path.join(fixtures, 'extensions', 'chrome-runtime'));
      w = new BrowserWindow({ show: false, webPreferences: { session: customSession, nodeIntegration: true, contextIsolation: false } });
      await w.loadURL(url);
    });
    it('getManifest()', async () => {
      const result = await exec('getManifest');
      expect(result).to.be.an('object').with.property('name', 'chrome-runtime');
    });
    it('id', async () => {
      const result = await exec('id');
      expect(result).to.be.a('string').with.lengthOf(32);
    });
    it('getURL()', async () => {
      const result = await exec('getURL');
      expect(result).to.be.a('string').and.match(/^chrome-extension:\/\/.*main.js$/);
    });
    it('getPlatformInfo()', async () => {
      const result = await exec('getPlatformInfo');
      expect(result).to.be.an('object');
      expect(result.os).to.be.a('string');
      expect(result.arch).to.be.a('string');
      expect(result.nacl_arch).to.be.a('string');
    });
  });

  describe('chrome.storage', () => {
    it('stores and retrieves a key', async () => {
      const customSession = session.fromPartition(`persist:${uuid.v4()}`);
      await customSession.loadExtension(path.join(fixtures, 'extensions', 'chrome-storage'));
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession, nodeIntegration: true, contextIsolation: false } });
      try {
        const p = emittedOnce(ipcMain, 'storage-success');
        await w.loadURL(url);
        const [, v] = await p;
        expect(v).to.equal('value');
      } finally {
        w.destroy();
      }
    });
  });

  describe('chrome.webRequest', () => {
    function fetch (contents: WebContents, url: string) {
      return contents.executeJavaScript(`fetch(${JSON.stringify(url)})`);
    }

    let customSession: Session;
    let w: BrowserWindow;

    beforeEach(() => {
      customSession = session.fromPartition(`persist:${uuid.v4()}`);
      w = new BrowserWindow({ show: false, webPreferences: { session: customSession, sandbox: true, contextIsolation: true } });
    });

    describe('onBeforeRequest', () => {
      it('can cancel http requests', async () => {
        await w.loadURL(url);
        await customSession.loadExtension(path.join(fixtures, 'extensions', 'chrome-webRequest'));
        await expect(fetch(w.webContents, url)).to.eventually.be.rejectedWith('Failed to fetch');
      });

      it('does not cancel http requests when no extension loaded', async () => {
        await w.loadURL(url);
        await expect(fetch(w.webContents, url)).to.not.be.rejectedWith('Failed to fetch');
      });
    });

    it('does not take precedence over Electron webRequest - http', async () => {
      return new Promise<void>((resolve) => {
        (async () => {
          customSession.webRequest.onBeforeRequest((details, callback) => {
            resolve();
            callback({ cancel: true });
          });
          await w.loadURL(url);

          await customSession.loadExtension(path.join(fixtures, 'extensions', 'chrome-webRequest'));
          fetch(w.webContents, url);
        })();
      });
    });

    it('does not take precedence over Electron webRequest - WebSocket', () => {
      return new Promise<void>((resolve) => {
        (async () => {
          customSession.webRequest.onBeforeSendHeaders(() => {
            resolve();
          });
          await w.loadFile(path.join(fixtures, 'api', 'webrequest.html'), { query: { port } });
          await customSession.loadExtension(path.join(fixtures, 'extensions', 'chrome-webRequest-wss'));
        })();
      });
    });

    describe('WebSocket', () => {
      it('can be proxied', async () => {
        await w.loadFile(path.join(fixtures, 'api', 'webrequest.html'), { query: { port } });
        await customSession.loadExtension(path.join(fixtures, 'extensions', 'chrome-webRequest-wss'));
        customSession.webRequest.onSendHeaders((details) => {
          if (details.url.startsWith('ws://')) {
            expect(details.requestHeaders.foo).be.equal('bar');
          }
        });
      });
    });
  });

  describe('chrome.tabs', () => {
    let customSession: Session;
    before(async () => {
      customSession = session.fromPartition(`persist:${uuid.v4()}`);
      await customSession.loadExtension(path.join(fixtures, 'extensions', 'chrome-api'));
    });

    it('executeScript', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession, nodeIntegration: true } });
      await w.loadURL(url);

      const message = { method: 'executeScript', args: ['1 + 2'] };
      w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

      const [,, responseString] = await emittedOnce(w.webContents, 'console-message');
      const response = JSON.parse(responseString);

      expect(response).to.equal(3);
    });

    it('connect', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession, nodeIntegration: true } });
      await w.loadURL(url);

      const portName = uuid.v4();
      const message = { method: 'connectTab', args: [portName] };
      w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

      const [,, responseString] = await emittedOnce(w.webContents, 'console-message');
      const response = responseString.split(',');
      expect(response[0]).to.equal(portName);
      expect(response[1]).to.equal('howdy');
    });

    it('sendMessage receives the response', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession, nodeIntegration: true } });
      await w.loadURL(url);

      const message = { method: 'sendMessage', args: ['Hello World!'] };
      w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

      const [,, responseString] = await emittedOnce(w.webContents, 'console-message');
      const response = JSON.parse(responseString);

      expect(response.message).to.equal('Hello World!');
      expect(response.tabId).to.equal(w.webContents.id);
    });

    it('update', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession, nodeIntegration: true } });
      await w.loadURL(url);

      const w2 = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
      await w2.loadURL('about:blank');

      const w2Navigated = emittedOnce(w2.webContents, 'did-navigate');

      const message = { method: 'update', args: [w2.webContents.id, { url }] };
      w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

      const [,, responseString] = await emittedOnce(w.webContents, 'console-message');
      const response = JSON.parse(responseString);

      await w2Navigated;

      expect(new URL(w2.getURL()).toString()).to.equal(new URL(url).toString());

      expect(response.id).to.equal(w2.webContents.id);
    });
  });

  describe('background pages', () => {
    it('loads a lazy background page when sending a message', async () => {
      const customSession = session.fromPartition(`persist:${uuid.v4()}`);
      await customSession.loadExtension(path.join(fixtures, 'extensions', 'lazy-background-page'));
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession, nodeIntegration: true, contextIsolation: false } });
      try {
        w.loadURL(url);
        const [, resp] = await emittedOnce(ipcMain, 'bg-page-message-response');
        expect(resp.message).to.deep.equal({ some: 'message' });
        expect(resp.sender.id).to.be.a('string');
        expect(resp.sender.origin).to.equal(url);
        expect(resp.sender.url).to.equal(url + '/');
      } finally {
        w.destroy();
      }
    });

    it('can use extension.getBackgroundPage from a ui page', async () => {
      const customSession = session.fromPartition(`persist:${uuid.v4()}`);
      const { id } = await customSession.loadExtension(path.join(fixtures, 'extensions', 'lazy-background-page'));
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
      await w.loadURL(`chrome-extension://${id}/page-get-background.html`);
      const receivedMessage = await w.webContents.executeJavaScript('window.completionPromise');
      expect(receivedMessage).to.deep.equal({ some: 'message' });
    });

    it('can use extension.getBackgroundPage from a ui page', async () => {
      const customSession = session.fromPartition(`persist:${uuid.v4()}`);
      const { id } = await customSession.loadExtension(path.join(fixtures, 'extensions', 'lazy-background-page'));
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
      await w.loadURL(`chrome-extension://${id}/page-get-background.html`);
      const receivedMessage = await w.webContents.executeJavaScript('window.completionPromise');
      expect(receivedMessage).to.deep.equal({ some: 'message' });
    });

    it('can use runtime.getBackgroundPage from a ui page', async () => {
      const customSession = session.fromPartition(`persist:${uuid.v4()}`);
      const { id } = await customSession.loadExtension(path.join(fixtures, 'extensions', 'lazy-background-page'));
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
      await w.loadURL(`chrome-extension://${id}/page-runtime-get-background.html`);
      const receivedMessage = await w.webContents.executeJavaScript('window.completionPromise');
      expect(receivedMessage).to.deep.equal({ some: 'message' });
    });

    it('has session in background page', async () => {
      const customSession = session.fromPartition(`persist:${require('uuid').v4()}`);
      const promise = emittedOnce(app, 'web-contents-created');
      const { id } = await customSession.loadExtension(path.join(fixtures, 'extensions', 'persistent-background-page'));
      const [, bgPageContents] = await promise;
      expect(bgPageContents.getType()).to.equal('backgroundPage');
      await emittedOnce(bgPageContents, 'did-finish-load');
      expect(bgPageContents.getURL()).to.equal(`chrome-extension://${id}/_generated_background_page.html`);
      expect(bgPageContents.session).to.not.equal(undefined);
    });

    it('can open devtools of background page', async () => {
      const customSession = session.fromPartition(`persist:${require('uuid').v4()}`);
      const promise = emittedOnce(app, 'web-contents-created');
      await customSession.loadExtension(path.join(fixtures, 'extensions', 'persistent-background-page'));
      const [, bgPageContents] = await promise;
      expect(bgPageContents.getType()).to.equal('backgroundPage');
      bgPageContents.openDevTools();
      bgPageContents.closeDevTools();
    });
  });

  describe('devtools extensions', () => {
    let showPanelTimeoutId: any = null;
    afterEach(() => {
      if (showPanelTimeoutId) clearTimeout(showPanelTimeoutId);
    });
    const showLastDevToolsPanel = (w: BrowserWindow) => {
      w.webContents.once('devtools-opened', () => {
        const show = () => {
          if (w == null || w.isDestroyed()) return;
          const { devToolsWebContents } = w as unknown as { devToolsWebContents: WebContents | undefined };
          if (devToolsWebContents == null || devToolsWebContents.isDestroyed()) {
            return;
          }

          const showLastPanel = () => {
            // this is executed in the devtools context, where UI is a global
            const { UI } = (window as any);
            const tabs = UI.inspectorView.tabbedPane.tabs;
            const lastPanelId = tabs[tabs.length - 1].id;
            UI.inspectorView.showPanel(lastPanelId);
          };
          devToolsWebContents.executeJavaScript(`(${showLastPanel})()`, false).then(() => {
            showPanelTimeoutId = setTimeout(show, 100);
          });
        };
        showPanelTimeoutId = setTimeout(show, 100);
      });
    };

    // TODO(jkleinsc) fix this flaky test on WOA
    ifit(process.platform !== 'win32' || process.arch !== 'arm64')('loads a devtools extension', async () => {
      const customSession = session.fromPartition(`persist:${uuid.v4()}`);
      customSession.loadExtension(path.join(fixtures, 'extensions', 'devtools-extension'));
      const winningMessage = emittedOnce(ipcMain, 'winning');
      const w = new BrowserWindow({ show: true, webPreferences: { session: customSession, nodeIntegration: true, contextIsolation: false } });
      await w.loadURL(url);
      w.webContents.openDevTools();
      showLastDevToolsPanel(w);
      await winningMessage;
    });
  });

  describe('chrome extension content scripts', () => {
    const fixtures = path.resolve(__dirname, 'fixtures');
    const extensionPath = path.resolve(fixtures, 'extensions');

    const addExtension = (name: string) => session.defaultSession.loadExtension(path.resolve(extensionPath, name));
    const removeAllExtensions = () => {
      Object.keys(session.defaultSession.getAllExtensions()).map(extName => {
        session.defaultSession.removeExtension(extName);
      });
    };

    let responseIdCounter = 0;
    const executeJavaScriptInFrame = (webContents: WebContents, frameRoutingId: number, code: string) => {
      return new Promise(resolve => {
        const responseId = responseIdCounter++;
        ipcMain.once(`executeJavaScriptInFrame_${responseId}`, (event, result) => {
          resolve(result);
        });
        webContents.send('executeJavaScriptInFrame', frameRoutingId, code, responseId);
      });
    };

    const generateTests = (sandboxEnabled: boolean, contextIsolationEnabled: boolean) => {
      describe(`with sandbox ${sandboxEnabled ? 'enabled' : 'disabled'} and context isolation ${contextIsolationEnabled ? 'enabled' : 'disabled'}`, () => {
        let w: BrowserWindow;

        describe('supports "run_at" option', () => {
          beforeEach(async () => {
            await closeWindow(w);
            w = new BrowserWindow({
              show: false,
              width: 400,
              height: 400,
              webPreferences: {
                contextIsolation: contextIsolationEnabled,
                sandbox: sandboxEnabled
              }
            });
          });

          afterEach(() => {
            removeAllExtensions();
            return closeWindow(w).then(() => { w = null as unknown as BrowserWindow; });
          });

          it('should run content script at document_start', async () => {
            await addExtension('content-script-document-start');
            w.webContents.once('dom-ready', async () => {
              const result = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor');
              expect(result).to.equal('red');
            });
            w.loadURL(url);
          });

          it('should run content script at document_idle', async () => {
            await addExtension('content-script-document-idle');
            w.loadURL(url);
            const result = await w.webContents.executeJavaScript('document.body.style.backgroundColor');
            expect(result).to.equal('red');
          });

          it('should run content script at document_end', async () => {
            await addExtension('content-script-document-end');
            w.webContents.once('did-finish-load', async () => {
              const result = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor');
              expect(result).to.equal('red');
            });
            w.loadURL(url);
          });
        });

        // TODO(nornagon): real extensions don't load on file: urls, so this
        // test needs to be updated to serve its content over http.
        describe.skip('supports "all_frames" option', () => {
          const contentScript = path.resolve(fixtures, 'extensions/content-script');

          // Computed style values
          const COLOR_RED = 'rgb(255, 0, 0)';
          const COLOR_BLUE = 'rgb(0, 0, 255)';
          const COLOR_TRANSPARENT = 'rgba(0, 0, 0, 0)';

          before(() => {
            session.defaultSession.loadExtension(contentScript);
          });

          after(() => {
            session.defaultSession.removeExtension('content-script-test');
          });

          beforeEach(() => {
            w = new BrowserWindow({
              show: false,
              webPreferences: {
                // enable content script injection in subframes
                nodeIntegrationInSubFrames: true,
                preload: path.join(contentScript, 'all_frames-preload.js')
              }
            });
          });

          afterEach(() =>
            closeWindow(w).then(() => {
              w = null as unknown as BrowserWindow;
            })
          );

          it('applies matching rules in subframes', async () => {
            const detailsPromise = emittedNTimes(w.webContents, 'did-frame-finish-load', 2);
            w.loadFile(path.join(contentScript, 'frame-with-frame.html'));
            const frameEvents = await detailsPromise;
            await Promise.all(
              frameEvents.map(async frameEvent => {
                const [, isMainFrame, , frameRoutingId] = frameEvent;
                const result: any = await executeJavaScriptInFrame(
                  w.webContents,
                  frameRoutingId,
                  `(() => {
                    const a = document.getElementById('all_frames_enabled')
                    const b = document.getElementById('all_frames_disabled')
                    return {
                      enabledColor: getComputedStyle(a).backgroundColor,
                      disabledColor: getComputedStyle(b).backgroundColor
                    }
                  })()`
                );
                expect(result.enabledColor).to.equal(COLOR_RED);
                if (isMainFrame) {
                  expect(result.disabledColor).to.equal(COLOR_BLUE);
                } else {
                  expect(result.disabledColor).to.equal(COLOR_TRANSPARENT); // null color
                }
              })
            );
          });
        });
      });
    };

    generateTests(false, false);
    generateTests(false, true);
    generateTests(true, false);
    generateTests(true, true);
  });

  describe('extension ui pages', () => {
    afterEach(() => {
      session.defaultSession.getAllExtensions().forEach(e => {
        session.defaultSession.removeExtension(e.id);
      });
    });

    it('loads a ui page of an extension', async () => {
      const { id } = await session.defaultSession.loadExtension(path.join(fixtures, 'extensions', 'ui-page'));
      const w = new BrowserWindow({ show: false });
      await w.loadURL(`chrome-extension://${id}/bare-page.html`);
      const textContent = await w.webContents.executeJavaScript('document.body.textContent');
      expect(textContent).to.equal('ui page loaded ok\n');
    });

    it('can load resources', async () => {
      const { id } = await session.defaultSession.loadExtension(path.join(fixtures, 'extensions', 'ui-page'));
      const w = new BrowserWindow({ show: false });
      await w.loadURL(`chrome-extension://${id}/page-script-load.html`);
      const textContent = await w.webContents.executeJavaScript('document.body.textContent');
      expect(textContent).to.equal('script loaded ok\n');
    });
  });

  describe('manifest v3', () => {
    it('registers background service worker', async () => {
      const customSession = session.fromPartition(`persist:${uuid.v4()}`);
      const registrationPromise = new Promise<string>(resolve => {
        customSession.serviceWorkers.once('registration-completed', (event, { scope }) => resolve(scope));
      });
      const extension = await customSession.loadExtension(path.join(fixtures, 'extensions', 'mv3-service-worker'));
      const scope = await registrationPromise;
      expect(scope).equals(extension.url);
    });
  });
});

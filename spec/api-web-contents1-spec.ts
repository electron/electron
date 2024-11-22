import { BrowserWindow, ipcMain, webContents, BrowserView, WebContents } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';
import * as http from 'node:http';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';

import { ifdescribe, waitUntil, listen, ifit } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';

const fixturesPath = path.resolve(__dirname, 'fixtures');
const features = process._linkedBinding('electron_common_features');

describe('webContents1 module', () => {
  ifdescribe(process.env.TEST_SHARD !== '1')('getAllWebContents() API', () => {
    afterEach(closeAllWindows);
    it('returns an array of web contents', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: { webviewTag: true }
      });
      w.loadFile(path.join(fixturesPath, 'pages', 'webview-zoom-factor.html'));

      await once(w.webContents, 'did-attach-webview') as [any, WebContents];

      w.webContents.openDevTools();

      await once(w.webContents, 'devtools-opened');

      const all = webContents.getAllWebContents().sort((a, b) => {
        return a.id - b.id;
      });

      expect(all).to.have.length(3);
      expect(all[0].getType()).to.equal('window');
      expect(all[all.length - 2].getType()).to.equal('webview');
      expect(all[all.length - 1].getType()).to.equal('remote');
    });
  });

  ifdescribe(process.env.TEST_SHARD !== '2')('webContents properties', () => {
    afterEach(closeAllWindows);

    it('has expected additional enumerable properties', () => {
      const w = new BrowserWindow({ show: false });
      const properties = Object.getOwnPropertyNames(w.webContents);
      expect(properties).to.include('ipc');
      expect(properties).to.include('navigationHistory');
    });
  });

  ifdescribe(process.env.TEST_SHARD !== '3')('fromId()', () => {
    it('returns undefined for an unknown id', () => {
      expect(webContents.fromId(12345)).to.be.undefined();
    });
  });

  ifdescribe(process.env.TEST_SHARD !== '4')('fromFrame()', () => {
    it('returns WebContents for mainFrame', () => {
      const contents = (webContents as typeof ElectronInternal.WebContents).create();
      expect(webContents.fromFrame(contents.mainFrame)).to.equal(contents);
    });
    it('returns undefined for disposed frame', async () => {
      const contents = (webContents as typeof ElectronInternal.WebContents).create();
      const { mainFrame } = contents;
      contents.destroy();
      await waitUntil(() => typeof webContents.fromFrame(mainFrame) === 'undefined');
    });
    it('throws when passing invalid argument', async () => {
      let errored = false;
      try {
        webContents.fromFrame({} as any);
      } catch {
        errored = true;
      }
      expect(errored).to.be.true();
    });
  });

  describe('fromDevToolsTargetId()', () => {
    it('returns WebContents for attached DevTools target', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');
      try {
        await w.webContents.debugger.attach('1.3');
        const { targetInfo } = await w.webContents.debugger.sendCommand('Target.getTargetInfo');
        expect(webContents.fromDevToolsTargetId(targetInfo.targetId)).to.equal(w.webContents);
      } finally {
        await w.webContents.debugger.detach();
      }
    });

    it('returns undefined for an unknown id', () => {
      expect(webContents.fromDevToolsTargetId('nope')).to.be.undefined();
    });
  });

  describe('will-prevent-unload event', function () {
    afterEach(closeAllWindows);
    it('does not emit if beforeunload returns undefined in a BrowserWindow', async () => {
      const w = new BrowserWindow({ show: false });
      w.webContents.once('will-prevent-unload', () => {
        expect.fail('should not have fired');
      });
      await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'beforeunload-undefined.html'));
      const wait = once(w, 'closed');
      w.close();
      await wait;
    });

    it('does not emit if beforeunload returns undefined in a BrowserView', async () => {
      const w = new BrowserWindow({ show: false });
      const view = new BrowserView();
      w.setBrowserView(view);
      view.setBounds(w.getBounds());

      view.webContents.once('will-prevent-unload', () => {
        expect.fail('should not have fired');
      });

      await view.webContents.loadFile(path.join(__dirname, 'fixtures', 'api', 'beforeunload-undefined.html'));
      const wait = once(w, 'closed');
      w.close();
      await wait;
    });

    it('emits if beforeunload returns false in a BrowserWindow', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'beforeunload-false.html'));
      w.close();
      await once(w.webContents, 'will-prevent-unload');
    });

    it('emits if beforeunload returns false in a BrowserView', async () => {
      const w = new BrowserWindow({ show: false });
      const view = new BrowserView();
      w.setBrowserView(view);
      view.setBounds(w.getBounds());

      await view.webContents.loadFile(path.join(__dirname, 'fixtures', 'api', 'beforeunload-false.html'));
      w.close();
      await once(view.webContents, 'will-prevent-unload');
    });

    it('supports calling preventDefault on will-prevent-unload events in a BrowserWindow', async () => {
      const w = new BrowserWindow({ show: false });
      w.webContents.once('will-prevent-unload', event => event.preventDefault());
      await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'beforeunload-false.html'));
      const wait = once(w, 'closed');
      w.close();
      await wait;
    });
  });

  describe('webContents.send(channel, args...)', () => {
    afterEach(closeAllWindows);
    it('throws an error when the channel is missing', () => {
      const w = new BrowserWindow({ show: false });
      expect(() => {
        (w.webContents.send as any)();
      }).to.throw('Missing required channel argument');

      expect(() => {
        w.webContents.send(null as any);
      }).to.throw('Missing required channel argument');
    });

    it('does not block node async APIs when sent before document is ready', (done) => {
      // Please reference https://github.com/electron/electron/issues/19368 if
      // this test fails.
      ipcMain.once('async-node-api-done', () => {
        done();
      });
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          sandbox: false,
          contextIsolation: false
        }
      });
      w.loadFile(path.join(fixturesPath, 'pages', 'send-after-node.html'));
      setTimeout(50).then(() => {
        w.webContents.send('test');
      });
    });
  });

  ifdescribe(features.isPrintingEnabled())('webContents.print()', () => {
    let w: BrowserWindow;

    beforeEach(() => {
      w = new BrowserWindow({ show: false });
    });

    afterEach(closeAllWindows);

    it('does not throw when options are not passed', () => {
      expect(() => {
        w.webContents.print();
      }).not.to.throw();

      expect(() => {
        w.webContents.print(undefined);
      }).not.to.throw();
    });

    it('does not throw when options object is empty', () => {
      expect(() => {
        w.webContents.print({});
      }).not.to.throw();
    });

    it('throws when invalid settings are passed', () => {
      expect(() => {
        // @ts-ignore this line is intentionally incorrect
        w.webContents.print(true);
      }).to.throw('webContents.print(): Invalid print settings specified.');

      expect(() => {
        // @ts-ignore this line is intentionally incorrect
        w.webContents.print(null);
      }).to.throw('webContents.print(): Invalid print settings specified.');
    });

    it('throws when an invalid pageSize is passed', () => {
      const badSize = 5;
      expect(() => {
        // @ts-ignore this line is intentionally incorrect
        w.webContents.print({ pageSize: badSize });
      }).to.throw(`Unsupported pageSize: ${badSize}`);
    });

    it('throws when an invalid callback is passed', () => {
      expect(() => {
        // @ts-ignore this line is intentionally incorrect
        w.webContents.print({}, true);
      }).to.throw('webContents.print(): Invalid optional callback provided.');
    });

    it('fails when an invalid deviceName is passed', (done) => {
      w.webContents.print({ deviceName: 'i-am-a-nonexistent-printer' }, (success, reason) => {
        expect(success).to.equal(false);
        expect(reason).to.match(/Invalid deviceName provided/);
        done();
      });
    });

    it('throws when an invalid pageSize is passed', () => {
      expect(() => {
        // @ts-ignore this line is intentionally incorrect
        w.webContents.print({ pageSize: 'i-am-a-bad-pagesize' }, () => {});
      }).to.throw('Unsupported pageSize: i-am-a-bad-pagesize');
    });

    it('throws when an invalid custom pageSize is passed', () => {
      expect(() => {
        w.webContents.print({
          pageSize: {
            width: 100,
            height: 200
          }
        });
      }).to.throw('height and width properties must be minimum 352 microns.');
    });

    it('does not crash with custom margins', () => {
      expect(() => {
        w.webContents.print({
          silent: true,
          margins: {
            marginType: 'custom',
            top: 1,
            bottom: 1,
            left: 1,
            right: 1
          }
        });
      }).to.not.throw();
    });
  });

  describe('webContents.executeJavaScript', () => {
    describe('in about:blank', () => {
      const expected = 'hello, world!';
      const expectedErrorMsg = 'woops!';
      const code = `(() => "${expected}")()`;
      const asyncCode = `(() => new Promise(r => setTimeout(() => r("${expected}"), 500)))()`;
      const badAsyncCode = `(() => new Promise((r, e) => setTimeout(() => e("${expectedErrorMsg}"), 500)))()`;
      const errorTypes = new Set([
        Error,
        ReferenceError,
        EvalError,
        RangeError,
        SyntaxError,
        TypeError,
        URIError
      ]);
      let w: BrowserWindow;

      before(async () => {
        w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: false } });
        await w.loadURL('about:blank');
      });
      after(closeAllWindows);

      it('resolves the returned promise with the result', async () => {
        const result = await w.webContents.executeJavaScript(code);
        expect(result).to.equal(expected);
      });
      it('resolves the returned promise with the result if the code returns an asynchronous promise', async () => {
        const result = await w.webContents.executeJavaScript(asyncCode);
        expect(result).to.equal(expected);
      });
      it('rejects the returned promise if an async error is thrown', async () => {
        await expect(w.webContents.executeJavaScript(badAsyncCode)).to.eventually.be.rejectedWith(expectedErrorMsg);
      });
      it('rejects the returned promise with an error if an Error.prototype is thrown', async () => {
        for (const error of errorTypes) {
          await expect(w.webContents.executeJavaScript(`Promise.reject(new ${error.name}("Wamp-wamp"))`))
            .to.eventually.be.rejectedWith(error);
        }
      });
    });

    describe('on a real page', () => {
      let w: BrowserWindow;
      beforeEach(() => {
        w = new BrowserWindow({ show: false });
      });
      afterEach(closeAllWindows);

      let server: http.Server;
      let serverUrl: string;

      before(async () => {
        server = http.createServer((request, response) => {
          response.end();
        });
        serverUrl = (await listen(server)).url;
      });

      after(() => {
        server.close();
        server = null as unknown as http.Server;
      });

      it('works after page load and during subframe load', async () => {
        await w.loadURL(serverUrl);
        // initiate a sub-frame load, then try and execute script during it
        await w.webContents.executeJavaScript(`
          var iframe = document.createElement('iframe')
          iframe.src = '${serverUrl}/slow'
          document.body.appendChild(iframe)
          null // don't return the iframe
        `);
        await w.webContents.executeJavaScript('console.log(\'hello\')');
      });

      it('executes after page load', async () => {
        const executeJavaScript = w.webContents.executeJavaScript('(() => "test")()');
        w.loadURL(serverUrl);
        const result = await executeJavaScript;
        expect(result).to.equal('test');
      });
    });
  });

  describe('webContents.executeJavaScriptInIsolatedWorld', () => {
    let w: BrowserWindow;

    before(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
      await w.loadURL('about:blank');
    });

    it('resolves the returned promise with the result', async () => {
      await w.webContents.executeJavaScriptInIsolatedWorld(999, [{ code: 'window.X = 123' }]);
      const isolatedResult = await w.webContents.executeJavaScriptInIsolatedWorld(999, [{ code: 'window.X' }]);
      const mainWorldResult = await w.webContents.executeJavaScript('window.X');
      expect(isolatedResult).to.equal(123);
      expect(mainWorldResult).to.equal(undefined);
    });
  });

  describe('loadURL() promise API', () => {
    let w: BrowserWindow;
    let s: http.Server;

    afterEach(() => {
      if (s) {
        s.close();
        s = null as unknown as http.Server;
      }
    });

    beforeEach(async () => {
      w = new BrowserWindow({ show: false });
    });
    afterEach(closeAllWindows);

    it('resolves when done loading', async () => {
      await expect(w.loadURL('about:blank')).to.eventually.be.fulfilled();
    });

    it('resolves when done loading a file URL', async () => {
      await expect(w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'))).to.eventually.be.fulfilled();
    });

    it('resolves when navigating within the page', async () => {
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));
      await setTimeout();
      await expect(w.loadURL(w.getURL() + '#foo')).to.eventually.be.fulfilled();
    });

    it('resolves after browser initiated navigation', async () => {
      let finishedLoading = false;
      w.webContents.on('did-finish-load', function () {
        finishedLoading = true;
      });

      await w.loadFile(path.join(fixturesPath, 'pages', 'navigate_in_page_and_wait.html'));
      expect(finishedLoading).to.be.true();
    });

    it('rejects when failing to load a file URL', async () => {
      await expect(w.loadURL('file:non-existent')).to.eventually.be.rejected()
        .and.have.property('code', 'ERR_FILE_NOT_FOUND');
    });

    // FIXME: Temporarily disable on WOA until
    // https://github.com/electron/electron/issues/20008 is resolved
    ifit(!(process.platform === 'win32' && process.arch === 'arm64'))('rejects when loading fails due to DNS not resolved', async () => {
      await expect(w.loadURL('https://err.name.not.resolved')).to.eventually.be.rejected()
        .and.have.property('code', 'ERR_NAME_NOT_RESOLVED');
    });

    it('rejects when navigation is cancelled due to a bad scheme', async () => {
      await expect(w.loadURL('bad-scheme://foo')).to.eventually.be.rejected()
        .and.have.property('code', 'ERR_FAILED');
    });

    it('does not crash when loading a new URL with emulation settings set', async () => {
      const setEmulation = async () => {
        if (w.webContents) {
          w.webContents.debugger.attach('1.3');

          const deviceMetrics = {
            width: 700,
            height: 600,
            deviceScaleFactor: 2,
            mobile: true,
            dontSetVisibleSize: true
          };
          await w.webContents.debugger.sendCommand(
            'Emulation.setDeviceMetricsOverride',
            deviceMetrics
          );
        }
      };

      try {
        await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
        await setEmulation();
        await w.loadURL('data:text/html,<h1>HELLO</h1>');
        await setEmulation();
      } catch (e) {
        expect((e as Error).message).to.match(/Debugger is already attached to the target/);
      }
    });

    it('fails if loadURL is called inside a non-reentrant critical section', (done) => {
      w.webContents.once('did-fail-load', (_event, _errorCode, _errorDescription, validatedURL) => {
        expect(validatedURL).to.contain('blank.html');
        done();
      });

      w.webContents.once('did-start-loading', () => {
        w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      });

      w.loadURL('data:text/html,<h1>HELLO</h1>');
    });

    it('sets appropriate error information on rejection', async () => {
      let err: any;
      try {
        await w.loadURL('file:non-existent');
      } catch (e) {
        err = e;
      }
      expect(err).not.to.be.null();
      expect(err.code).to.eql('ERR_FILE_NOT_FOUND');
      expect(err.errno).to.eql(-6);
      expect(err.url).to.eql(process.platform === 'win32' ? 'file://non-existent/' : 'file:///non-existent');
    });

    it('rejects if the load is aborted', async () => {
      s = http.createServer(() => { /* never complete the request */ });
      const { port } = await listen(s);
      const p = expect(w.loadURL(`http://127.0.0.1:${port}`)).to.eventually.be.rejectedWith(Error, /ERR_ABORTED/);
      // load a different file before the first load completes, causing the
      // first load to be aborted.
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));
      await p;
    });

    it("doesn't reject when a subframe fails to load", async () => {
      let resp = null as unknown as http.ServerResponse;
      s = http.createServer((req, res) => {
        res.writeHead(200, { 'Content-Type': 'text/html' });
        res.write('<iframe src="http://err.name.not.resolved"></iframe>');
        resp = res;
        // don't end the response yet
      });
      const { port } = await listen(s);
      const p = new Promise<void>(resolve => {
        w.webContents.on('did-fail-load', (event, errorCode, errorDescription, validatedURL, isMainFrame) => {
          if (!isMainFrame) {
            resolve();
          }
        });
      });
      const main = w.loadURL(`http://127.0.0.1:${port}`);
      await p;
      resp.end();
      await main;
    });

    it("doesn't resolve when a subframe loads", async () => {
      let resp = null as unknown as http.ServerResponse;
      s = http.createServer((req, res) => {
        res.writeHead(200, { 'Content-Type': 'text/html' });
        res.write('<iframe src="about:blank"></iframe>');
        resp = res;
        // don't end the response yet
      });
      const { port } = await listen(s);
      const p = new Promise<void>(resolve => {
        w.webContents.on('did-frame-finish-load', (event, isMainFrame) => {
          if (!isMainFrame) {
            resolve();
          }
        });
      });
      const main = w.loadURL(`http://127.0.0.1:${port}`);
      await p;
      resp.destroy(); // cause the main request to fail
      await expect(main).to.eventually.be.rejected()
        .and.have.property('errno', -355); // ERR_INCOMPLETE_CHUNKED_ENCODING
    });

    it('subsequent load failures reject each time', async () => {
      await expect(w.loadURL('file:non-existent')).to.eventually.be.rejected();
      await expect(w.loadURL('file:non-existent')).to.eventually.be.rejected();
    });

    it('invalid URL load rejects', async () => {
      await expect(w.loadURL('invalidURL')).to.eventually.be.rejected();
    });
  });

  describe('navigationHistory', () => {
    let w: BrowserWindow;
    const urlPage1 = 'data:text/html,<html><head><script>document.title = "Page 1";</script></head><body></body></html>';
    const urlPage2 = 'data:text/html,<html><head><script>document.title = "Page 2";</script></head><body></body></html>';
    const urlPage3 = 'data:text/html,<html><head><script>document.title = "Page 3";</script></head><body></body></html>';

    beforeEach(async () => {
      w = new BrowserWindow({ show: false });
    });
    afterEach(closeAllWindows);
    describe('navigationHistory.removeEntryAtIndex(index) API', () => {
      it('should remove a navigation entry given a valid index', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);
        const initialLength = w.webContents.navigationHistory.length();
        const wasRemoved = w.webContents.navigationHistory.removeEntryAtIndex(1); // Attempt to remove the second entry
        const newLength = w.webContents.navigationHistory.length();
        expect(wasRemoved).to.be.true();
        expect(newLength).to.equal(initialLength - 1);
      });

      it('should not remove the current active navigation entry', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        const activeIndex = w.webContents.navigationHistory.getActiveIndex();
        const wasRemoved = w.webContents.navigationHistory.removeEntryAtIndex(activeIndex);
        expect(wasRemoved).to.be.false();
      });

      it('should return false given an invalid index larger than history length', async () => {
        await w.loadURL(urlPage1);
        const wasRemoved = w.webContents.navigationHistory.removeEntryAtIndex(5); // Index larger than history length
        expect(wasRemoved).to.be.false();
      });

      it('should return false given an invalid negative index', async () => {
        await w.loadURL(urlPage1);
        const wasRemoved = w.webContents.navigationHistory.removeEntryAtIndex(-1); // Negative index
        expect(wasRemoved).to.be.false();
      });
    });

    describe('navigationHistory.canGoBack and navigationHistory.goBack API', () => {
      it('should not be able to go back if history is empty', async () => {
        expect(w.webContents.navigationHistory.canGoBack()).to.be.false();
      });

      it('should be able to go back if history is not empty', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(1);
        expect(w.webContents.navigationHistory.canGoBack()).to.be.true();
        w.webContents.navigationHistory.goBack();
        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(0);
      });
    });

    describe('navigationHistory.canGoForward and navigationHistory.goForward API', () => {
      it('should not be able to go forward if history is empty', async () => {
        expect(w.webContents.navigationHistory.canGoForward()).to.be.false();
      });

      it('should not be able to go forward if current index is same as history length', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        expect(w.webContents.navigationHistory.canGoForward()).to.be.false();
      });

      it('should be able to go forward if history is not empty and active index is less than history length', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        w.webContents.navigationHistory.goBack();
        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(0);
        expect(w.webContents.navigationHistory.canGoForward()).to.be.true();
        w.webContents.navigationHistory.goForward();
        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(1);
      });
    });

    describe('navigationHistory.canGoToOffset(index) and navigationHistory.goToOffset(index) API', () => {
      it('should not be able to go to invalid offset', async () => {
        expect(w.webContents.navigationHistory.canGoToOffset(-1)).to.be.false();
        expect(w.webContents.navigationHistory.canGoToOffset(10)).to.be.false();
      });

      it('should be able to go to valid negative offset', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);
        expect(w.webContents.navigationHistory.canGoToOffset(-2)).to.be.true();
        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(2);
        w.webContents.navigationHistory.goToOffset(-2);
        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(0);
      });

      it('should be able to go to valid positive offset', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);

        w.webContents.navigationHistory.goBack();
        expect(w.webContents.navigationHistory.canGoToOffset(1)).to.be.true();
        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(1);
        w.webContents.navigationHistory.goToOffset(1);
        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(2);
      });
    });

    describe('navigationHistory.clear API', () => {
      it('should be able clear history', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);

        expect(w.webContents.navigationHistory.length()).to.equal(3);
        w.webContents.navigationHistory.clear();
        expect(w.webContents.navigationHistory.length()).to.equal(1);
      });
    });

    describe('navigationHistory.getEntryAtIndex(index) API ', () => {
      it('should fetch default navigation entry when no urls are loaded', async () => {
        const result = w.webContents.navigationHistory.getEntryAtIndex(0);
        expect(result).to.deep.equal({ url: '', title: '' });
      });
      it('should fetch navigation entry given a valid index', async () => {
        await w.loadURL(urlPage1);
        const result = w.webContents.navigationHistory.getEntryAtIndex(0);
        expect(result).to.deep.equal({ url: urlPage1, title: 'Page 1' });
      });
      it('should return null given an invalid index larger than history length', async () => {
        await w.loadURL(urlPage1);
        const result = w.webContents.navigationHistory.getEntryAtIndex(5);
        expect(result).to.be.null();
      });
      it('should return null given an invalid negative index', async () => {
        await w.loadURL(urlPage1);
        const result = w.webContents.navigationHistory.getEntryAtIndex(-1);
        expect(result).to.be.null();
      });
    });

    describe('navigationHistory.getActiveIndex() API', () => {
      it('should return valid active index after a single page visit', async () => {
        await w.loadURL(urlPage1);
        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(0);
      });

      it('should return valid active index after a multiple page visits', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);

        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(2);
      });

      it('should return valid active index given no page visits', async () => {
        expect(w.webContents.navigationHistory.getActiveIndex()).to.equal(0);
      });
    });

    describe('navigationHistory.length() API', () => {
      it('should return valid history length after a single page visit', async () => {
        await w.loadURL(urlPage1);
        expect(w.webContents.navigationHistory.length()).to.equal(1);
      });

      it('should return valid history length after a multiple page visits', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);

        expect(w.webContents.navigationHistory.length()).to.equal(3);
      });

      it('should return valid history length given no page visits', async () => {
        // Note: Even if no navigation has committed, the history list will always start with an initial navigation entry
        // Ref: https://source.chromium.org/chromium/chromium/src/+/main:ceontent/public/browser/navigation_controller.h;l=381
        expect(w.webContents.navigationHistory.length()).to.equal(1);
      });
    });

    describe('navigationHistory.getAllEntries() API', () => {
      it('should return all navigation entries as an array of NavigationEntry objects', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);
        const entries = w.webContents.navigationHistory.getAllEntries();
        expect(entries.length).to.equal(3);
        expect(entries[0]).to.deep.equal({ url: urlPage1, title: 'Page 1' });
        expect(entries[1]).to.deep.equal({ url: urlPage2, title: 'Page 2' });
        expect(entries[2]).to.deep.equal({ url: urlPage3, title: 'Page 3' });
      });

      it('should return an empty array when there is no navigation history', async () => {
        const entries = w.webContents.navigationHistory.getAllEntries();
        expect(entries.length).to.equal(0);
      });
    });
  });
});

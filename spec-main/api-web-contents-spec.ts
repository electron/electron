import { expect } from 'chai';
import { AddressInfo } from 'net';
import * as path from 'path';
import * as fs from 'fs';
import * as http from 'http';
import { BrowserWindow, ipcMain, webContents, session, WebContents, app, BrowserView } from 'electron/main';
import { clipboard } from 'electron/common';
import { emittedOnce } from './events-helpers';
import { closeAllWindows } from './window-helpers';
import { ifdescribe, ifit, delay, defer } from './spec-helpers';

const pdfjs = require('pdfjs-dist');
const fixturesPath = path.resolve(__dirname, '..', 'spec', 'fixtures');
const mainFixturesPath = path.resolve(__dirname, 'fixtures');
const features = process._linkedBinding('electron_common_features');

describe('webContents module', () => {
  describe('getAllWebContents() API', () => {
    afterEach(closeAllWindows);
    it('returns an array of web contents', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: { webviewTag: true }
      });
      w.loadFile(path.join(fixturesPath, 'pages', 'webview-zoom-factor.html'));

      await emittedOnce(w.webContents, 'did-attach-webview');

      w.webContents.openDevTools();

      await emittedOnce(w.webContents, 'devtools-opened');

      const all = webContents.getAllWebContents().sort((a, b) => {
        return a.id - b.id;
      });

      expect(all).to.have.length(3);
      expect(all[0].getType()).to.equal('window');
      expect(all[all.length - 2].getType()).to.equal('webview');
      expect(all[all.length - 1].getType()).to.equal('remote');
    });
  });

  describe('fromId()', () => {
    it('returns undefined for an unknown id', () => {
      expect(webContents.fromId(12345)).to.be.undefined();
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
      const wait = emittedOnce(w, 'closed');
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
      const wait = emittedOnce(w, 'closed');
      w.close();
      await wait;
    });

    it('emits if beforeunload returns false in a BrowserWindow', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'beforeunload-false.html'));
      w.close();
      await emittedOnce(w.webContents, 'will-prevent-unload');
    });

    it('emits if beforeunload returns false in a BrowserView', async () => {
      const w = new BrowserWindow({ show: false });
      const view = new BrowserView();
      w.setBrowserView(view);
      view.setBounds(w.getBounds());

      await view.webContents.loadFile(path.join(__dirname, 'fixtures', 'api', 'beforeunload-false.html'));
      w.close();
      await emittedOnce(view.webContents, 'will-prevent-unload');
    });

    it('supports calling preventDefault on will-prevent-unload events in a BrowserWindow', async () => {
      const w = new BrowserWindow({ show: false });
      w.webContents.once('will-prevent-unload', event => event.preventDefault());
      await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'beforeunload-false.html'));
      const wait = emittedOnce(w, 'closed');
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
      setTimeout(() => {
        w.webContents.send('test');
      }, 50);
    });
  });

  ifdescribe(features.isPrintingEnabled())('webContents.print()', () => {
    let w: BrowserWindow;

    beforeEach(() => {
      w = new BrowserWindow({ show: false });
    });

    afterEach(closeAllWindows);

    it('throws when invalid settings are passed', () => {
      expect(() => {
        // @ts-ignore this line is intentionally incorrect
        w.webContents.print(true);
      }).to.throw('webContents.print(): Invalid print settings specified.');
    });

    it('throws when an invalid callback is passed', () => {
      expect(() => {
        // @ts-ignore this line is intentionally incorrect
        w.webContents.print({}, true);
      }).to.throw('webContents.print(): Invalid optional callback provided.');
    });

    ifit(process.platform !== 'linux')('throws when an invalid deviceName is passed', () => {
      expect(() => {
        w.webContents.print({ deviceName: 'i-am-a-nonexistent-printer' }, () => {});
      }).to.throw('webContents.print(): Invalid deviceName provided.');
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
      it('resolves the returned promise with the result if the code returns an asyncronous promise', async () => {
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

      let server: http.Server = null as unknown as http.Server;
      let serverUrl: string = null as unknown as string;

      before((done) => {
        server = http.createServer((request, response) => {
          response.end();
        }).listen(0, '127.0.0.1', () => {
          serverUrl = 'http://127.0.0.1:' + (server.address() as AddressInfo).port;
          done();
        });
      });

      after(() => {
        server.close();
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

  describe('webContents.getExtensionTabDetails', () => {
    let w: BrowserWindow;

    before(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true, sandbox: true } });
      await w.loadURL('about:blank');
    });

    after(() => {
      session.defaultSession.setExtensionAPIHandlers({
        getTab: null
      });
    });

    after(closeAllWindows);

    it('returns correct chrome tab details', async () => {
      session.defaultSession.setExtensionAPIHandlers({
        getTab: () => {
          return {
            windowId: w.id
          };
        }
      });
      const details = w.webContents.getExtensionTabDetails();
      expect(details?.url).to.equal('about:blank');
    });
  });

  describe('loadURL() promise API', () => {
    let w: BrowserWindow;
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

    it('rejects when failing to load a file URL', async () => {
      await expect(w.loadURL('file:non-existent')).to.eventually.be.rejected()
        .and.have.property('code', 'ERR_FILE_NOT_FOUND');
    });

    // Temporarily disable on WOA until
    // https://github.com/electron/electron/issues/20008 is resolved
    const testFn = (process.platform === 'win32' && process.arch === 'arm64' ? it.skip : it);
    testFn('rejects when loading fails due to DNS not resolved', async () => {
      await expect(w.loadURL('https://err.name.not.resolved')).to.eventually.be.rejected()
        .and.have.property('code', 'ERR_NAME_NOT_RESOLVED');
    });

    it('rejects when navigation is cancelled due to a bad scheme', async () => {
      await expect(w.loadURL('bad-scheme://foo')).to.eventually.be.rejected()
        .and.have.property('code', 'ERR_FAILED');
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
      const s = http.createServer(() => { /* never complete the request */ });
      await new Promise<void>(resolve => s.listen(0, '127.0.0.1', resolve));
      const { port } = s.address() as AddressInfo;
      const p = expect(w.loadURL(`http://127.0.0.1:${port}`)).to.eventually.be.rejectedWith(Error, /ERR_ABORTED/);
      // load a different file before the first load completes, causing the
      // first load to be aborted.
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));
      await p;
      s.close();
    });

    it("doesn't reject when a subframe fails to load", async () => {
      let resp = null as unknown as http.ServerResponse;
      const s = http.createServer((req, res) => {
        res.writeHead(200, { 'Content-Type': 'text/html' });
        res.write('<iframe src="http://err.name.not.resolved"></iframe>');
        resp = res;
        // don't end the response yet
      });
      await new Promise<void>(resolve => s.listen(0, '127.0.0.1', resolve));
      const { port } = s.address() as AddressInfo;
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
      s.close();
    });

    it("doesn't resolve when a subframe loads", async () => {
      let resp = null as unknown as http.ServerResponse;
      const s = http.createServer((req, res) => {
        res.writeHead(200, { 'Content-Type': 'text/html' });
        res.write('<iframe src="about:blank"></iframe>');
        resp = res;
        // don't end the response yet
      });
      await new Promise<void>(resolve => s.listen(0, '127.0.0.1', resolve));
      const { port } = s.address() as AddressInfo;
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
      s.close();
    });
  });

  describe('getFocusedWebContents() API', () => {
    afterEach(closeAllWindows);

    const testFn = (process.platform === 'win32' && process.arch === 'arm64' ? it.skip : it);
    testFn('returns the focused web contents', async () => {
      const w = new BrowserWindow({ show: true });
      await w.loadFile(path.join(__dirname, 'fixtures', 'blank.html'));
      expect(webContents.getFocusedWebContents().id).to.equal(w.webContents.id);

      const devToolsOpened = emittedOnce(w.webContents, 'devtools-opened');
      w.webContents.openDevTools();
      await devToolsOpened;
      expect(webContents.getFocusedWebContents().id).to.equal(w.webContents.devToolsWebContents!.id);
      const devToolsClosed = emittedOnce(w.webContents, 'devtools-closed');
      w.webContents.closeDevTools();
      await devToolsClosed;
      expect(webContents.getFocusedWebContents().id).to.equal(w.webContents.id);
    });

    it('does not crash when called on a detached dev tools window', async () => {
      const w = new BrowserWindow({ show: true });

      w.webContents.openDevTools({ mode: 'detach' });
      w.webContents.inspectElement(100, 100);

      // For some reason we have to wait for two focused events...?
      await emittedOnce(w.webContents, 'devtools-focused');

      expect(() => { webContents.getFocusedWebContents(); }).to.not.throw();

      // Work around https://github.com/electron/electron/issues/19985
      await delay();

      const devToolsClosed = emittedOnce(w.webContents, 'devtools-closed');
      w.webContents.closeDevTools();
      await devToolsClosed;
      expect(() => { webContents.getFocusedWebContents(); }).to.not.throw();
    });
  });

  describe('setDevToolsWebContents() API', () => {
    afterEach(closeAllWindows);
    it('sets arbitrary webContents as devtools', async () => {
      const w = new BrowserWindow({ show: false });
      const devtools = new BrowserWindow({ show: false });
      const promise = emittedOnce(devtools.webContents, 'dom-ready');
      w.webContents.setDevToolsWebContents(devtools.webContents);
      w.webContents.openDevTools();
      await promise;
      expect(devtools.webContents.getURL().startsWith('devtools://devtools')).to.be.true();
      const result = await devtools.webContents.executeJavaScript('InspectorFrontendHost.constructor.name');
      expect(result).to.equal('InspectorFrontendHostImpl');
      devtools.destroy();
    });
  });

  describe('isFocused() API', () => {
    it('returns false when the window is hidden', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');
      expect(w.isVisible()).to.be.false();
      expect(w.webContents.isFocused()).to.be.false();
    });
  });

  describe('isCurrentlyAudible() API', () => {
    afterEach(closeAllWindows);
    it('returns whether audio is playing', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');
      await w.webContents.executeJavaScript(`
        window.context = new AudioContext
        // Start in suspended state, because of the
        // new web audio api policy.
        context.suspend()
        window.oscillator = context.createOscillator()
        oscillator.connect(context.destination)
        oscillator.start()
      `);
      let p = emittedOnce(w.webContents, '-audio-state-changed');
      w.webContents.executeJavaScript('context.resume()');
      await p;
      expect(w.webContents.isCurrentlyAudible()).to.be.true();
      p = emittedOnce(w.webContents, '-audio-state-changed');
      w.webContents.executeJavaScript('oscillator.stop()');
      await p;
      expect(w.webContents.isCurrentlyAudible()).to.be.false();
    });
  });

  describe('openDevTools() API', () => {
    afterEach(closeAllWindows);
    it('can show window with activation', async () => {
      const w = new BrowserWindow({ show: false });
      const focused = emittedOnce(w, 'focus');
      w.show();
      await focused;
      expect(w.isFocused()).to.be.true();
      const blurred = emittedOnce(w, 'blur');
      w.webContents.openDevTools({ mode: 'detach', activate: true });
      await Promise.all([
        emittedOnce(w.webContents, 'devtools-opened'),
        emittedOnce(w.webContents, 'devtools-focused')
      ]);
      await blurred;
      expect(w.isFocused()).to.be.false();
    });

    it('can show window without activation', async () => {
      const w = new BrowserWindow({ show: false });
      const devtoolsOpened = emittedOnce(w.webContents, 'devtools-opened');
      w.webContents.openDevTools({ mode: 'detach', activate: false });
      await devtoolsOpened;
      expect(w.webContents.isDevToolsOpened()).to.be.true();
    });
  });

  describe('before-input-event event', () => {
    afterEach(closeAllWindows);
    it('can prevent document keyboard events', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      await w.loadFile(path.join(fixturesPath, 'pages', 'key-events.html'));
      const keyDown = new Promise(resolve => {
        ipcMain.once('keydown', (event, key) => resolve(key));
      });
      w.webContents.once('before-input-event', (event, input) => {
        if (input.key === 'a') event.preventDefault();
      });
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'a' });
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'b' });
      expect(await keyDown).to.equal('b');
    });

    it('has the correct properties', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));
      const testBeforeInput = async (opts: any) => {
        const modifiers = [];
        if (opts.shift) modifiers.push('shift');
        if (opts.control) modifiers.push('control');
        if (opts.alt) modifiers.push('alt');
        if (opts.meta) modifiers.push('meta');
        if (opts.isAutoRepeat) modifiers.push('isAutoRepeat');

        const p = emittedOnce(w.webContents, 'before-input-event');
        w.webContents.sendInputEvent({
          type: opts.type,
          keyCode: opts.keyCode,
          modifiers: modifiers as any
        });
        const [, input] = await p;

        expect(input.type).to.equal(opts.type);
        expect(input.key).to.equal(opts.key);
        expect(input.code).to.equal(opts.code);
        expect(input.isAutoRepeat).to.equal(opts.isAutoRepeat);
        expect(input.shift).to.equal(opts.shift);
        expect(input.control).to.equal(opts.control);
        expect(input.alt).to.equal(opts.alt);
        expect(input.meta).to.equal(opts.meta);
      };
      await testBeforeInput({
        type: 'keyDown',
        key: 'A',
        code: 'KeyA',
        keyCode: 'a',
        shift: true,
        control: true,
        alt: true,
        meta: true,
        isAutoRepeat: true
      });
      await testBeforeInput({
        type: 'keyUp',
        key: '.',
        code: 'Period',
        keyCode: '.',
        shift: false,
        control: true,
        alt: true,
        meta: false,
        isAutoRepeat: false
      });
      await testBeforeInput({
        type: 'keyUp',
        key: '!',
        code: 'Digit1',
        keyCode: '1',
        shift: true,
        control: false,
        alt: false,
        meta: true,
        isAutoRepeat: false
      });
      await testBeforeInput({
        type: 'keyUp',
        key: 'Tab',
        code: 'Tab',
        keyCode: 'Tab',
        shift: false,
        control: true,
        alt: false,
        meta: false,
        isAutoRepeat: true
      });
    });
  });

  // On Mac, zooming isn't done with the mouse wheel.
  ifdescribe(process.platform !== 'darwin')('zoom-changed', () => {
    afterEach(closeAllWindows);
    it('is emitted with the correct zoom-in info', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));

      const testZoomChanged = async () => {
        w.webContents.sendInputEvent({
          type: 'mouseWheel',
          x: 300,
          y: 300,
          deltaX: 0,
          deltaY: 1,
          wheelTicksX: 0,
          wheelTicksY: 1,
          modifiers: ['control', 'meta']
        });

        const [, zoomDirection] = await emittedOnce(w.webContents, 'zoom-changed');
        expect(zoomDirection).to.equal('in');
      };

      await testZoomChanged();
    });

    it('is emitted with the correct zoom-out info', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));

      const testZoomChanged = async () => {
        w.webContents.sendInputEvent({
          type: 'mouseWheel',
          x: 300,
          y: 300,
          deltaX: 0,
          deltaY: -1,
          wheelTicksX: 0,
          wheelTicksY: -1,
          modifiers: ['control', 'meta']
        });

        const [, zoomDirection] = await emittedOnce(w.webContents, 'zoom-changed');
        expect(zoomDirection).to.equal('out');
      };

      await testZoomChanged();
    });
  });

  describe('sendInputEvent(event)', () => {
    let w: BrowserWindow;
    beforeEach(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      await w.loadFile(path.join(fixturesPath, 'pages', 'key-events.html'));
    });
    afterEach(closeAllWindows);

    it('can send keydown events', async () => {
      const keydown = emittedOnce(ipcMain, 'keydown');
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'A' });
      const [, key, code, keyCode, shiftKey, ctrlKey, altKey] = await keydown;
      expect(key).to.equal('a');
      expect(code).to.equal('KeyA');
      expect(keyCode).to.equal(65);
      expect(shiftKey).to.be.false();
      expect(ctrlKey).to.be.false();
      expect(altKey).to.be.false();
    });

    it('can send keydown events with modifiers', async () => {
      const keydown = emittedOnce(ipcMain, 'keydown');
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Z', modifiers: ['shift', 'ctrl'] });
      const [, key, code, keyCode, shiftKey, ctrlKey, altKey] = await keydown;
      expect(key).to.equal('Z');
      expect(code).to.equal('KeyZ');
      expect(keyCode).to.equal(90);
      expect(shiftKey).to.be.true();
      expect(ctrlKey).to.be.true();
      expect(altKey).to.be.false();
    });

    it('can send keydown events with special keys', async () => {
      const keydown = emittedOnce(ipcMain, 'keydown');
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Tab', modifiers: ['alt'] });
      const [, key, code, keyCode, shiftKey, ctrlKey, altKey] = await keydown;
      expect(key).to.equal('Tab');
      expect(code).to.equal('Tab');
      expect(keyCode).to.equal(9);
      expect(shiftKey).to.be.false();
      expect(ctrlKey).to.be.false();
      expect(altKey).to.be.true();
    });

    it('can send char events', async () => {
      const keypress = emittedOnce(ipcMain, 'keypress');
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'A' });
      w.webContents.sendInputEvent({ type: 'char', keyCode: 'A' });
      const [, key, code, keyCode, shiftKey, ctrlKey, altKey] = await keypress;
      expect(key).to.equal('a');
      expect(code).to.equal('KeyA');
      expect(keyCode).to.equal(65);
      expect(shiftKey).to.be.false();
      expect(ctrlKey).to.be.false();
      expect(altKey).to.be.false();
    });

    it('can send char events with modifiers', async () => {
      const keypress = emittedOnce(ipcMain, 'keypress');
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Z' });
      w.webContents.sendInputEvent({ type: 'char', keyCode: 'Z', modifiers: ['shift', 'ctrl'] });
      const [, key, code, keyCode, shiftKey, ctrlKey, altKey] = await keypress;
      expect(key).to.equal('Z');
      expect(code).to.equal('KeyZ');
      expect(keyCode).to.equal(90);
      expect(shiftKey).to.be.true();
      expect(ctrlKey).to.be.true();
      expect(altKey).to.be.false();
    });
  });

  describe('insertCSS', () => {
    afterEach(closeAllWindows);
    it('supports inserting CSS', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadURL('about:blank');
      await w.webContents.insertCSS('body { background-repeat: round; }');
      const result = await w.webContents.executeJavaScript('window.getComputedStyle(document.body).getPropertyValue("background-repeat")');
      expect(result).to.equal('round');
    });

    it('supports removing inserted CSS', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadURL('about:blank');
      const key = await w.webContents.insertCSS('body { background-repeat: round; }');
      await w.webContents.removeInsertedCSS(key);
      const result = await w.webContents.executeJavaScript('window.getComputedStyle(document.body).getPropertyValue("background-repeat")');
      expect(result).to.equal('repeat');
    });
  });

  describe('inspectElement()', () => {
    afterEach(closeAllWindows);
    it('supports inspecting an element in the devtools', (done) => {
      const w = new BrowserWindow({ show: false });
      w.loadURL('about:blank');
      w.webContents.once('devtools-opened', () => { done(); });
      w.webContents.inspectElement(10, 10);
    });
  });

  describe('startDrag({file, icon})', () => {
    it('throws errors for a missing file or a missing/empty icon', () => {
      const w = new BrowserWindow({ show: false });
      expect(() => {
        w.webContents.startDrag({ icon: path.join(fixturesPath, 'assets', 'logo.png') } as any);
      }).to.throw('Must specify either \'file\' or \'files\' option');

      expect(() => {
        w.webContents.startDrag({ file: __filename } as any);
      }).to.throw('\'icon\' parameter is required');

      expect(() => {
        w.webContents.startDrag({ file: __filename, icon: path.join(mainFixturesPath, 'blank.png') });
      }).to.throw(/Failed to load image from path (.+)/);
    });
  });

  describe('focus APIs', () => {
    describe('focus()', () => {
      afterEach(closeAllWindows);
      it('does not blur the focused window when the web contents is hidden', async () => {
        const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } });
        w.show();
        await w.loadURL('about:blank');
        w.focus();
        const child = new BrowserWindow({ show: false });
        child.loadURL('about:blank');
        child.webContents.focus();
        const currentFocused = w.isFocused();
        const childFocused = child.isFocused();
        child.close();
        expect(currentFocused).to.be.true();
        expect(childFocused).to.be.false();
      });
    });

    const moveFocusToDevTools = async (win: BrowserWindow) => {
      const devToolsOpened = emittedOnce(win.webContents, 'devtools-opened');
      win.webContents.openDevTools({ mode: 'right' });
      await devToolsOpened;
      win.webContents.devToolsWebContents!.focus();
    };

    describe('focus event', () => {
      afterEach(closeAllWindows);

      it('is triggered when web contents is focused', async () => {
        const w = new BrowserWindow({ show: false });
        await w.loadURL('about:blank');
        await moveFocusToDevTools(w);
        const focusPromise = emittedOnce(w.webContents, 'focus');
        w.webContents.focus();
        await expect(focusPromise).to.eventually.be.fulfilled();
      });

      it('is triggered when BrowserWindow is focused', async () => {
        const window1 = new BrowserWindow({ show: false });
        const window2 = new BrowserWindow({ show: false });

        await Promise.all([
          window1.loadURL('about:blank'),
          window2.loadURL('about:blank')
        ]);

        const focusPromise1 = emittedOnce(window1.webContents, 'focus');
        const focusPromise2 = emittedOnce(window2.webContents, 'focus');

        window1.showInactive();
        window2.showInactive();

        window1.focus();
        await expect(focusPromise1).to.eventually.be.fulfilled();

        window2.focus();
        await expect(focusPromise2).to.eventually.be.fulfilled();
      });
    });

    describe('blur event', () => {
      afterEach(closeAllWindows);
      it('is triggered when web contents is blurred', async () => {
        const w = new BrowserWindow({ show: true });
        await w.loadURL('about:blank');
        w.webContents.focus();
        const blurPromise = emittedOnce(w.webContents, 'blur');
        await moveFocusToDevTools(w);
        await expect(blurPromise).to.eventually.be.fulfilled();
      });
    });
  });

  describe('getOSProcessId()', () => {
    afterEach(closeAllWindows);
    it('returns a valid procress id', async () => {
      const w = new BrowserWindow({ show: false });
      expect(w.webContents.getOSProcessId()).to.equal(0);

      await w.loadURL('about:blank');
      expect(w.webContents.getOSProcessId()).to.be.above(0);
    });
  });

  describe('getMediaSourceId()', () => {
    afterEach(closeAllWindows);
    it('returns a valid stream id', () => {
      const w = new BrowserWindow({ show: false });
      expect(w.webContents.getMediaSourceId(w.webContents)).to.be.a('string').that.is.not.empty();
    });
  });

  describe('userAgent APIs', () => {
    it('can set the user agent (functions)', () => {
      const w = new BrowserWindow({ show: false });
      const userAgent = w.webContents.getUserAgent();

      w.webContents.setUserAgent('my-user-agent');
      expect(w.webContents.getUserAgent()).to.equal('my-user-agent');

      w.webContents.setUserAgent(userAgent);
      expect(w.webContents.getUserAgent()).to.equal(userAgent);
    });

    it('can set the user agent (properties)', () => {
      const w = new BrowserWindow({ show: false });
      const userAgent = w.webContents.userAgent;

      w.webContents.userAgent = 'my-user-agent';
      expect(w.webContents.userAgent).to.equal('my-user-agent');

      w.webContents.userAgent = userAgent;
      expect(w.webContents.userAgent).to.equal(userAgent);
    });
  });

  describe('audioMuted APIs', () => {
    it('can set the audio mute level (functions)', () => {
      const w = new BrowserWindow({ show: false });

      w.webContents.setAudioMuted(true);
      expect(w.webContents.isAudioMuted()).to.be.true();

      w.webContents.setAudioMuted(false);
      expect(w.webContents.isAudioMuted()).to.be.false();
    });

    it('can set the audio mute level (functions)', () => {
      const w = new BrowserWindow({ show: false });

      w.webContents.audioMuted = true;
      expect(w.webContents.audioMuted).to.be.true();

      w.webContents.audioMuted = false;
      expect(w.webContents.audioMuted).to.be.false();
    });
  });

  describe('zoom api', () => {
    const hostZoomMap: Record<string, number> = {
      host1: 0.3,
      host2: 0.7,
      host3: 0.2
    };

    before(() => {
      const protocol = session.defaultSession.protocol;
      protocol.registerStringProtocol(standardScheme, (request, callback) => {
        const response = `<script>
                            const {ipcRenderer} = require('electron')
                            ipcRenderer.send('set-zoom', window.location.hostname)
                            ipcRenderer.on(window.location.hostname + '-zoom-set', () => {
                              ipcRenderer.send(window.location.hostname + '-zoom-level')
                            })
                          </script>`;
        callback({ data: response, mimeType: 'text/html' });
      });
    });

    after(() => {
      const protocol = session.defaultSession.protocol;
      protocol.unregisterProtocol(standardScheme);
    });

    afterEach(closeAllWindows);

    it('throws on an invalid zoomFactor', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');

      expect(() => {
        w.webContents.setZoomFactor(0.0);
      }).to.throw(/'zoomFactor' must be a double greater than 0.0/);

      expect(() => {
        w.webContents.setZoomFactor(-2.0);
      }).to.throw(/'zoomFactor' must be a double greater than 0.0/);
    });

    it('can set the correct zoom level (functions)', async () => {
      const w = new BrowserWindow({ show: false });
      try {
        await w.loadURL('about:blank');
        const zoomLevel = w.webContents.getZoomLevel();
        expect(zoomLevel).to.eql(0.0);
        w.webContents.setZoomLevel(0.5);
        const newZoomLevel = w.webContents.getZoomLevel();
        expect(newZoomLevel).to.eql(0.5);
      } finally {
        w.webContents.setZoomLevel(0);
      }
    });

    it('can set the correct zoom level (properties)', async () => {
      const w = new BrowserWindow({ show: false });
      try {
        await w.loadURL('about:blank');
        const zoomLevel = w.webContents.zoomLevel;
        expect(zoomLevel).to.eql(0.0);
        w.webContents.zoomLevel = 0.5;
        const newZoomLevel = w.webContents.zoomLevel;
        expect(newZoomLevel).to.eql(0.5);
      } finally {
        w.webContents.zoomLevel = 0;
      }
    });

    it('can set the correct zoom factor (functions)', async () => {
      const w = new BrowserWindow({ show: false });
      try {
        await w.loadURL('about:blank');
        const zoomFactor = w.webContents.getZoomFactor();
        expect(zoomFactor).to.eql(1.0);

        w.webContents.setZoomFactor(0.5);
        const newZoomFactor = w.webContents.getZoomFactor();
        expect(newZoomFactor).to.eql(0.5);
      } finally {
        w.webContents.setZoomFactor(1.0);
      }
    });

    it('can set the correct zoom factor (properties)', async () => {
      const w = new BrowserWindow({ show: false });
      try {
        await w.loadURL('about:blank');
        const zoomFactor = w.webContents.zoomFactor;
        expect(zoomFactor).to.eql(1.0);

        w.webContents.zoomFactor = 0.5;
        const newZoomFactor = w.webContents.zoomFactor;
        expect(newZoomFactor).to.eql(0.5);
      } finally {
        w.webContents.zoomFactor = 1.0;
      }
    });

    it('can persist zoom level across navigation', (done) => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      let finalNavigation = false;
      ipcMain.on('set-zoom', (e, host) => {
        const zoomLevel = hostZoomMap[host];
        if (!finalNavigation) w.webContents.zoomLevel = zoomLevel;
        e.sender.send(`${host}-zoom-set`);
      });
      ipcMain.on('host1-zoom-level', (e) => {
        try {
          const zoomLevel = e.sender.getZoomLevel();
          const expectedZoomLevel = hostZoomMap.host1;
          expect(zoomLevel).to.equal(expectedZoomLevel);
          if (finalNavigation) {
            done();
          } else {
            w.loadURL(`${standardScheme}://host2`);
          }
        } catch (e) {
          done(e);
        }
      });
      ipcMain.once('host2-zoom-level', (e) => {
        try {
          const zoomLevel = e.sender.getZoomLevel();
          const expectedZoomLevel = hostZoomMap.host2;
          expect(zoomLevel).to.equal(expectedZoomLevel);
          finalNavigation = true;
          w.webContents.goBack();
        } catch (e) {
          done(e);
        }
      });
      w.loadURL(`${standardScheme}://host1`);
    });

    it('can propagate zoom level across same session', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } });
      const w2 = new BrowserWindow({ show: false });

      defer(() => {
        w2.setClosable(true);
        w2.close();
      });

      await w.loadURL(`${standardScheme}://host3`);
      w.webContents.zoomLevel = hostZoomMap.host3;

      await w2.loadURL(`${standardScheme}://host3`);
      const zoomLevel1 = w.webContents.zoomLevel;
      expect(zoomLevel1).to.equal(hostZoomMap.host3);

      const zoomLevel2 = w2.webContents.zoomLevel;
      expect(zoomLevel1).to.equal(zoomLevel2);
    });

    it('cannot propagate zoom level across different session', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } });
      const w2 = new BrowserWindow({
        show: false,
        webPreferences: {
          partition: 'temp'
        }
      });
      const protocol = w2.webContents.session.protocol;
      protocol.registerStringProtocol(standardScheme, (request, callback) => {
        callback('hello');
      });

      defer(() => {
        w2.setClosable(true);
        w2.close();

        protocol.unregisterProtocol(standardScheme);
      });

      await w.loadURL(`${standardScheme}://host3`);
      w.webContents.zoomLevel = hostZoomMap.host3;

      await w2.loadURL(`${standardScheme}://host3`);
      const zoomLevel1 = w.webContents.zoomLevel;
      expect(zoomLevel1).to.equal(hostZoomMap.host3);

      const zoomLevel2 = w2.webContents.zoomLevel;
      expect(zoomLevel2).to.equal(0);
      expect(zoomLevel1).to.not.equal(zoomLevel2);
    });

    it('can persist when it contains iframe', (done) => {
      const w = new BrowserWindow({ show: false });
      const server = http.createServer((req, res) => {
        setTimeout(() => {
          res.end();
        }, 200);
      });
      server.listen(0, '127.0.0.1', () => {
        const url = 'http://127.0.0.1:' + (server.address() as AddressInfo).port;
        const content = `<iframe src=${url}></iframe>`;
        w.webContents.on('did-frame-finish-load', (e, isMainFrame) => {
          if (!isMainFrame) {
            try {
              const zoomLevel = w.webContents.zoomLevel;
              expect(zoomLevel).to.equal(2.0);

              w.webContents.zoomLevel = 0;
              done();
            } catch (e) {
              done(e);
            } finally {
              server.close();
            }
          }
        });
        w.webContents.on('dom-ready', () => {
          w.webContents.zoomLevel = 2.0;
        });
        w.loadURL(`data:text/html,${content}`);
      });
    });

    it('cannot propagate when used with webframe', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      const w2 = new BrowserWindow({ show: false });

      const temporaryZoomSet = emittedOnce(ipcMain, 'temporary-zoom-set');
      w.loadFile(path.join(fixturesPath, 'pages', 'webframe-zoom.html'));
      await temporaryZoomSet;

      const finalZoomLevel = w.webContents.getZoomLevel();
      await w2.loadFile(path.join(fixturesPath, 'pages', 'c.html'));
      const zoomLevel1 = w.webContents.zoomLevel;
      const zoomLevel2 = w2.webContents.zoomLevel;

      w2.setClosable(true);
      w2.close();

      expect(zoomLevel1).to.equal(finalZoomLevel);
      expect(zoomLevel2).to.equal(0);
      expect(zoomLevel1).to.not.equal(zoomLevel2);
    });

    describe('with unique domains', () => {
      let server: http.Server;
      let serverUrl: string;
      let crossSiteUrl: string;

      before((done) => {
        server = http.createServer((req, res) => {
          setTimeout(() => res.end('hey'), 0);
        });
        server.listen(0, '127.0.0.1', () => {
          serverUrl = `http://127.0.0.1:${(server.address() as AddressInfo).port}`;
          crossSiteUrl = `http://localhost:${(server.address() as AddressInfo).port}`;
          done();
        });
      });

      after(() => {
        server.close();
      });

      it('cannot persist zoom level after navigation with webFrame', async () => {
        const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
        const source = `
          const {ipcRenderer, webFrame} = require('electron')
          webFrame.setZoomLevel(0.6)
          ipcRenderer.send('zoom-level-set', webFrame.getZoomLevel())
        `;
        const zoomLevelPromise = emittedOnce(ipcMain, 'zoom-level-set');
        await w.loadURL(serverUrl);
        await w.webContents.executeJavaScript(source);
        let [, zoomLevel] = await zoomLevelPromise;
        expect(zoomLevel).to.equal(0.6);
        const loadPromise = emittedOnce(w.webContents, 'did-finish-load');
        await w.loadURL(crossSiteUrl);
        await loadPromise;
        zoomLevel = w.webContents.zoomLevel;
        expect(zoomLevel).to.equal(0);
      });
    });
  });

  describe('webrtc ip policy api', () => {
    afterEach(closeAllWindows);
    it('can set and get webrtc ip policies', () => {
      const w = new BrowserWindow({ show: false });
      const policies = [
        'default',
        'default_public_interface_only',
        'default_public_and_private_interfaces',
        'disable_non_proxied_udp'
      ];
      policies.forEach((policy) => {
        w.webContents.setWebRTCIPHandlingPolicy(policy as any);
        expect(w.webContents.getWebRTCIPHandlingPolicy()).to.equal(policy);
      });
    });
  });

  describe('render view deleted events', () => {
    let server: http.Server;
    let serverUrl: string;
    let crossSiteUrl: string;

    before((done) => {
      server = http.createServer((req, res) => {
        const respond = () => {
          if (req.url === '/redirect-cross-site') {
            res.setHeader('Location', `${crossSiteUrl}/redirected`);
            res.statusCode = 302;
            res.end();
          } else if (req.url === '/redirected') {
            res.end('<html><script>window.localStorage</script></html>');
          } else if (req.url === '/first-window-open') {
            res.end(`<html><script>window.open('${serverUrl}/second-window-open', 'first child');</script></html>`);
          } else if (req.url === '/second-window-open') {
            res.end('<html><script>window.open(\'wrong://url\', \'second child\');</script></html>');
          } else {
            res.end();
          }
        };
        setTimeout(respond, 0);
      });
      server.listen(0, '127.0.0.1', () => {
        serverUrl = `http://127.0.0.1:${(server.address() as AddressInfo).port}`;
        crossSiteUrl = `http://localhost:${(server.address() as AddressInfo).port}`;
        done();
      });
    });

    after(() => {
      server.close();
    });

    afterEach(closeAllWindows);

    it('does not emit current-render-view-deleted when speculative RVHs are deleted', async () => {
      const w = new BrowserWindow({ show: false });
      let currentRenderViewDeletedEmitted = false;
      const renderViewDeletedHandler = () => {
        currentRenderViewDeletedEmitted = true;
      };
      w.webContents.on('current-render-view-deleted' as any, renderViewDeletedHandler);
      w.webContents.on('did-finish-load', () => {
        w.webContents.removeListener('current-render-view-deleted' as any, renderViewDeletedHandler);
        w.close();
      });
      const destroyed = emittedOnce(w.webContents, 'destroyed');
      w.loadURL(`${serverUrl}/redirect-cross-site`);
      await destroyed;
      expect(currentRenderViewDeletedEmitted).to.be.false('current-render-view-deleted was emitted');
    });

    it('does not emit current-render-view-deleted when speculative RVHs are deleted', async () => {
      const parentWindow = new BrowserWindow({ show: false });
      let currentRenderViewDeletedEmitted = false;
      let childWindow: BrowserWindow | null = null;
      const destroyed = emittedOnce(parentWindow.webContents, 'destroyed');
      const renderViewDeletedHandler = () => {
        currentRenderViewDeletedEmitted = true;
      };
      const childWindowCreated = new Promise<void>((resolve) => {
        app.once('browser-window-created', (event, window) => {
          childWindow = window;
          window.webContents.on('current-render-view-deleted' as any, renderViewDeletedHandler);
          resolve();
        });
      });
      parentWindow.loadURL(`${serverUrl}/first-window-open`);
      await childWindowCreated;
      childWindow!.webContents.removeListener('current-render-view-deleted' as any, renderViewDeletedHandler);
      parentWindow.close();
      await destroyed;
      expect(currentRenderViewDeletedEmitted).to.be.false('child window was destroyed');
    });

    it('emits current-render-view-deleted if the current RVHs are deleted', async () => {
      const w = new BrowserWindow({ show: false });
      let currentRenderViewDeletedEmitted = false;
      w.webContents.on('current-render-view-deleted' as any, () => {
        currentRenderViewDeletedEmitted = true;
      });
      w.webContents.on('did-finish-load', () => {
        w.close();
      });
      const destroyed = emittedOnce(w.webContents, 'destroyed');
      w.loadURL(`${serverUrl}/redirect-cross-site`);
      await destroyed;
      expect(currentRenderViewDeletedEmitted).to.be.true('current-render-view-deleted wasn\'t emitted');
    });

    it('emits render-view-deleted if any RVHs are deleted', async () => {
      const w = new BrowserWindow({ show: false });
      let rvhDeletedCount = 0;
      w.webContents.on('render-view-deleted' as any, () => {
        rvhDeletedCount++;
      });
      w.webContents.on('did-finish-load', () => {
        w.close();
      });
      const destroyed = emittedOnce(w.webContents, 'destroyed');
      w.loadURL(`${serverUrl}/redirect-cross-site`);
      await destroyed;
      const expectedRenderViewDeletedEventCount = 1;
      expect(rvhDeletedCount).to.equal(expectedRenderViewDeletedEventCount, 'render-view-deleted wasn\'t emitted the expected nr. of times');
    });
  });

  describe('setIgnoreMenuShortcuts(ignore)', () => {
    afterEach(closeAllWindows);
    it('does not throw', () => {
      const w = new BrowserWindow({ show: false });
      expect(() => {
        w.webContents.setIgnoreMenuShortcuts(true);
        w.webContents.setIgnoreMenuShortcuts(false);
      }).to.not.throw();
    });
  });

  const crashPrefs = [
    {
      nodeIntegration: true
    },
    {
      sandbox: true
    }
  ];

  const nicePrefs = (o: any) => {
    let s = '';
    for (const key of Object.keys(o)) {
      s += `${key}=${o[key]}, `;
    }
    return `(${s.slice(0, s.length - 2)})`;
  };

  for (const prefs of crashPrefs) {
    describe(`crash  with webPreferences ${nicePrefs(prefs)}`, () => {
      let w: BrowserWindow;
      beforeEach(async () => {
        w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } });
        await w.loadURL('about:blank');
      });
      afterEach(closeAllWindows);

      it('isCrashed() is false by default', () => {
        expect(w.webContents.isCrashed()).to.equal(false);
      });

      it('forcefullyCrashRenderer() crashes the process with reason=killed||crashed', async () => {
        expect(w.webContents.isCrashed()).to.equal(false);
        const crashEvent = emittedOnce(w.webContents, 'render-process-gone');
        w.webContents.forcefullyCrashRenderer();
        const [, details] = await crashEvent;
        expect(details.reason === 'killed' || details.reason === 'crashed').to.equal(true, 'reason should be killed || crashed');
        expect(w.webContents.isCrashed()).to.equal(true);
      });

      it('a crashed process is recoverable with reload()', async () => {
        expect(w.webContents.isCrashed()).to.equal(false);
        w.webContents.forcefullyCrashRenderer();
        w.webContents.reload();
        expect(w.webContents.isCrashed()).to.equal(false);
      });
    });
  }

  // Destroying webContents in its event listener is going to crash when
  // Electron is built in Debug mode.
  describe('destroy()', () => {
    let server: http.Server;
    let serverUrl: string;

    before((done) => {
      server = http.createServer((request, response) => {
        switch (request.url) {
          case '/net-error':
            response.destroy();
            break;
          case '/200':
            response.end();
            break;
          default:
            done('unsupported endpoint');
        }
      }).listen(0, '127.0.0.1', () => {
        serverUrl = 'http://127.0.0.1:' + (server.address() as AddressInfo).port;
        done();
      });
    });

    after(() => {
      server.close();
    });

    const events = [
      { name: 'did-start-loading', url: '/200' },
      { name: 'dom-ready', url: '/200' },
      { name: 'did-stop-loading', url: '/200' },
      { name: 'did-finish-load', url: '/200' },
      // FIXME: Multiple Emit calls inside an observer assume that object
      // will be alive till end of the observer. Synchronous `destroy` api
      // violates this contract and crashes.
      { name: 'did-frame-finish-load', url: '/200' },
      { name: 'did-fail-load', url: '/net-error' }
    ];
    for (const e of events) {
      it(`should not crash when invoked synchronously inside ${e.name} handler`, async function () {
        // This test is flaky on Windows CI and we don't know why, but the
        // purpose of this test is to make sure Electron does not crash so it
        // is fine to retry this test for a few times.
        this.retries(3);

        const contents = (webContents as any).create() as WebContents;
        const originalEmit = contents.emit.bind(contents);
        contents.emit = (...args) => { return originalEmit(...args); };
        contents.once(e.name as any, () => (contents as any).destroy());
        const destroyed = emittedOnce(contents, 'destroyed');
        contents.loadURL(serverUrl + e.url);
        await destroyed;
      });
    }
  });

  describe('did-change-theme-color event', () => {
    afterEach(closeAllWindows);
    it('is triggered with correct theme color', (done) => {
      const w = new BrowserWindow({ show: true });
      let count = 0;
      w.webContents.on('did-change-theme-color', (e, color) => {
        try {
          if (count === 0) {
            count += 1;
            expect(color).to.equal('#FFEEDD');
            w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));
          } else if (count === 1) {
            expect(color).to.be.null();
            done();
          }
        } catch (e) {
          done(e);
        }
      });
      w.loadFile(path.join(fixturesPath, 'pages', 'theme-color.html'));
    });
  });

  describe('console-message event', () => {
    afterEach(closeAllWindows);
    it('is triggered with correct log message', (done) => {
      const w = new BrowserWindow({ show: true });
      w.webContents.on('console-message', (e, level, message) => {
        // Don't just assert as Chromium might emit other logs that we should ignore.
        if (message === 'a') {
          done();
        }
      });
      w.loadFile(path.join(fixturesPath, 'pages', 'a.html'));
    });
  });

  describe('ipc-message event', () => {
    afterEach(closeAllWindows);
    it('emits when the renderer process sends an asynchronous message', async () => {
      const w = new BrowserWindow({ show: true, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      await w.webContents.loadURL('about:blank');
      w.webContents.executeJavaScript(`
        require('electron').ipcRenderer.send('message', 'Hello World!')
      `);

      const [, channel, message] = await emittedOnce(w.webContents, 'ipc-message');
      expect(channel).to.equal('message');
      expect(message).to.equal('Hello World!');
    });
  });

  describe('ipc-message-sync event', () => {
    afterEach(closeAllWindows);
    it('emits when the renderer process sends a synchronous message', async () => {
      const w = new BrowserWindow({ show: true, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      await w.webContents.loadURL('about:blank');
      const promise: Promise<[string, string]> = new Promise(resolve => {
        w.webContents.once('ipc-message-sync', (event, channel, arg) => {
          event.returnValue = 'foobar' as any;
          resolve([channel, arg]);
        });
      });
      const result = await w.webContents.executeJavaScript(`
        require('electron').ipcRenderer.sendSync('message', 'Hello World!')
      `);

      const [channel, message] = await promise;
      expect(channel).to.equal('message');
      expect(message).to.equal('Hello World!');
      expect(result).to.equal('foobar');
    });
  });

  describe('referrer', () => {
    afterEach(closeAllWindows);
    it('propagates referrer information to new target=_blank windows', (done) => {
      const w = new BrowserWindow({ show: false });
      const server = http.createServer((req, res) => {
        if (req.url === '/should_have_referrer') {
          try {
            expect(req.headers.referer).to.equal(`http://127.0.0.1:${(server.address() as AddressInfo).port}/`);
            return done();
          } catch (e) {
            return done(e);
          } finally {
            server.close();
          }
        }
        res.end('<a id="a" href="/should_have_referrer" target="_blank">link</a>');
      });
      server.listen(0, '127.0.0.1', () => {
        const url = 'http://127.0.0.1:' + (server.address() as AddressInfo).port + '/';
        w.webContents.once('did-finish-load', () => {
          w.webContents.once('new-window', (event, newUrl, frameName, disposition, options, features, referrer) => {
            expect(referrer.url).to.equal(url);
            expect(referrer.policy).to.equal('strict-origin-when-cross-origin');
          });
          w.webContents.executeJavaScript('a.click()');
        });
        w.loadURL(url);
      });
    });

    // TODO(jeremy): window.open() in a real browser passes the referrer, but
    // our hacked-up window.open() shim doesn't. It should.
    xit('propagates referrer information to windows opened with window.open', (done) => {
      const w = new BrowserWindow({ show: false });
      const server = http.createServer((req, res) => {
        if (req.url === '/should_have_referrer') {
          try {
            expect(req.headers.referer).to.equal(`http://127.0.0.1:${(server.address() as AddressInfo).port}/`);
            return done();
          } catch (e) {
            return done(e);
          }
        }
        res.end('');
      });
      server.listen(0, '127.0.0.1', () => {
        const url = 'http://127.0.0.1:' + (server.address() as AddressInfo).port + '/';
        w.webContents.once('did-finish-load', () => {
          w.webContents.once('new-window', (event, newUrl, frameName, disposition, options, features, referrer) => {
            expect(referrer.url).to.equal(url);
            expect(referrer.policy).to.equal('no-referrer-when-downgrade');
          });
          w.webContents.executeJavaScript('window.open(location.href + "should_have_referrer")');
        });
        w.loadURL(url);
      });
    });
  });

  describe('webframe messages in sandboxed contents', () => {
    afterEach(closeAllWindows);
    it('responds to executeJavaScript', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { sandbox: true } });
      await w.loadURL('about:blank');
      const result = await w.webContents.executeJavaScript('37 + 5');
      expect(result).to.equal(42);
    });
  });

  describe('preload-error event', () => {
    afterEach(closeAllWindows);
    const generateSpecs = (description: string, sandbox: boolean) => {
      describe(description, () => {
        it('is triggered when unhandled exception is thrown', async () => {
          const preload = path.join(fixturesPath, 'module', 'preload-error-exception.js');

          const w = new BrowserWindow({
            show: false,
            webPreferences: {
              sandbox,
              preload
            }
          });

          const promise = emittedOnce(w.webContents, 'preload-error');
          w.loadURL('about:blank');

          const [, preloadPath, error] = await promise;
          expect(preloadPath).to.equal(preload);
          expect(error.message).to.equal('Hello World!');
        });

        it('is triggered on syntax errors', async () => {
          const preload = path.join(fixturesPath, 'module', 'preload-error-syntax.js');

          const w = new BrowserWindow({
            show: false,
            webPreferences: {
              sandbox,
              preload
            }
          });

          const promise = emittedOnce(w.webContents, 'preload-error');
          w.loadURL('about:blank');

          const [, preloadPath, error] = await promise;
          expect(preloadPath).to.equal(preload);
          expect(error.message).to.equal('foobar is not defined');
        });

        it('is triggered when preload script loading fails', async () => {
          const preload = path.join(fixturesPath, 'module', 'preload-invalid.js');

          const w = new BrowserWindow({
            show: false,
            webPreferences: {
              sandbox,
              preload
            }
          });

          const promise = emittedOnce(w.webContents, 'preload-error');
          w.loadURL('about:blank');

          const [, preloadPath, error] = await promise;
          expect(preloadPath).to.equal(preload);
          expect(error.message).to.contain('preload-invalid.js');
        });
      });
    };

    generateSpecs('without sandbox', false);
    generateSpecs('with sandbox', true);
  });

  describe('takeHeapSnapshot()', () => {
    afterEach(closeAllWindows);

    it('works with sandboxed renderers', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          sandbox: true
        }
      });

      await w.loadURL('about:blank');

      const filePath = path.join(app.getPath('temp'), 'test.heapsnapshot');

      const cleanup = () => {
        try {
          fs.unlinkSync(filePath);
        } catch (e) {
          // ignore error
        }
      };

      try {
        await w.webContents.takeHeapSnapshot(filePath);
        const stats = fs.statSync(filePath);
        expect(stats.size).not.to.be.equal(0);
      } finally {
        cleanup();
      }
    });

    it('fails with invalid file path', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          sandbox: true
        }
      });

      await w.loadURL('about:blank');

      const promise = w.webContents.takeHeapSnapshot('');
      return expect(promise).to.be.eventually.rejectedWith(Error, 'takeHeapSnapshot failed');
    });
  });

  describe('setBackgroundThrottling()', () => {
    afterEach(closeAllWindows);
    it('does not crash when allowing', () => {
      const w = new BrowserWindow({ show: false });
      w.webContents.setBackgroundThrottling(true);
    });

    it('does not crash when called via BrowserWindow', () => {
      const w = new BrowserWindow({ show: false });

      (w as any).setBackgroundThrottling(true);
    });

    it('does not crash when disallowing', () => {
      const w = new BrowserWindow({ show: false, webPreferences: { backgroundThrottling: true } });

      w.webContents.setBackgroundThrottling(false);
    });
  });

  describe('getBackgroundThrottling()', () => {
    afterEach(closeAllWindows);
    it('works via getter', () => {
      const w = new BrowserWindow({ show: false });

      w.webContents.setBackgroundThrottling(false);
      expect(w.webContents.getBackgroundThrottling()).to.equal(false);

      w.webContents.setBackgroundThrottling(true);
      expect(w.webContents.getBackgroundThrottling()).to.equal(true);
    });

    it('works via property', () => {
      const w = new BrowserWindow({ show: false });

      w.webContents.backgroundThrottling = false;
      expect(w.webContents.backgroundThrottling).to.equal(false);

      w.webContents.backgroundThrottling = true;
      expect(w.webContents.backgroundThrottling).to.equal(true);
    });

    it('works via BrowserWindow', () => {
      const w = new BrowserWindow({ show: false });

      (w as any).setBackgroundThrottling(false);
      expect((w as any).getBackgroundThrottling()).to.equal(false);

      (w as any).setBackgroundThrottling(true);
      expect((w as any).getBackgroundThrottling()).to.equal(true);
    });
  });

  ifdescribe(features.isPrintingEnabled())('getPrinters()', () => {
    afterEach(closeAllWindows);
    it('can get printer list', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { sandbox: true } });
      await w.loadURL('about:blank');
      const printers = w.webContents.getPrinters();
      expect(printers).to.be.an('array');
    });
  });

  ifdescribe(features.isPrintingEnabled())('getPrintersAsync()', () => {
    afterEach(closeAllWindows);
    it('can get printer list', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { sandbox: true } });
      await w.loadURL('about:blank');
      const printers = await w.webContents.getPrintersAsync();
      expect(printers).to.be.an('array');
    });
  });

  ifdescribe(features.isPrintingEnabled())('printToPDF()', () => {
    let w: BrowserWindow;

    beforeEach(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { sandbox: true } });
      await w.loadURL('data:text/html,<h1>Hello, World!</h1>');
    });

    afterEach(closeAllWindows);

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
        await expect(w.webContents.printToPDF(param)).to.eventually.be.rejected();
      }
    });

    it('can print to PDF', async () => {
      const data = await w.webContents.printToPDF({});
      expect(data).to.be.an.instanceof(Buffer).that.is.not.empty();
    });

    it('does not crash when called multiple times in parallel', async () => {
      const promises = [];
      for (let i = 0; i < 3; i++) {
        promises.push(w.webContents.printToPDF({}));
      }

      const results = await Promise.all(promises);
      for (const data of results) {
        expect(data).to.be.an.instanceof(Buffer).that.is.not.empty();
      }
    });

    it('does not crash when called multiple times in sequence', async () => {
      const results = [];
      for (let i = 0; i < 3; i++) {
        const result = await w.webContents.printToPDF({});
        results.push(result);
      }

      for (const data of results) {
        expect(data).to.be.an.instanceof(Buffer).that.is.not.empty();
      }
    });

    // TODO(codebytere): Re-enable after Chromium fixes upstream v8_scriptormodule_legacy_lifetime crash.
    xdescribe('using a large document', () => {
      beforeEach(async () => {
        w = new BrowserWindow({ show: false, webPreferences: { sandbox: true } });
        await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'print-to-pdf.html'));
      });

      afterEach(closeAllWindows);

      it('respects custom settings', async () => {
        const data = await w.webContents.printToPDF({
          pageRanges: {
            from: 0,
            to: 2
          },
          landscape: true
        });

        const doc = await pdfjs.getDocument(data).promise;

        // Check that correct # of pages are rendered.
        expect(doc.numPages).to.equal(3);

        // Check that PDF is generated in landscape mode.
        const firstPage = await doc.getPage(1);
        const { width, height } = firstPage.getViewport({ scale: 100 });
        expect(width).to.be.greaterThan(height);
      });
    });
  });

  describe('PictureInPicture video', () => {
    afterEach(closeAllWindows);
    it('works as expected', async function () {
      const w = new BrowserWindow({ show: false, webPreferences: { sandbox: true } });
      await w.loadFile(path.join(fixturesPath, 'api', 'picture-in-picture.html'));

      if (!await w.webContents.executeJavaScript('document.createElement(\'video\').canPlayType(\'video/webm; codecs="vp8.0"\')')) {
        this.skip();
      }

      const result = await w.webContents.executeJavaScript(
        `runTest(${features.isPictureInPictureEnabled()})`, true);
      expect(result).to.be.true();
    });
  });

  describe('devtools window', () => {
    let hasRobotJS = false;
    try {
      // We have other tests that check if native modules work, if we fail to require
      // robotjs let's skip this test to avoid false negatives
      require('robotjs');
      hasRobotJS = true;
    } catch (err) { /* no-op */ }

    afterEach(closeAllWindows);

    // NB. on macOS, this requires that you grant your terminal the ability to
    // control your computer. Open System Preferences > Security & Privacy >
    // Privacy > Accessibility and grant your terminal the permission to control
    // your computer.
    ifit(hasRobotJS)('can receive and handle menu events', async () => {
      const w = new BrowserWindow({ show: true, webPreferences: { nodeIntegration: true } });
      w.loadFile(path.join(fixturesPath, 'pages', 'key-events.html'));

      // Ensure the devtools are loaded
      w.webContents.closeDevTools();
      const opened = emittedOnce(w.webContents, 'devtools-opened');
      w.webContents.openDevTools();
      await opened;
      await emittedOnce(w.webContents.devToolsWebContents!, 'did-finish-load');
      w.webContents.devToolsWebContents!.focus();

      // Focus an input field
      await w.webContents.devToolsWebContents!.executeJavaScript(`
        const input = document.createElement('input')
        document.body.innerHTML = ''
        document.body.appendChild(input)
        input.focus()
      `);

      // Write something to the clipboard
      clipboard.writeText('test value');

      const pasted = w.webContents.devToolsWebContents!.executeJavaScript(`new Promise(resolve => {
        document.querySelector('input').addEventListener('paste', (e) => {
          resolve(e.target.value)
        })
      })`);

      // Fake a paste request using robotjs to emulate a REAL keyboard paste event
      require('robotjs').keyTap('v', process.platform === 'darwin' ? ['command'] : ['control']);

      const val = await pasted;

      // Once we're done expect the paste to have been successful
      expect(val).to.equal('test value', 'value should eventually become the pasted value');
    });
  });

  describe('Shared Workers', () => {
    afterEach(closeAllWindows);

    it('can get multiple shared workers', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });

      const ready = emittedOnce(ipcMain, 'ready');
      w.loadFile(path.join(fixturesPath, 'api', 'shared-worker', 'shared-worker.html'));
      await ready;

      const sharedWorkers = w.webContents.getAllSharedWorkers();

      expect(sharedWorkers).to.have.lengthOf(2);
      expect(sharedWorkers[0].url).to.contain('shared-worker');
      expect(sharedWorkers[1].url).to.contain('shared-worker');
    });

    it('can inspect a specific shared worker', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });

      const ready = emittedOnce(ipcMain, 'ready');
      w.loadFile(path.join(fixturesPath, 'api', 'shared-worker', 'shared-worker.html'));
      await ready;

      const sharedWorkers = w.webContents.getAllSharedWorkers();

      const devtoolsOpened = emittedOnce(w.webContents, 'devtools-opened');
      w.webContents.inspectSharedWorkerById(sharedWorkers[0].id);
      await devtoolsOpened;

      const devtoolsClosed = emittedOnce(w.webContents, 'devtools-closed');
      w.webContents.closeDevTools();
      await devtoolsClosed;
    });
  });

  describe('login event', () => {
    afterEach(closeAllWindows);

    let server: http.Server;
    let serverUrl: string;
    let serverPort: number;
    let proxyServer: http.Server;
    let proxyServerPort: number;

    before((done) => {
      server = http.createServer((request, response) => {
        if (request.url === '/no-auth') {
          return response.end('ok');
        }
        if (request.headers.authorization) {
          response.writeHead(200, { 'Content-type': 'text/plain' });
          return response.end(request.headers.authorization);
        }
        response
          .writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' })
          .end('401');
      }).listen(0, '127.0.0.1', () => {
        serverPort = (server.address() as AddressInfo).port;
        serverUrl = `http://127.0.0.1:${serverPort}`;
        done();
      });
    });

    before((done) => {
      proxyServer = http.createServer((request, response) => {
        if (request.headers['proxy-authorization']) {
          response.writeHead(200, { 'Content-type': 'text/plain' });
          return response.end(request.headers['proxy-authorization']);
        }
        response
          .writeHead(407, { 'Proxy-Authenticate': 'Basic realm="Foo"' })
          .end();
      }).listen(0, '127.0.0.1', () => {
        proxyServerPort = (proxyServer.address() as AddressInfo).port;
        done();
      });
    });

    afterEach(async () => {
      await session.defaultSession.clearAuthCache();
    });

    after(() => {
      server.close();
      proxyServer.close();
    });

    it('is emitted when navigating', async () => {
      const [user, pass] = ['user', 'pass'];
      const w = new BrowserWindow({ show: false });
      let eventRequest: any;
      let eventAuthInfo: any;
      w.webContents.on('login', (event, request, authInfo, cb) => {
        eventRequest = request;
        eventAuthInfo = authInfo;
        event.preventDefault();
        cb(user, pass);
      });
      await w.loadURL(serverUrl);
      const body = await w.webContents.executeJavaScript('document.documentElement.textContent');
      expect(body).to.equal(`Basic ${Buffer.from(`${user}:${pass}`).toString('base64')}`);
      expect(eventRequest.url).to.equal(serverUrl + '/');
      expect(eventAuthInfo.isProxy).to.be.false();
      expect(eventAuthInfo.scheme).to.equal('basic');
      expect(eventAuthInfo.host).to.equal('127.0.0.1');
      expect(eventAuthInfo.port).to.equal(serverPort);
      expect(eventAuthInfo.realm).to.equal('Foo');
    });

    it('is emitted when a proxy requests authorization', async () => {
      const customSession = session.fromPartition(`${Math.random()}`);
      await customSession.setProxy({ proxyRules: `127.0.0.1:${proxyServerPort}`, proxyBypassRules: '<-loopback>' });
      const [user, pass] = ['user', 'pass'];
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
      let eventRequest: any;
      let eventAuthInfo: any;
      w.webContents.on('login', (event, request, authInfo, cb) => {
        eventRequest = request;
        eventAuthInfo = authInfo;
        event.preventDefault();
        cb(user, pass);
      });
      await w.loadURL(`${serverUrl}/no-auth`);
      const body = await w.webContents.executeJavaScript('document.documentElement.textContent');
      expect(body).to.equal(`Basic ${Buffer.from(`${user}:${pass}`).toString('base64')}`);
      expect(eventRequest.url).to.equal(`${serverUrl}/no-auth`);
      expect(eventAuthInfo.isProxy).to.be.true();
      expect(eventAuthInfo.scheme).to.equal('basic');
      expect(eventAuthInfo.host).to.equal('127.0.0.1');
      expect(eventAuthInfo.port).to.equal(proxyServerPort);
      expect(eventAuthInfo.realm).to.equal('Foo');
    });

    it('cancels authentication when callback is called with no arguments', async () => {
      const w = new BrowserWindow({ show: false });
      w.webContents.on('login', (event, request, authInfo, cb) => {
        event.preventDefault();
        cb();
      });
      await w.loadURL(serverUrl);
      const body = await w.webContents.executeJavaScript('document.documentElement.textContent');
      expect(body).to.equal('401');
    });
  });

  describe('page-title-updated event', () => {
    afterEach(closeAllWindows);
    it('is emitted with a full title for pages with no navigation', async () => {
      const bw = new BrowserWindow({ show: false });
      await bw.loadURL('about:blank');
      bw.webContents.executeJavaScript('child = window.open("", "", "show=no"); null');
      const [, child] = await emittedOnce(app, 'web-contents-created');
      bw.webContents.executeJavaScript('child.document.title = "new title"');
      const [, title] = await emittedOnce(child, 'page-title-updated');
      expect(title).to.equal('new title');
    });
  });

  describe('crashed event', () => {
    it('does not crash main process when destroying WebContents in it', (done) => {
      const contents = (webContents as any).create({ nodeIntegration: true });
      contents.once('crashed', () => {
        contents.destroy();
        done();
      });
      contents.loadURL('about:blank').then(() => contents.forcefullyCrashRenderer());
    });
  });

  describe('context-menu event', () => {
    afterEach(closeAllWindows);
    it('emits when right-clicked in page', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));

      const promise = emittedOnce(w.webContents, 'context-menu');

      // Simulate right-click to create context-menu event.
      const opts = { x: 0, y: 0, button: 'right' as any };
      w.webContents.sendInputEvent({ ...opts, type: 'mouseDown' });
      w.webContents.sendInputEvent({ ...opts, type: 'mouseUp' });

      const [, params] = await promise;

      expect(params.pageURL).to.equal(w.webContents.getURL());
      expect(params.frame).to.be.an('object');
      expect(params.x).to.be.a('number');
      expect(params.y).to.be.a('number');
    });
  });

  it('emits a cancelable event before creating a child webcontents', async () => {
    const w = new BrowserWindow({
      show: false,
      webPreferences: {
        sandbox: true
      }
    });
    w.webContents.on('-will-add-new-contents' as any, (event: any, url: any) => {
      expect(url).to.equal('about:blank');
      event.preventDefault();
    });
    let wasCalled = false;
    w.webContents.on('new-window' as any, () => {
      wasCalled = true;
    });
    await w.loadURL('about:blank');
    await w.webContents.executeJavaScript('window.open(\'about:blank\')');
    await new Promise((resolve) => { process.nextTick(resolve); });
    expect(wasCalled).to.equal(false);
    await closeAllWindows();
  });
});

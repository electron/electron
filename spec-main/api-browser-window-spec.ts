import { expect } from 'chai';
import * as childProcess from 'child_process';
import * as path from 'path';
import * as fs from 'fs';
import * as os from 'os';
import * as qs from 'querystring';
import * as http from 'http';
import * as semver from 'semver';
import { AddressInfo } from 'net';
import { app, BrowserWindow, BrowserView, dialog, ipcMain, OnBeforeSendHeadersListenerDetails, protocol, screen, webContents, session, WebContents, BrowserWindowConstructorOptions } from 'electron/main';

import { emittedOnce, emittedUntil, emittedNTimes } from './events-helpers';
import { ifit, ifdescribe, defer, delay } from './spec-helpers';
import { closeWindow, closeAllWindows } from './window-helpers';

const features = process._linkedBinding('electron_common_features');
const fixtures = path.resolve(__dirname, '..', 'spec', 'fixtures');

// Is the display's scale factor possibly causing rounding of pixel coordinate
// values?
const isScaleFactorRounding = () => {
  const { scaleFactor } = screen.getPrimaryDisplay();
  // Return true if scale factor is non-integer value
  if (Math.round(scaleFactor) !== scaleFactor) return true;
  // Return true if scale factor is odd number above 2
  return scaleFactor > 2 && scaleFactor % 2 === 1;
};

const expectBoundsEqual = (actual: any, expected: any) => {
  if (!isScaleFactorRounding()) {
    expect(expected).to.deep.equal(actual);
  } else if (Array.isArray(actual)) {
    expect(actual[0]).to.be.closeTo(expected[0], 1);
    expect(actual[1]).to.be.closeTo(expected[1], 1);
  } else {
    expect(actual.x).to.be.closeTo(expected.x, 1);
    expect(actual.y).to.be.closeTo(expected.y, 1);
    expect(actual.width).to.be.closeTo(expected.width, 1);
    expect(actual.height).to.be.closeTo(expected.height, 1);
  }
};

const isBeforeUnload = (event: Event, level: number, message: string) => {
  return (message === 'beforeunload');
};

describe('BrowserWindow module', () => {
  describe('BrowserWindow constructor', () => {
    it('allows passing void 0 as the webContents', async () => {
      expect(() => {
        const w = new BrowserWindow({
          show: false,
          // apparently void 0 had different behaviour from undefined in the
          // issue that this test is supposed to catch.
          webContents: void 0 // eslint-disable-line no-void
        } as any);
        w.destroy();
      }).not.to.throw();
    });

    ifit(process.platform === 'linux')('does not crash when setting large window icons', async () => {
      const appPath = path.join(__dirname, 'spec-main', 'fixtures', 'apps', 'xwindow-icon');
      const appProcess = childProcess.spawn(process.execPath, [appPath]);
      await new Promise((resolve) => { appProcess.once('exit', resolve); });
    });

    it('does not crash or throw when passed an invalid icon', async () => {
      expect(() => {
        const w = new BrowserWindow({
          icon: undefined
        } as any);
        w.destroy();
      }).not.to.throw();
    });
  });

  describe('garbage collection', () => {
    const v8Util = process._linkedBinding('electron_common_v8_util');
    afterEach(closeAllWindows);

    it('window does not get garbage collected when opened', async () => {
      const w = new BrowserWindow({ show: false });
      // Keep a weak reference to the window.
      // eslint-disable-next-line no-undef
      const wr = new WeakRef(w);
      await delay();
      // Do garbage collection, since |w| is not referenced in this closure
      // it would be gone after next call if there is no other reference.
      v8Util.requestGarbageCollectionForTesting();

      await delay();
      expect(wr.deref()).to.not.be.undefined();
    });
  });

  describe('BrowserWindow.close()', () => {
    let w = null as unknown as BrowserWindow;
    beforeEach(() => {
      w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
    });
    afterEach(async () => {
      await closeWindow(w);
      w = null as unknown as BrowserWindow;
    });

    it('should work if called when a messageBox is showing', async () => {
      const closed = emittedOnce(w, 'closed');
      dialog.showMessageBox(w, { message: 'Hello Error' });
      w.close();
      await closed;
    });

    it('closes window without rounded corners', async () => {
      await closeWindow(w);
      w = new BrowserWindow({ show: false, frame: false, roundedCorners: false });
      const closed = emittedOnce(w, 'closed');
      w.close();
      await closed;
    });

    it('should not crash if called after webContents is destroyed', () => {
      w.webContents.destroy();
      w.webContents.on('destroyed', () => w.close());
    });

    it('should emit unload handler', async () => {
      await w.loadFile(path.join(fixtures, 'api', 'unload.html'));
      const closed = emittedOnce(w, 'closed');
      w.close();
      await closed;
      const test = path.join(fixtures, 'api', 'unload');
      const content = fs.readFileSync(test);
      fs.unlinkSync(test);
      expect(String(content)).to.equal('unload');
    });

    it('should emit beforeunload handler', async () => {
      await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'beforeunload-false.html'));
      w.close();
      await emittedOnce(w.webContents, 'before-unload-fired');
    });

    it('should not crash when keyboard event is sent before closing', async () => {
      await w.loadURL('data:text/html,pls no crash');
      const closed = emittedOnce(w, 'closed');
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Escape' });
      w.close();
      await closed;
    });

    describe('when invoked synchronously inside navigation observer', () => {
      let server: http.Server = null as unknown as http.Server;
      let url: string = null as unknown as string;

      before((done) => {
        server = http.createServer((request, response) => {
          switch (request.url) {
            case '/net-error':
              response.destroy();
              break;
            case '/301':
              response.statusCode = 301;
              response.setHeader('Location', '/200');
              response.end();
              break;
            case '/200':
              response.statusCode = 200;
              response.end('hello');
              break;
            case '/title':
              response.statusCode = 200;
              response.end('<title>Hello</title>');
              break;
            default:
              throw new Error(`unsupported endpoint: ${request.url}`);
          }
        }).listen(0, '127.0.0.1', () => {
          url = 'http://127.0.0.1:' + (server.address() as AddressInfo).port;
          done();
        });
      });

      after(() => {
        server.close();
      });

      const events = [
        { name: 'did-start-loading', path: '/200' },
        { name: 'dom-ready', path: '/200' },
        { name: 'page-title-updated', path: '/title' },
        { name: 'did-stop-loading', path: '/200' },
        { name: 'did-finish-load', path: '/200' },
        { name: 'did-frame-finish-load', path: '/200' },
        { name: 'did-fail-load', path: '/net-error' }
      ];

      for (const { name, path } of events) {
        it(`should not crash when closed during ${name}`, async () => {
          const w = new BrowserWindow({ show: false });
          w.webContents.once((name as any), () => {
            w.close();
          });
          const destroyed = emittedOnce(w.webContents, 'destroyed');
          w.webContents.loadURL(url + path);
          await destroyed;
        });
      }
    });
  });

  describe('window.close()', () => {
    let w = null as unknown as BrowserWindow;
    beforeEach(() => {
      w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
    });
    afterEach(async () => {
      await closeWindow(w);
      w = null as unknown as BrowserWindow;
    });

    it('should emit unload event', async () => {
      w.loadFile(path.join(fixtures, 'api', 'close.html'));
      await emittedOnce(w, 'closed');
      const test = path.join(fixtures, 'api', 'close');
      const content = fs.readFileSync(test).toString();
      fs.unlinkSync(test);
      expect(content).to.equal('close');
    });

    it('should emit beforeunload event', async function () {
      await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'beforeunload-false.html'));
      w.webContents.executeJavaScript('window.close()', true);
      await emittedOnce(w.webContents, 'before-unload-fired');
    });
  });

  describe('BrowserWindow.destroy()', () => {
    let w = null as unknown as BrowserWindow;
    beforeEach(() => {
      w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } });
    });
    afterEach(async () => {
      await closeWindow(w);
      w = null as unknown as BrowserWindow;
    });

    it('prevents users to access methods of webContents', async () => {
      const contents = w.webContents;
      w.destroy();
      await new Promise(setImmediate);
      expect(() => {
        contents.getProcessId();
      }).to.throw('Object has been destroyed');
    });
    it('should not crash when destroying windows with pending events', () => {
      const focusListener = () => { };
      app.on('browser-window-focus', focusListener);
      const windowCount = 3;
      const windowOptions = {
        show: false,
        width: 400,
        height: 400,
        webPreferences: {
          backgroundThrottling: false
        }
      };
      const windows = Array.from(Array(windowCount)).map(() => new BrowserWindow(windowOptions));
      windows.forEach(win => win.show());
      windows.forEach(win => win.focus());
      windows.forEach(win => win.destroy());
      app.removeListener('browser-window-focus', focusListener);
    });
  });

  describe('BrowserWindow.loadURL(url)', () => {
    let w = null as unknown as BrowserWindow;
    const scheme = 'other';
    const srcPath = path.join(fixtures, 'api', 'loaded-from-dataurl.js');
    before(() => {
      protocol.registerFileProtocol(scheme, (request, callback) => {
        callback(srcPath);
      });
    });

    after(() => {
      protocol.unregisterProtocol(scheme);
    });

    beforeEach(() => {
      w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } });
    });
    afterEach(async () => {
      await closeWindow(w);
      w = null as unknown as BrowserWindow;
    });
    let server = null as unknown as http.Server;
    let url = null as unknown as string;
    let postData = null as any;
    before((done) => {
      const filePath = path.join(fixtures, 'pages', 'a.html');
      const fileStats = fs.statSync(filePath);
      postData = [
        {
          type: 'rawData',
          bytes: Buffer.from('username=test&file=')
        },
        {
          type: 'file',
          filePath: filePath,
          offset: 0,
          length: fileStats.size,
          modificationTime: fileStats.mtime.getTime() / 1000
        }
      ];
      server = http.createServer((req, res) => {
        function respond () {
          if (req.method === 'POST') {
            let body = '';
            req.on('data', (data) => {
              if (data) body += data;
            });
            req.on('end', () => {
              const parsedData = qs.parse(body);
              fs.readFile(filePath, (err, data) => {
                if (err) return;
                if (parsedData.username === 'test' &&
                  parsedData.file === data.toString()) {
                  res.end();
                }
              });
            });
          } else if (req.url === '/302') {
            res.setHeader('Location', '/200');
            res.statusCode = 302;
            res.end();
          } else {
            res.end();
          }
        }
        setTimeout(respond, req.url && req.url.includes('slow') ? 200 : 0);
      });
      server.listen(0, '127.0.0.1', () => {
        url = `http://127.0.0.1:${(server.address() as AddressInfo).port}`;
        done();
      });
    });

    after(() => {
      server.close();
    });

    it('should emit did-start-loading event', async () => {
      const didStartLoading = emittedOnce(w.webContents, 'did-start-loading');
      w.loadURL('about:blank');
      await didStartLoading;
    });
    it('should emit ready-to-show event', async () => {
      const readyToShow = emittedOnce(w, 'ready-to-show');
      w.loadURL('about:blank');
      await readyToShow;
    });
    // TODO(deepak1556): The error code now seems to be `ERR_FAILED`, verify what
    // changed and adjust the test.
    it.skip('should emit did-fail-load event for files that do not exist', async () => {
      const didFailLoad = emittedOnce(w.webContents, 'did-fail-load');
      w.loadURL('file://a.txt');
      const [, code, desc,, isMainFrame] = await didFailLoad;
      expect(code).to.equal(-6);
      expect(desc).to.equal('ERR_FILE_NOT_FOUND');
      expect(isMainFrame).to.equal(true);
    });
    it('should emit did-fail-load event for invalid URL', async () => {
      const didFailLoad = emittedOnce(w.webContents, 'did-fail-load');
      w.loadURL('http://example:port');
      const [, code, desc,, isMainFrame] = await didFailLoad;
      expect(desc).to.equal('ERR_INVALID_URL');
      expect(code).to.equal(-300);
      expect(isMainFrame).to.equal(true);
    });
    it('should set `mainFrame = false` on did-fail-load events in iframes', async () => {
      const didFailLoad = emittedOnce(w.webContents, 'did-fail-load');
      w.loadFile(path.join(fixtures, 'api', 'did-fail-load-iframe.html'));
      const [,,,, isMainFrame] = await didFailLoad;
      expect(isMainFrame).to.equal(false);
    });
    it('does not crash in did-fail-provisional-load handler', (done) => {
      w.webContents.once('did-fail-provisional-load', () => {
        w.loadURL('http://127.0.0.1:11111');
        done();
      });
      w.loadURL('http://127.0.0.1:11111');
    });
    it('should emit did-fail-load event for URL exceeding character limit', async () => {
      const data = Buffer.alloc(2 * 1024 * 1024).toString('base64');
      const didFailLoad = emittedOnce(w.webContents, 'did-fail-load');
      w.loadURL(`data:image/png;base64,${data}`);
      const [, code, desc,, isMainFrame] = await didFailLoad;
      expect(desc).to.equal('ERR_INVALID_URL');
      expect(code).to.equal(-300);
      expect(isMainFrame).to.equal(true);
    });

    it('should return a promise', () => {
      const p = w.loadURL('about:blank');
      expect(p).to.have.property('then');
    });

    it('should return a promise that resolves', async () => {
      await expect(w.loadURL('about:blank')).to.eventually.be.fulfilled();
    });

    it('should return a promise that rejects on a load failure', async () => {
      const data = Buffer.alloc(2 * 1024 * 1024).toString('base64');
      const p = w.loadURL(`data:image/png;base64,${data}`);
      await expect(p).to.eventually.be.rejected;
    });

    it('should return a promise that resolves even if pushState occurs during navigation', async () => {
      const p = w.loadURL('data:text/html,<script>window.history.pushState({}, "/foo")</script>');
      await expect(p).to.eventually.be.fulfilled;
    });

    describe('POST navigations', () => {
      afterEach(() => { w.webContents.session.webRequest.onBeforeSendHeaders(null); });

      it('supports specifying POST data', async () => {
        await w.loadURL(url, { postData });
      });
      it('sets the content type header on URL encoded forms', async () => {
        await w.loadURL(url);
        const requestDetails: Promise<OnBeforeSendHeadersListenerDetails> = new Promise(resolve => {
          w.webContents.session.webRequest.onBeforeSendHeaders((details) => {
            resolve(details);
          });
        });
        w.webContents.executeJavaScript(`
          form = document.createElement('form')
          document.body.appendChild(form)
          form.method = 'POST'
          form.submit()
        `);
        const details = await requestDetails;
        expect(details.requestHeaders['Content-Type']).to.equal('application/x-www-form-urlencoded');
      });
      it('sets the content type header on multi part forms', async () => {
        await w.loadURL(url);
        const requestDetails: Promise<OnBeforeSendHeadersListenerDetails> = new Promise(resolve => {
          w.webContents.session.webRequest.onBeforeSendHeaders((details) => {
            resolve(details);
          });
        });
        w.webContents.executeJavaScript(`
          form = document.createElement('form')
          document.body.appendChild(form)
          form.method = 'POST'
          form.enctype = 'multipart/form-data'
          file = document.createElement('input')
          file.type = 'file'
          file.name = 'file'
          form.appendChild(file)
          form.submit()
        `);
        const details = await requestDetails;
        expect(details.requestHeaders['Content-Type'].startsWith('multipart/form-data; boundary=----WebKitFormBoundary')).to.equal(true);
      });
    });

    it('should support base url for data urls', async () => {
      await w.loadURL('data:text/html,<script src="loaded-from-dataurl.js"></script>', { baseURLForDataURL: `other://${path.join(fixtures, 'api')}${path.sep}` });
      expect(await w.webContents.executeJavaScript('window.ping')).to.equal('pong');
    });
  });

  for (const sandbox of [false, true]) {
    describe(`navigation events${sandbox ? ' with sandbox' : ''}`, () => {
      let w = null as unknown as BrowserWindow;
      beforeEach(() => {
        w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: false, sandbox } });
      });
      afterEach(async () => {
        await closeWindow(w);
        w = null as unknown as BrowserWindow;
      });

      describe('will-navigate event', () => {
        let server = null as unknown as http.Server;
        let url = null as unknown as string;
        before((done) => {
          server = http.createServer((req, res) => {
            if (req.url === '/navigate-top') {
              res.end('<a target=_top href="/">navigate _top</a>');
            } else {
              res.end('');
            }
          });
          server.listen(0, '127.0.0.1', () => {
            url = `http://127.0.0.1:${(server.address() as AddressInfo).port}/`;
            done();
          });
        });

        after(() => {
          server.close();
        });

        it('allows the window to be closed from the event listener', (done) => {
          w.webContents.once('will-navigate', () => {
            w.close();
            done();
          });
          w.loadFile(path.join(fixtures, 'pages', 'will-navigate.html'));
        });

        it('can be prevented', (done) => {
          let willNavigate = false;
          w.webContents.once('will-navigate', (e) => {
            willNavigate = true;
            e.preventDefault();
          });
          w.webContents.on('did-stop-loading', () => {
            if (willNavigate) {
              // i.e. it shouldn't have had '?navigated' appended to it.
              try {
                expect(w.webContents.getURL().endsWith('will-navigate.html')).to.be.true();
                done();
              } catch (e) {
                done(e);
              }
            }
          });
          w.loadFile(path.join(fixtures, 'pages', 'will-navigate.html'));
        });

        it('is triggered when navigating from file: to http:', async () => {
          await w.loadFile(path.join(fixtures, 'api', 'blank.html'));
          w.webContents.executeJavaScript(`location.href = ${JSON.stringify(url)}`);
          const navigatedTo = await new Promise(resolve => {
            w.webContents.once('will-navigate', (e, url) => {
              e.preventDefault();
              resolve(url);
            });
          });
          expect(navigatedTo).to.equal(url);
          expect(w.webContents.getURL()).to.match(/^file:/);
        });

        it('is triggered when navigating from about:blank to http:', async () => {
          await w.loadURL('about:blank');
          w.webContents.executeJavaScript(`location.href = ${JSON.stringify(url)}`);
          const navigatedTo = await new Promise(resolve => {
            w.webContents.once('will-navigate', (e, url) => {
              e.preventDefault();
              resolve(url);
            });
          });
          expect(navigatedTo).to.equal(url);
          expect(w.webContents.getURL()).to.equal('about:blank');
        });

        it('is triggered when a cross-origin iframe navigates _top', async () => {
          await w.loadURL(`data:text/html,<iframe src="http://127.0.0.1:${(server.address() as AddressInfo).port}/navigate-top"></iframe>`);
          await delay(1000);
          w.webContents.debugger.attach('1.1');
          const targets = await w.webContents.debugger.sendCommand('Target.getTargets');
          const iframeTarget = targets.targetInfos.find((t: any) => t.type === 'iframe');
          const { sessionId } = await w.webContents.debugger.sendCommand('Target.attachToTarget', {
            targetId: iframeTarget.targetId,
            flatten: true
          });
          await w.webContents.debugger.sendCommand('Input.dispatchMouseEvent', {
            type: 'mousePressed',
            x: 10,
            y: 10,
            clickCount: 1,
            button: 'left'
          }, sessionId);
          await w.webContents.debugger.sendCommand('Input.dispatchMouseEvent', {
            type: 'mouseReleased',
            x: 10,
            y: 10,
            clickCount: 1,
            button: 'left'
          }, sessionId);
          let willNavigateEmitted = false;
          w.webContents.on('will-navigate', () => {
            willNavigateEmitted = true;
          });
          await emittedOnce(w.webContents, 'did-navigate');
          expect(willNavigateEmitted).to.be.true();
        });
      });

      describe('will-redirect event', () => {
        let server = null as unknown as http.Server;
        let url = null as unknown as string;
        before((done) => {
          server = http.createServer((req, res) => {
            if (req.url === '/302') {
              res.setHeader('Location', '/200');
              res.statusCode = 302;
              res.end();
            } else if (req.url === '/navigate-302') {
              res.end(`<html><body><script>window.location='${url}/302'</script></body></html>`);
            } else {
              res.end();
            }
          });
          server.listen(0, '127.0.0.1', () => {
            url = `http://127.0.0.1:${(server.address() as AddressInfo).port}`;
            done();
          });
        });

        after(() => {
          server.close();
        });
        it('is emitted on redirects', async () => {
          const willRedirect = emittedOnce(w.webContents, 'will-redirect');
          w.loadURL(`${url}/302`);
          await willRedirect;
        });

        it('is emitted after will-navigate on redirects', async () => {
          let navigateCalled = false;
          w.webContents.on('will-navigate', () => {
            navigateCalled = true;
          });
          const willRedirect = emittedOnce(w.webContents, 'will-redirect');
          w.loadURL(`${url}/navigate-302`);
          await willRedirect;
          expect(navigateCalled).to.equal(true, 'should have called will-navigate first');
        });

        it('is emitted before did-stop-loading on redirects', async () => {
          let stopCalled = false;
          w.webContents.on('did-stop-loading', () => {
            stopCalled = true;
          });
          const willRedirect = emittedOnce(w.webContents, 'will-redirect');
          w.loadURL(`${url}/302`);
          await willRedirect;
          expect(stopCalled).to.equal(false, 'should not have called did-stop-loading first');
        });

        it('allows the window to be closed from the event listener', (done) => {
          w.webContents.once('will-redirect', () => {
            w.close();
            done();
          });
          w.loadURL(`${url}/302`);
        });

        it('can be prevented', (done) => {
          w.webContents.once('will-redirect', (event) => {
            event.preventDefault();
          });
          w.webContents.on('will-navigate', (e, u) => {
            expect(u).to.equal(`${url}/302`);
          });
          w.webContents.on('did-stop-loading', () => {
            try {
              expect(w.webContents.getURL()).to.equal(
                `${url}/navigate-302`,
                'url should not have changed after navigation event'
              );
              done();
            } catch (e) {
              done(e);
            }
          });
          w.webContents.on('will-redirect', (e, u) => {
            try {
              expect(u).to.equal(`${url}/200`);
            } catch (e) {
              done(e);
            }
          });
          w.loadURL(`${url}/navigate-302`);
        });
      });
    });
  }

  describe('focus and visibility', () => {
    let w = null as unknown as BrowserWindow;
    beforeEach(() => {
      w = new BrowserWindow({ show: false });
    });
    afterEach(async () => {
      await closeWindow(w);
      w = null as unknown as BrowserWindow;
    });

    describe('BrowserWindow.show()', () => {
      it('should focus on window', () => {
        w.show();
        expect(w.isFocused()).to.equal(true);
      });
      it('should make the window visible', () => {
        w.show();
        expect(w.isVisible()).to.equal(true);
      });
      it('emits when window is shown', async () => {
        const show = emittedOnce(w, 'show');
        w.show();
        await show;
        expect(w.isVisible()).to.equal(true);
      });
    });

    describe('BrowserWindow.hide()', () => {
      it('should defocus on window', () => {
        w.hide();
        expect(w.isFocused()).to.equal(false);
      });
      it('should make the window not visible', () => {
        w.show();
        w.hide();
        expect(w.isVisible()).to.equal(false);
      });
      it('emits when window is hidden', async () => {
        const shown = emittedOnce(w, 'show');
        w.show();
        await shown;
        const hidden = emittedOnce(w, 'hide');
        w.hide();
        await hidden;
        expect(w.isVisible()).to.equal(false);
      });
    });

    describe('BrowserWindow.showInactive()', () => {
      it('should not focus on window', () => {
        w.showInactive();
        expect(w.isFocused()).to.equal(false);
      });
    });

    describe('BrowserWindow.focus()', () => {
      it('does not make the window become visible', () => {
        expect(w.isVisible()).to.equal(false);
        w.focus();
        expect(w.isVisible()).to.equal(false);
      });
    });

    describe('BrowserWindow.blur()', () => {
      it('removes focus from window', () => {
        w.blur();
        expect(w.isFocused()).to.equal(false);
      });
    });

    describe('BrowserWindow.getFocusedWindow()', () => {
      it('returns the opener window when dev tools window is focused', async () => {
        w.show();
        w.webContents.openDevTools({ mode: 'undocked' });
        await emittedOnce(w.webContents, 'devtools-focused');
        expect(BrowserWindow.getFocusedWindow()).to.equal(w);
      });
    });

    describe('BrowserWindow.moveTop()', () => {
      it('should not steal focus', async () => {
        const posDelta = 50;
        const wShownInactive = emittedOnce(w, 'show');
        w.showInactive();
        await wShownInactive;
        expect(w.isFocused()).to.equal(false);

        const otherWindow = new BrowserWindow({ show: false, title: 'otherWindow' });
        const otherWindowShown = emittedOnce(otherWindow, 'show');
        const otherWindowFocused = emittedOnce(otherWindow, 'focus');
        otherWindow.show();
        await otherWindowShown;
        await otherWindowFocused;
        expect(otherWindow.isFocused()).to.equal(true);

        w.moveTop();
        const wPos = w.getPosition();
        const wMoving = emittedOnce(w, 'move');
        w.setPosition(wPos[0] + posDelta, wPos[1] + posDelta);
        await wMoving;
        expect(w.isFocused()).to.equal(false);
        expect(otherWindow.isFocused()).to.equal(true);

        const wFocused = emittedOnce(w, 'focus');
        const otherWindowBlurred = emittedOnce(otherWindow, 'blur');
        w.focus();
        await wFocused;
        await otherWindowBlurred;
        expect(w.isFocused()).to.equal(true);

        otherWindow.moveTop();
        const otherWindowPos = otherWindow.getPosition();
        const otherWindowMoving = emittedOnce(otherWindow, 'move');
        otherWindow.setPosition(otherWindowPos[0] + posDelta, otherWindowPos[1] + posDelta);
        await otherWindowMoving;
        expect(otherWindow.isFocused()).to.equal(false);
        expect(w.isFocused()).to.equal(true);

        await closeWindow(otherWindow, { assertNotWindows: false });
        expect(BrowserWindow.getAllWindows()).to.have.lengthOf(1);
      });
    });

    ifdescribe(features.isDesktopCapturerEnabled())('BrowserWindow.moveAbove(mediaSourceId)', () => {
      it('should throw an exception if wrong formatting', async () => {
        const fakeSourceIds = [
          'none', 'screen:0', 'window:fake', 'window:1234', 'foobar:1:2'
        ];
        fakeSourceIds.forEach((sourceId) => {
          expect(() => {
            w.moveAbove(sourceId);
          }).to.throw(/Invalid media source id/);
        });
      });

      it('should throw an exception if wrong type', async () => {
        const fakeSourceIds = [null as any, 123 as any];
        fakeSourceIds.forEach((sourceId) => {
          expect(() => {
            w.moveAbove(sourceId);
          }).to.throw(/Error processing argument at index 0 */);
        });
      });

      it('should throw an exception if invalid window', async () => {
        // It is very unlikely that these window id exist.
        const fakeSourceIds = ['window:99999999:0', 'window:123456:1',
          'window:123456:9'];
        fakeSourceIds.forEach((sourceId) => {
          expect(() => {
            w.moveAbove(sourceId);
          }).to.throw(/Invalid media source id/);
        });
      });

      it('should not throw an exception', async () => {
        const w2 = new BrowserWindow({ show: false, title: 'window2' });
        const w2Shown = emittedOnce(w2, 'show');
        w2.show();
        await w2Shown;

        expect(() => {
          w.moveAbove(w2.getMediaSourceId());
        }).to.not.throw();

        await closeWindow(w2, { assertNotWindows: false });
      });
    });

    describe('BrowserWindow.setFocusable()', () => {
      it('can set unfocusable window to focusable', async () => {
        const w2 = new BrowserWindow({ focusable: false });
        const w2Focused = emittedOnce(w2, 'focus');
        w2.setFocusable(true);
        w2.focus();
        await w2Focused;
        await closeWindow(w2, { assertNotWindows: false });
      });
    });

    describe('BrowserWindow.isFocusable()', () => {
      it('correctly returns whether a window is focusable', async () => {
        const w2 = new BrowserWindow({ focusable: false });
        expect(w2.isFocusable()).to.be.false();

        w2.setFocusable(true);
        expect(w2.isFocusable()).to.be.true();
        await closeWindow(w2, { assertNotWindows: false });
      });
    });
  });

  describe('sizing', () => {
    let w = null as unknown as BrowserWindow;

    beforeEach(() => {
      w = new BrowserWindow({ show: false, width: 400, height: 400 });
    });

    afterEach(async () => {
      await closeWindow(w);
      w = null as unknown as BrowserWindow;
    });

    describe('BrowserWindow.setBounds(bounds[, animate])', () => {
      it('sets the window bounds with full bounds', () => {
        const fullBounds = { x: 440, y: 225, width: 500, height: 400 };
        w.setBounds(fullBounds);

        expectBoundsEqual(w.getBounds(), fullBounds);
      });

      it('sets the window bounds with partial bounds', () => {
        const fullBounds = { x: 440, y: 225, width: 500, height: 400 };
        w.setBounds(fullBounds);

        const boundsUpdate = { width: 200 };
        w.setBounds(boundsUpdate as any);

        const expectedBounds = Object.assign(fullBounds, boundsUpdate);
        expectBoundsEqual(w.getBounds(), expectedBounds);
      });

      ifit(process.platform === 'darwin')('on macOS', () => {
        it('emits \'resized\' event after animating', async () => {
          const fullBounds = { x: 440, y: 225, width: 500, height: 400 };
          w.setBounds(fullBounds, true);

          await expect(emittedOnce(w, 'resized')).to.eventually.be.fulfilled();
        });
      });
    });

    describe('BrowserWindow.setSize(width, height)', () => {
      it('sets the window size', async () => {
        const size = [300, 400];

        const resized = emittedOnce(w, 'resize');
        w.setSize(size[0], size[1]);
        await resized;

        expectBoundsEqual(w.getSize(), size);
      });

      ifit(process.platform === 'darwin')('on macOS', () => {
        it('emits \'resized\' event after animating', async () => {
          const size = [300, 400];
          w.setSize(size[0], size[1], true);

          await expect(emittedOnce(w, 'resized')).to.eventually.be.fulfilled();
        });
      });
    });

    describe('BrowserWindow.setMinimum/MaximumSize(width, height)', () => {
      it('sets the maximum and minimum size of the window', () => {
        expect(w.getMinimumSize()).to.deep.equal([0, 0]);
        expect(w.getMaximumSize()).to.deep.equal([0, 0]);

        w.setMinimumSize(100, 100);
        expectBoundsEqual(w.getMinimumSize(), [100, 100]);
        expectBoundsEqual(w.getMaximumSize(), [0, 0]);

        w.setMaximumSize(900, 600);
        expectBoundsEqual(w.getMinimumSize(), [100, 100]);
        expectBoundsEqual(w.getMaximumSize(), [900, 600]);
      });
    });

    describe('BrowserWindow.setAspectRatio(ratio)', () => {
      it('resets the behaviour when passing in 0', async () => {
        const size = [300, 400];
        w.setAspectRatio(1 / 2);
        w.setAspectRatio(0);
        const resize = emittedOnce(w, 'resize');
        w.setSize(size[0], size[1]);
        await resize;
        expectBoundsEqual(w.getSize(), size);
      });

      it('doesn\'t change bounds when maximum size is set', () => {
        w.setMenu(null);
        w.setMaximumSize(400, 400);
        // Without https://github.com/electron/electron/pull/29101
        // following call would shrink the window to 384x361.
        // There would be also DCHECK in resize_utils.cc on
        // debug build.
        w.setAspectRatio(1.0);
        expectBoundsEqual(w.getSize(), [400, 400]);
      });
    });

    describe('BrowserWindow.setPosition(x, y)', () => {
      it('sets the window position', async () => {
        const pos = [10, 10];
        const move = emittedOnce(w, 'move');
        w.setPosition(pos[0], pos[1]);
        await move;
        expect(w.getPosition()).to.deep.equal(pos);
      });
    });

    describe('BrowserWindow.setContentSize(width, height)', () => {
      it('sets the content size', async () => {
        // NB. The CI server has a very small screen. Attempting to size the window
        // larger than the screen will limit the window's size to the screen and
        // cause the test to fail.
        const size = [456, 567];
        w.setContentSize(size[0], size[1]);
        await new Promise(setImmediate);
        const after = w.getContentSize();
        expect(after).to.deep.equal(size);
      });

      it('works for a frameless window', async () => {
        w.destroy();
        w = new BrowserWindow({
          show: false,
          frame: false,
          width: 400,
          height: 400
        });
        const size = [456, 567];
        w.setContentSize(size[0], size[1]);
        await new Promise(setImmediate);
        const after = w.getContentSize();
        expect(after).to.deep.equal(size);
      });
    });

    describe('BrowserWindow.setContentBounds(bounds)', () => {
      it('sets the content size and position', async () => {
        const bounds = { x: 10, y: 10, width: 250, height: 250 };
        const resize = emittedOnce(w, 'resize');
        w.setContentBounds(bounds);
        await resize;
        await delay();
        expectBoundsEqual(w.getContentBounds(), bounds);
      });
      it('works for a frameless window', async () => {
        w.destroy();
        w = new BrowserWindow({
          show: false,
          frame: false,
          width: 300,
          height: 300
        });
        const bounds = { x: 10, y: 10, width: 250, height: 250 };
        const resize = emittedOnce(w, 'resize');
        w.setContentBounds(bounds);
        await resize;
        await delay();
        expectBoundsEqual(w.getContentBounds(), bounds);
      });
    });

    describe('BrowserWindow.getBackgroundColor()', () => {
      it('returns default value if no backgroundColor is set', () => {
        w.destroy();
        w = new BrowserWindow({});
        expect(w.getBackgroundColor()).to.equal('#FFFFFF');
      });
      it('returns correct value if backgroundColor is set', () => {
        const backgroundColor = '#BBAAFF';
        w.destroy();
        w = new BrowserWindow({
          backgroundColor: backgroundColor
        });
        expect(w.getBackgroundColor()).to.equal(backgroundColor);
      });
      it('returns correct value from setBackgroundColor()', () => {
        const backgroundColor = '#AABBFF';
        w.destroy();
        w = new BrowserWindow({});
        w.setBackgroundColor(backgroundColor);
        expect(w.getBackgroundColor()).to.equal(backgroundColor);
      });
    });

    describe('BrowserWindow.getNormalBounds()', () => {
      describe('Normal state', () => {
        it('checks normal bounds after resize', async () => {
          const size = [300, 400];
          const resize = emittedOnce(w, 'resize');
          w.setSize(size[0], size[1]);
          await resize;
          expectBoundsEqual(w.getNormalBounds(), w.getBounds());
        });
        it('checks normal bounds after move', async () => {
          const pos = [10, 10];
          const move = emittedOnce(w, 'move');
          w.setPosition(pos[0], pos[1]);
          await move;
          expectBoundsEqual(w.getNormalBounds(), w.getBounds());
        });
      });

      ifdescribe(process.platform !== 'linux')('Maximized state', () => {
        it('checks normal bounds when maximized', async () => {
          const bounds = w.getBounds();
          const maximize = emittedOnce(w, 'maximize');
          w.show();
          w.maximize();
          await maximize;
          expectBoundsEqual(w.getNormalBounds(), bounds);
        });
        it('checks normal bounds when unmaximized', async () => {
          const bounds = w.getBounds();
          w.once('maximize', () => {
            w.unmaximize();
          });
          const unmaximize = emittedOnce(w, 'unmaximize');
          w.show();
          w.maximize();
          await unmaximize;
          expectBoundsEqual(w.getNormalBounds(), bounds);
        });
        it('does not change size for a frameless window with min size', async () => {
          w.destroy();
          w = new BrowserWindow({
            show: false,
            frame: false,
            width: 300,
            height: 300,
            minWidth: 300,
            minHeight: 300
          });
          const bounds = w.getBounds();
          w.once('maximize', () => {
            w.unmaximize();
          });
          const unmaximize = emittedOnce(w, 'unmaximize');
          w.show();
          w.maximize();
          await unmaximize;
          expectBoundsEqual(w.getNormalBounds(), bounds);
        });
        it('correctly checks transparent window maximization state', async () => {
          w.destroy();
          w = new BrowserWindow({
            show: false,
            width: 300,
            height: 300,
            transparent: true
          });

          const maximize = emittedOnce(w, 'maximize');
          w.show();
          w.maximize();
          await maximize;
          expect(w.isMaximized()).to.equal(true);
          const unmaximize = emittedOnce(w, 'unmaximize');
          w.unmaximize();
          await unmaximize;
          expect(w.isMaximized()).to.equal(false);
        });
        it('returns the correct value for windows with an aspect ratio', async () => {
          w.destroy();
          w = new BrowserWindow({
            show: false,
            fullscreenable: false
          });

          w.setAspectRatio(16 / 11);

          const maximize = emittedOnce(w, 'resize');
          w.show();
          w.maximize();
          await maximize;

          expect(w.isMaximized()).to.equal(true);
          w.resizable = false;
          expect(w.isMaximized()).to.equal(true);
        });
      });

      ifdescribe(process.platform !== 'linux')('Minimized state', () => {
        it('checks normal bounds when minimized', async () => {
          const bounds = w.getBounds();
          const minimize = emittedOnce(w, 'minimize');
          w.show();
          w.minimize();
          await minimize;
          expectBoundsEqual(w.getNormalBounds(), bounds);
        });
        it('checks normal bounds when restored', async () => {
          const bounds = w.getBounds();
          w.once('minimize', () => {
            w.restore();
          });
          const restore = emittedOnce(w, 'restore');
          w.show();
          w.minimize();
          await restore;
          expectBoundsEqual(w.getNormalBounds(), bounds);
        });
        it('does not change size for a frameless window with min size', async () => {
          w.destroy();
          w = new BrowserWindow({
            show: false,
            frame: false,
            width: 300,
            height: 300,
            minWidth: 300,
            minHeight: 300
          });
          const bounds = w.getBounds();
          w.once('minimize', () => {
            w.restore();
          });
          const restore = emittedOnce(w, 'restore');
          w.show();
          w.minimize();
          await restore;
          expectBoundsEqual(w.getNormalBounds(), bounds);
        });
      });

      ifdescribe(process.platform === 'win32')('Fullscreen state', () => {
        it('with properties', () => {
          it('can be set with the fullscreen constructor option', () => {
            w = new BrowserWindow({ fullscreen: true });
            expect(w.fullScreen).to.be.true();
          });

          it('can be changed', () => {
            w.fullScreen = false;
            expect(w.fullScreen).to.be.false();
            w.fullScreen = true;
            expect(w.fullScreen).to.be.true();
          });

          it('checks normal bounds when fullscreen\'ed', async () => {
            const bounds = w.getBounds();
            const enterFullScreen = emittedOnce(w, 'enter-full-screen');
            w.show();
            w.fullScreen = true;
            await enterFullScreen;
            expectBoundsEqual(w.getNormalBounds(), bounds);
          });

          it('checks normal bounds when unfullscreen\'ed', async () => {
            const bounds = w.getBounds();
            w.once('enter-full-screen', () => {
              w.fullScreen = false;
            });
            const leaveFullScreen = emittedOnce(w, 'leave-full-screen');
            w.show();
            w.fullScreen = true;
            await leaveFullScreen;
            expectBoundsEqual(w.getNormalBounds(), bounds);
          });
        });

        it('with functions', () => {
          it('can be set with the fullscreen constructor option', () => {
            w = new BrowserWindow({ fullscreen: true });
            expect(w.isFullScreen()).to.be.true();
          });

          it('can be changed', () => {
            w.setFullScreen(false);
            expect(w.isFullScreen()).to.be.false();
            w.setFullScreen(true);
            expect(w.isFullScreen()).to.be.true();
          });

          it('checks normal bounds when fullscreen\'ed', async () => {
            const bounds = w.getBounds();
            w.show();

            const enterFullScreen = emittedOnce(w, 'enter-full-screen');
            w.setFullScreen(true);
            await enterFullScreen;

            expectBoundsEqual(w.getNormalBounds(), bounds);
          });

          it('checks normal bounds when unfullscreen\'ed', async () => {
            const bounds = w.getBounds();
            w.show();

            const enterFullScreen = emittedOnce(w, 'enter-full-screen');
            w.setFullScreen(true);
            await enterFullScreen;

            const leaveFullScreen = emittedOnce(w, 'leave-full-screen');
            w.setFullScreen(false);
            await leaveFullScreen;

            expectBoundsEqual(w.getNormalBounds(), bounds);
          });
        });
      });
    });
  });

  ifdescribe(process.platform === 'darwin')('tabbed windows', () => {
    let w = null as unknown as BrowserWindow;
    beforeEach(() => {
      w = new BrowserWindow({ show: false });
    });
    afterEach(async () => {
      await closeWindow(w);
      w = null as unknown as BrowserWindow;
    });

    describe('BrowserWindow.selectPreviousTab()', () => {
      it('does not throw', () => {
        expect(() => {
          w.selectPreviousTab();
        }).to.not.throw();
      });
    });

    describe('BrowserWindow.selectNextTab()', () => {
      it('does not throw', () => {
        expect(() => {
          w.selectNextTab();
        }).to.not.throw();
      });
    });

    describe('BrowserWindow.mergeAllWindows()', () => {
      it('does not throw', () => {
        expect(() => {
          w.mergeAllWindows();
        }).to.not.throw();
      });
    });

    describe('BrowserWindow.moveTabToNewWindow()', () => {
      it('does not throw', () => {
        expect(() => {
          w.moveTabToNewWindow();
        }).to.not.throw();
      });
    });

    describe('BrowserWindow.toggleTabBar()', () => {
      it('does not throw', () => {
        expect(() => {
          w.toggleTabBar();
        }).to.not.throw();
      });
    });

    describe('BrowserWindow.addTabbedWindow()', () => {
      it('does not throw', async () => {
        const tabbedWindow = new BrowserWindow({});
        expect(() => {
          w.addTabbedWindow(tabbedWindow);
        }).to.not.throw();

        expect(BrowserWindow.getAllWindows()).to.have.lengthOf(2); // w + tabbedWindow

        await closeWindow(tabbedWindow, { assertNotWindows: false });
        expect(BrowserWindow.getAllWindows()).to.have.lengthOf(1); // w
      });

      it('throws when called on itself', () => {
        expect(() => {
          w.addTabbedWindow(w);
        }).to.throw('AddTabbedWindow cannot be called by a window on itself.');
      });
    });
  });

  describe('autoHideMenuBar state', () => {
    afterEach(closeAllWindows);

    it('for properties', () => {
      it('can be set with autoHideMenuBar constructor option', () => {
        const w = new BrowserWindow({ show: false, autoHideMenuBar: true });
        expect(w.autoHideMenuBar).to.be.true('autoHideMenuBar');
      });

      it('can be changed', () => {
        const w = new BrowserWindow({ show: false });
        expect(w.autoHideMenuBar).to.be.false('autoHideMenuBar');
        w.autoHideMenuBar = true;
        expect(w.autoHideMenuBar).to.be.true('autoHideMenuBar');
        w.autoHideMenuBar = false;
        expect(w.autoHideMenuBar).to.be.false('autoHideMenuBar');
      });
    });

    it('for functions', () => {
      it('can be set with autoHideMenuBar constructor option', () => {
        const w = new BrowserWindow({ show: false, autoHideMenuBar: true });
        expect(w.isMenuBarAutoHide()).to.be.true('autoHideMenuBar');
      });

      it('can be changed', () => {
        const w = new BrowserWindow({ show: false });
        expect(w.isMenuBarAutoHide()).to.be.false('autoHideMenuBar');
        w.setAutoHideMenuBar(true);
        expect(w.isMenuBarAutoHide()).to.be.true('autoHideMenuBar');
        w.setAutoHideMenuBar(false);
        expect(w.isMenuBarAutoHide()).to.be.false('autoHideMenuBar');
      });
    });
  });

  describe('BrowserWindow.capturePage(rect)', () => {
    afterEach(closeAllWindows);

    it('returns a Promise with a Buffer', async () => {
      const w = new BrowserWindow({ show: false });
      const image = await w.capturePage({
        x: 0,
        y: 0,
        width: 100,
        height: 100
      });

      expect(image.isEmpty()).to.equal(true);
    });

    it('resolves after the window is hidden', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadFile(path.join(fixtures, 'pages', 'a.html'));
      await emittedOnce(w, 'ready-to-show');
      w.show();

      const visibleImage = await w.capturePage();
      expect(visibleImage.isEmpty()).to.equal(false);

      w.hide();

      const hiddenImage = await w.capturePage();
      const isEmpty = process.platform !== 'darwin';
      expect(hiddenImage.isEmpty()).to.equal(isEmpty);
    });

    it('preserves transparency', async () => {
      const w = new BrowserWindow({ show: false, transparent: true });
      w.loadFile(path.join(fixtures, 'pages', 'theme-color.html'));
      await emittedOnce(w, 'ready-to-show');
      w.show();

      const image = await w.capturePage();
      const imgBuffer = image.toPNG();

      // Check the 25th byte in the PNG.
      // Values can be 0,2,3,4, or 6. We want 6, which is RGB + Alpha
      expect(imgBuffer[25]).to.equal(6);
    });
  });

  describe('BrowserWindow.setProgressBar(progress)', () => {
    let w = null as unknown as BrowserWindow;
    before(() => {
      w = new BrowserWindow({ show: false });
    });
    after(async () => {
      await closeWindow(w);
      w = null as unknown as BrowserWindow;
    });
    it('sets the progress', () => {
      expect(() => {
        if (process.platform === 'darwin') {
          app.dock.setIcon(path.join(fixtures, 'assets', 'logo.png'));
        }
        w.setProgressBar(0.5);

        if (process.platform === 'darwin') {
          app.dock.setIcon(null as any);
        }
        w.setProgressBar(-1);
      }).to.not.throw();
    });
    it('sets the progress using "paused" mode', () => {
      expect(() => {
        w.setProgressBar(0.5, { mode: 'paused' });
      }).to.not.throw();
    });
    it('sets the progress using "error" mode', () => {
      expect(() => {
        w.setProgressBar(0.5, { mode: 'error' });
      }).to.not.throw();
    });
    it('sets the progress using "normal" mode', () => {
      expect(() => {
        w.setProgressBar(0.5, { mode: 'normal' });
      }).to.not.throw();
    });
  });

  describe('BrowserWindow.setAlwaysOnTop(flag, level)', () => {
    let w = null as unknown as BrowserWindow;

    afterEach(closeAllWindows);

    beforeEach(() => {
      w = new BrowserWindow({ show: true });
    });

    it('sets the window as always on top', () => {
      expect(w.isAlwaysOnTop()).to.be.false('is alwaysOnTop');
      w.setAlwaysOnTop(true, 'screen-saver');
      expect(w.isAlwaysOnTop()).to.be.true('is not alwaysOnTop');
      w.setAlwaysOnTop(false);
      expect(w.isAlwaysOnTop()).to.be.false('is alwaysOnTop');
      w.setAlwaysOnTop(true);
      expect(w.isAlwaysOnTop()).to.be.true('is not alwaysOnTop');
    });

    ifit(process.platform === 'darwin')('resets the windows level on minimize', () => {
      expect(w.isAlwaysOnTop()).to.be.false('is alwaysOnTop');
      w.setAlwaysOnTop(true, 'screen-saver');
      expect(w.isAlwaysOnTop()).to.be.true('is not alwaysOnTop');
      w.minimize();
      expect(w.isAlwaysOnTop()).to.be.false('is alwaysOnTop');
      w.restore();
      expect(w.isAlwaysOnTop()).to.be.true('is not alwaysOnTop');
    });

    it('causes the right value to be emitted on `always-on-top-changed`', async () => {
      const alwaysOnTopChanged = emittedOnce(w, 'always-on-top-changed');
      expect(w.isAlwaysOnTop()).to.be.false('is alwaysOnTop');
      w.setAlwaysOnTop(true);
      const [, alwaysOnTop] = await alwaysOnTopChanged;
      expect(alwaysOnTop).to.be.true('is not alwaysOnTop');
    });

    ifit(process.platform === 'darwin')('honors the alwaysOnTop level of a child window', () => {
      w = new BrowserWindow({ show: false });
      const c = new BrowserWindow({ parent: w });
      c.setAlwaysOnTop(true, 'screen-saver');

      expect(w.isAlwaysOnTop()).to.be.false();
      expect(c.isAlwaysOnTop()).to.be.true('child is not always on top');
      expect((c as any)._getAlwaysOnTopLevel()).to.equal('screen-saver');
    });
  });

  describe('preconnect feature', () => {
    let w = null as unknown as BrowserWindow;

    let server = null as unknown as http.Server;
    let url = null as unknown as string;
    let connections = 0;

    beforeEach(async () => {
      connections = 0;
      server = http.createServer((req, res) => {
        if (req.url === '/link') {
          res.setHeader('Content-type', 'text/html');
          res.end('<head><link rel="preconnect" href="//example.com" /></head><body>foo</body>');
          return;
        }
        res.end();
      });
      server.on('connection', () => { connections++; });

      await new Promise<void>(resolve => server.listen(0, '127.0.0.1', () => resolve()));
      url = `http://127.0.0.1:${(server.address() as AddressInfo).port}`;
    });
    afterEach(async () => {
      server.close();
      await closeWindow(w);
      w = null as unknown as BrowserWindow;
      server = null as unknown as http.Server;
    });

    it('calling preconnect() connects to the server', async () => {
      w = new BrowserWindow({ show: false });
      w.webContents.on('did-start-navigation', (event, url) => {
        w.webContents.session.preconnect({ url, numSockets: 4 });
      });
      await w.loadURL(url);
      expect(connections).to.equal(4);
    });

    it('does not preconnect unless requested', async () => {
      w = new BrowserWindow({ show: false });
      await w.loadURL(url);
      expect(connections).to.equal(1);
    });

    it('parses <link rel=preconnect>', async () => {
      w = new BrowserWindow({ show: true });
      const p = emittedOnce(w.webContents.session, 'preconnect');
      w.loadURL(url + '/link');
      const [, preconnectUrl, allowCredentials] = await p;
      expect(preconnectUrl).to.equal('http://example.com/');
      expect(allowCredentials).to.be.true('allowCredentials');
    });
  });

  describe('BrowserWindow.setAutoHideCursor(autoHide)', () => {
    let w = null as unknown as BrowserWindow;
    beforeEach(() => {
      w = new BrowserWindow({ show: false });
    });
    afterEach(async () => {
      await closeWindow(w);
      w = null as unknown as BrowserWindow;
    });

    ifit(process.platform === 'darwin')('on macOS', () => {
      it('allows changing cursor auto-hiding', () => {
        expect(() => {
          w.setAutoHideCursor(false);
          w.setAutoHideCursor(true);
        }).to.not.throw();
      });
    });

    ifit(process.platform !== 'darwin')('on non-macOS platforms', () => {
      it('is not available', () => {
        expect(w.setAutoHideCursor).to.be.undefined('setAutoHideCursor function');
      });
    });
  });

  ifdescribe(process.platform === 'darwin')('BrowserWindow.setWindowButtonVisibility()', () => {
    afterEach(closeAllWindows);

    it('does not throw', () => {
      const w = new BrowserWindow({ show: false });
      expect(() => {
        w.setWindowButtonVisibility(true);
        w.setWindowButtonVisibility(false);
      }).to.not.throw();
    });

    it('changes window button visibility for normal window', () => {
      const w = new BrowserWindow({ show: false });
      expect(w._getWindowButtonVisibility()).to.equal(true);
      w.setWindowButtonVisibility(false);
      expect(w._getWindowButtonVisibility()).to.equal(false);
      w.setWindowButtonVisibility(true);
      expect(w._getWindowButtonVisibility()).to.equal(true);
    });

    it('changes window button visibility for frameless window', () => {
      const w = new BrowserWindow({ show: false, frame: false });
      expect(w._getWindowButtonVisibility()).to.equal(false);
      w.setWindowButtonVisibility(true);
      expect(w._getWindowButtonVisibility()).to.equal(true);
      w.setWindowButtonVisibility(false);
      expect(w._getWindowButtonVisibility()).to.equal(false);
    });

    it('changes window button visibility for hiddenInset window', () => {
      const w = new BrowserWindow({ show: false, frame: false, titleBarStyle: 'hiddenInset' });
      expect(w._getWindowButtonVisibility()).to.equal(true);
      w.setWindowButtonVisibility(false);
      expect(w._getWindowButtonVisibility()).to.equal(false);
      w.setWindowButtonVisibility(true);
      expect(w._getWindowButtonVisibility()).to.equal(true);
    });

    // Buttons of customButtonsOnHover are always hidden unless hovered.
    it('does not change window button visibility for customButtonsOnHover window', () => {
      const w = new BrowserWindow({ show: false, frame: false, titleBarStyle: 'customButtonsOnHover' });
      expect(w._getWindowButtonVisibility()).to.equal(false);
      w.setWindowButtonVisibility(true);
      expect(w._getWindowButtonVisibility()).to.equal(false);
      w.setWindowButtonVisibility(false);
      expect(w._getWindowButtonVisibility()).to.equal(false);
    });
  });

  ifdescribe(process.platform === 'darwin')('BrowserWindow.setVibrancy(type)', () => {
    afterEach(closeAllWindows);

    it('allows setting, changing, and removing the vibrancy', () => {
      const w = new BrowserWindow({ show: false });
      expect(() => {
        w.setVibrancy('light');
        w.setVibrancy('dark');
        w.setVibrancy(null);
        w.setVibrancy('ultra-dark');
        w.setVibrancy('' as any);
      }).to.not.throw();
    });

    it('does not crash if vibrancy is set to an invalid value', () => {
      const w = new BrowserWindow({ show: false });
      expect(() => {
        w.setVibrancy('i-am-not-a-valid-vibrancy-type' as any);
      }).to.not.throw();
    });
  });

  ifdescribe(process.platform === 'darwin')('trafficLightPosition', () => {
    const pos = { x: 10, y: 10 };
    afterEach(closeAllWindows);

    describe('BrowserWindow.getTrafficLightPosition(pos)', () => {
      it('gets position property for "hidden" titleBarStyle', () => {
        const w = new BrowserWindow({ show: false, titleBarStyle: 'hidden', trafficLightPosition: pos });
        expect(w.getTrafficLightPosition()).to.deep.equal(pos);
      });

      it('gets position property for "customButtonsOnHover" titleBarStyle', () => {
        const w = new BrowserWindow({ show: false, titleBarStyle: 'customButtonsOnHover', trafficLightPosition: pos });
        expect(w.getTrafficLightPosition()).to.deep.equal(pos);
      });
    });

    describe('BrowserWindow.setTrafficLightPosition(pos)', () => {
      it('sets position property for "hidden" titleBarStyle', () => {
        const w = new BrowserWindow({ show: false, titleBarStyle: 'hidden', trafficLightPosition: pos });
        const newPos = { x: 20, y: 20 };
        w.setTrafficLightPosition(newPos);
        expect(w.getTrafficLightPosition()).to.deep.equal(newPos);
      });

      it('sets position property for "customButtonsOnHover" titleBarStyle', () => {
        const w = new BrowserWindow({ show: false, titleBarStyle: 'customButtonsOnHover', trafficLightPosition: pos });
        const newPos = { x: 20, y: 20 };
        w.setTrafficLightPosition(newPos);
        expect(w.getTrafficLightPosition()).to.deep.equal(newPos);
      });
    });
  });

  ifdescribe(process.platform === 'win32')('BrowserWindow.setAppDetails(options)', () => {
    afterEach(closeAllWindows);

    it('supports setting the app details', () => {
      const w = new BrowserWindow({ show: false });
      const iconPath = path.join(fixtures, 'assets', 'icon.ico');

      expect(() => {
        w.setAppDetails({ appId: 'my.app.id' });
        w.setAppDetails({ appIconPath: iconPath, appIconIndex: 0 });
        w.setAppDetails({ appIconPath: iconPath });
        w.setAppDetails({ relaunchCommand: 'my-app.exe arg1 arg2', relaunchDisplayName: 'My app name' });
        w.setAppDetails({ relaunchCommand: 'my-app.exe arg1 arg2' });
        w.setAppDetails({ relaunchDisplayName: 'My app name' });
        w.setAppDetails({
          appId: 'my.app.id',
          appIconPath: iconPath,
          appIconIndex: 0,
          relaunchCommand: 'my-app.exe arg1 arg2',
          relaunchDisplayName: 'My app name'
        });
        w.setAppDetails({});
      }).to.not.throw();

      expect(() => {
        (w.setAppDetails as any)();
      }).to.throw('Insufficient number of arguments.');
    });
  });

  describe('BrowserWindow.fromId(id)', () => {
    afterEach(closeAllWindows);
    it('returns the window with id', () => {
      const w = new BrowserWindow({ show: false });
      expect(BrowserWindow.fromId(w.id)!.id).to.equal(w.id);
    });
  });

  describe('BrowserWindow.fromWebContents(webContents)', () => {
    afterEach(closeAllWindows);

    it('returns the window with the webContents', () => {
      const w = new BrowserWindow({ show: false });
      const found = BrowserWindow.fromWebContents(w.webContents);
      expect(found!.id).to.equal(w.id);
    });

    it('returns null for webContents without a BrowserWindow', () => {
      const contents = (webContents as any).create({});
      try {
        expect(BrowserWindow.fromWebContents(contents)).to.be.null('BrowserWindow.fromWebContents(contents)');
      } finally {
        contents.destroy();
      }
    });

    it('returns the correct window for a BrowserView webcontents', async () => {
      const w = new BrowserWindow({ show: false });
      const bv = new BrowserView();
      w.setBrowserView(bv);
      defer(() => {
        w.removeBrowserView(bv);
        (bv.webContents as any).destroy();
      });
      await bv.webContents.loadURL('about:blank');
      expect(BrowserWindow.fromWebContents(bv.webContents)!.id).to.equal(w.id);
    });

    it('returns the correct window for a WebView webcontents', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { webviewTag: true } });
      w.loadURL('data:text/html,<webview src="data:text/html,hi"></webview>');
      // NOTE(nornagon): Waiting for 'did-attach-webview' is a workaround for
      // https://github.com/electron/electron/issues/25413, and is not integral
      // to the test.
      const p = emittedOnce(w.webContents, 'did-attach-webview');
      const [, webviewContents] = await emittedOnce(app, 'web-contents-created');
      expect(BrowserWindow.fromWebContents(webviewContents)!.id).to.equal(w.id);
      await p;
    });
  });

  describe('BrowserWindow.openDevTools()', () => {
    afterEach(closeAllWindows);
    it('does not crash for frameless window', () => {
      const w = new BrowserWindow({ show: false, frame: false });
      w.webContents.openDevTools();
    });
  });

  describe('BrowserWindow.fromBrowserView(browserView)', () => {
    afterEach(closeAllWindows);

    it('returns the window with the BrowserView', () => {
      const w = new BrowserWindow({ show: false });
      const bv = new BrowserView();
      w.setBrowserView(bv);
      defer(() => {
        w.removeBrowserView(bv);
        (bv.webContents as any).destroy();
      });
      expect(BrowserWindow.fromBrowserView(bv)!.id).to.equal(w.id);
    });

    it('returns the window when there are multiple BrowserViews', () => {
      const w = new BrowserWindow({ show: false });
      const bv1 = new BrowserView();
      w.addBrowserView(bv1);
      const bv2 = new BrowserView();
      w.addBrowserView(bv2);
      defer(() => {
        w.removeBrowserView(bv1);
        w.removeBrowserView(bv2);
        (bv1.webContents as any).destroy();
        (bv2.webContents as any).destroy();
      });
      expect(BrowserWindow.fromBrowserView(bv1)!.id).to.equal(w.id);
      expect(BrowserWindow.fromBrowserView(bv2)!.id).to.equal(w.id);
    });

    it('returns undefined if not attached', () => {
      const bv = new BrowserView();
      defer(() => {
        (bv.webContents as any).destroy();
      });
      expect(BrowserWindow.fromBrowserView(bv)).to.be.null('BrowserWindow associated with bv');
    });
  });

  describe('BrowserWindow.setOpacity(opacity)', () => {
    afterEach(closeAllWindows);

    ifdescribe(process.platform !== 'linux')(('Windows and Mac'), () => {
      it('make window with initial opacity', () => {
        const w = new BrowserWindow({ show: false, opacity: 0.5 });
        expect(w.getOpacity()).to.equal(0.5);
      });
      it('allows setting the opacity', () => {
        const w = new BrowserWindow({ show: false });
        expect(() => {
          w.setOpacity(0.0);
          expect(w.getOpacity()).to.equal(0.0);
          w.setOpacity(0.5);
          expect(w.getOpacity()).to.equal(0.5);
          w.setOpacity(1.0);
          expect(w.getOpacity()).to.equal(1.0);
        }).to.not.throw();
      });

      it('clamps opacity to [0.0...1.0]', () => {
        const w = new BrowserWindow({ show: false, opacity: 0.5 });
        w.setOpacity(100);
        expect(w.getOpacity()).to.equal(1.0);
        w.setOpacity(-100);
        expect(w.getOpacity()).to.equal(0.0);
      });
    });

    ifdescribe(process.platform === 'linux')(('Linux'), () => {
      it('sets 1 regardless of parameter', () => {
        const w = new BrowserWindow({ show: false });
        w.setOpacity(0);
        expect(w.getOpacity()).to.equal(1.0);
        w.setOpacity(0.5);
        expect(w.getOpacity()).to.equal(1.0);
      });
    });
  });

  describe('BrowserWindow.setShape(rects)', () => {
    afterEach(closeAllWindows);
    it('allows setting shape', () => {
      const w = new BrowserWindow({ show: false });
      expect(() => {
        w.setShape([]);
        w.setShape([{ x: 0, y: 0, width: 100, height: 100 }]);
        w.setShape([{ x: 0, y: 0, width: 100, height: 100 }, { x: 0, y: 200, width: 1000, height: 100 }]);
        w.setShape([]);
      }).to.not.throw();
    });
  });

  describe('"useContentSize" option', () => {
    afterEach(closeAllWindows);
    it('make window created with content size when used', () => {
      const w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        useContentSize: true
      });
      const contentSize = w.getContentSize();
      expect(contentSize).to.deep.equal([400, 400]);
    });
    it('make window created with window size when not used', () => {
      const w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400
      });
      const size = w.getSize();
      expect(size).to.deep.equal([400, 400]);
    });
    it('works for a frameless window', () => {
      const w = new BrowserWindow({
        show: false,
        frame: false,
        width: 400,
        height: 400,
        useContentSize: true
      });
      const contentSize = w.getContentSize();
      expect(contentSize).to.deep.equal([400, 400]);
      const size = w.getSize();
      expect(size).to.deep.equal([400, 400]);
    });
  });

  ifdescribe(process.platform === 'win32' || (process.platform === 'darwin' && semver.gte(os.release(), '14.0.0')))('"titleBarStyle" option', () => {
    const testWindowsOverlay = async (style: any) => {
      const w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        titleBarStyle: style,
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false
        },
        titleBarOverlay: true
      });
      const overlayHTML = path.join(__dirname, 'fixtures', 'pages', 'overlay.html');
      if (process.platform === 'darwin') {
        await w.loadFile(overlayHTML);
      } else {
        const overlayReady = emittedOnce(ipcMain, 'geometrychange');
        await w.loadFile(overlayHTML);
        await overlayReady;
      }
      const overlayEnabled = await w.webContents.executeJavaScript('navigator.windowControlsOverlay.visible');
      expect(overlayEnabled).to.be.true('overlayEnabled');
      const overlayRect = await w.webContents.executeJavaScript('getJSOverlayProperties()');
      expect(overlayRect.y).to.equal(0);
      if (process.platform === 'darwin') {
        expect(overlayRect.x).to.be.greaterThan(0);
      } else {
        expect(overlayRect.x).to.equal(0);
      }
      expect(overlayRect.width).to.be.greaterThan(0);
      expect(overlayRect.height).to.be.greaterThan(0);
      const cssOverlayRect = await w.webContents.executeJavaScript('getCssOverlayProperties();');
      expect(cssOverlayRect).to.deep.equal(overlayRect);
      const geometryChange = emittedOnce(ipcMain, 'geometrychange');
      w.setBounds({ width: 800 });
      const [, newOverlayRect] = await geometryChange;
      expect(newOverlayRect.width).to.equal(overlayRect.width + 400);
    };
    afterEach(closeAllWindows);
    afterEach(() => { ipcMain.removeAllListeners('geometrychange'); });
    it('creates browser window with hidden title bar', () => {
      const w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        titleBarStyle: 'hidden'
      });
      const contentSize = w.getContentSize();
      expect(contentSize).to.deep.equal([400, 400]);
    });
    ifit(process.platform === 'darwin')('creates browser window with hidden inset title bar', () => {
      const w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        titleBarStyle: 'hiddenInset'
      });
      const contentSize = w.getContentSize();
      expect(contentSize).to.deep.equal([400, 400]);
    });
    it('sets Window Control Overlay with hidden title bar', async () => {
      await testWindowsOverlay('hidden');
    });
    ifit(process.platform === 'darwin')('sets Window Control Overlay with hidden inset title bar', async () => {
      await testWindowsOverlay('hiddenInset');
    });
  });

  ifdescribe(process.platform === 'win32' || (process.platform === 'darwin' && semver.gte(os.release(), '14.0.0')))('"titleBarOverlay" option', () => {
    const testWindowsOverlayHeight = async (size: any) => {
      const w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        titleBarStyle: 'hidden',
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false
        },
        titleBarOverlay: {
          height: size
        }
      });
      const overlayHTML = path.join(__dirname, 'fixtures', 'pages', 'overlay.html');
      if (process.platform === 'darwin') {
        await w.loadFile(overlayHTML);
      } else {
        const overlayReady = emittedOnce(ipcMain, 'geometrychange');
        await w.loadFile(overlayHTML);
        await overlayReady;
      }
      const overlayEnabled = await w.webContents.executeJavaScript('navigator.windowControlsOverlay.visible');
      expect(overlayEnabled).to.be.true('overlayEnabled');
      const overlayRectPreMax = await w.webContents.executeJavaScript('getJSOverlayProperties()');
      await w.maximize();
      const max = await w.isMaximized();
      expect(max).to.equal(true);
      const overlayRectPostMax = await w.webContents.executeJavaScript('getJSOverlayProperties()');

      expect(overlayRectPreMax.y).to.equal(0);
      if (process.platform === 'darwin') {
        expect(overlayRectPreMax.x).to.be.greaterThan(0);
      } else {
        expect(overlayRectPreMax.x).to.equal(0);
      }
      expect(overlayRectPreMax.width).to.be.greaterThan(0);

      expect(overlayRectPreMax.height).to.equal(size);
      // Confirm that maximization only affected the height of the buttons and not the title bar
      expect(overlayRectPostMax.height).to.equal(size);
    };
    afterEach(closeAllWindows);
    afterEach(() => { ipcMain.removeAllListeners('geometrychange'); });
    it('sets Window Control Overlay with title bar height of 40', async () => {
      await testWindowsOverlayHeight(40);
    });
  });

  ifdescribe(process.platform === 'darwin')('"enableLargerThanScreen" option', () => {
    afterEach(closeAllWindows);
    it('can move the window out of screen', () => {
      const w = new BrowserWindow({ show: true, enableLargerThanScreen: true });
      w.setPosition(-10, 50);
      const after = w.getPosition();
      expect(after).to.deep.equal([-10, 50]);
    });
    it('cannot move the window behind menu bar', () => {
      const w = new BrowserWindow({ show: true, enableLargerThanScreen: true });
      w.setPosition(-10, -10);
      const after = w.getPosition();
      expect(after[1]).to.be.at.least(0);
    });
    it('can move the window behind menu bar if it has no frame', () => {
      const w = new BrowserWindow({ show: true, enableLargerThanScreen: true, frame: false });
      w.setPosition(-10, -10);
      const after = w.getPosition();
      expect(after[0]).to.be.equal(-10);
      expect(after[1]).to.be.equal(-10);
    });
    it('without it, cannot move the window out of screen', () => {
      const w = new BrowserWindow({ show: true, enableLargerThanScreen: false });
      w.setPosition(-10, -10);
      const after = w.getPosition();
      expect(after[1]).to.be.at.least(0);
    });
    it('can set the window larger than screen', () => {
      const w = new BrowserWindow({ show: true, enableLargerThanScreen: true });
      const size = screen.getPrimaryDisplay().size;
      size.width += 100;
      size.height += 100;
      w.setSize(size.width, size.height);
      expectBoundsEqual(w.getSize(), [size.width, size.height]);
    });
    it('without it, cannot set the window larger than screen', () => {
      const w = new BrowserWindow({ show: true, enableLargerThanScreen: false });
      const size = screen.getPrimaryDisplay().size;
      size.width += 100;
      size.height += 100;
      w.setSize(size.width, size.height);
      expect(w.getSize()[1]).to.at.most(screen.getPrimaryDisplay().size.height);
    });
  });

  ifdescribe(process.platform === 'darwin')('"zoomToPageWidth" option', () => {
    afterEach(closeAllWindows);
    it('sets the window width to the page width when used', () => {
      const w = new BrowserWindow({
        show: false,
        width: 500,
        height: 400,
        zoomToPageWidth: true
      });
      w.maximize();
      expect(w.getSize()[0]).to.equal(500);
    });
  });

  describe('"tabbingIdentifier" option', () => {
    afterEach(closeAllWindows);
    it('can be set on a window', () => {
      expect(() => {
        /* eslint-disable no-new */
        new BrowserWindow({
          tabbingIdentifier: 'group1'
        });
        new BrowserWindow({
          tabbingIdentifier: 'group2',
          frame: false
        });
        /* eslint-enable no-new */
      }).not.to.throw();
    });
  });

  describe('"webPreferences" option', () => {
    afterEach(() => { ipcMain.removeAllListeners('answer'); });
    afterEach(closeAllWindows);

    describe('"preload" option', () => {
      const doesNotLeakSpec = (name: string, webPrefs: { nodeIntegration: boolean, sandbox: boolean, contextIsolation: boolean }) => {
        it(name, async () => {
          const w = new BrowserWindow({
            webPreferences: {
              ...webPrefs,
              preload: path.resolve(fixtures, 'module', 'empty.js')
            },
            show: false
          });
          w.loadFile(path.join(fixtures, 'api', 'no-leak.html'));
          const [, result] = await emittedOnce(ipcMain, 'leak-result');
          expect(result).to.have.property('require', 'undefined');
          expect(result).to.have.property('exports', 'undefined');
          expect(result).to.have.property('windowExports', 'undefined');
          expect(result).to.have.property('windowPreload', 'undefined');
          expect(result).to.have.property('windowRequire', 'undefined');
        });
      };
      doesNotLeakSpec('does not leak require', {
        nodeIntegration: false,
        sandbox: false,
        contextIsolation: false
      });
      doesNotLeakSpec('does not leak require when sandbox is enabled', {
        nodeIntegration: false,
        sandbox: true,
        contextIsolation: false
      });
      doesNotLeakSpec('does not leak require when context isolation is enabled', {
        nodeIntegration: false,
        sandbox: false,
        contextIsolation: true
      });
      doesNotLeakSpec('does not leak require when context isolation and sandbox are enabled', {
        nodeIntegration: false,
        sandbox: true,
        contextIsolation: true
      });
      it('does not leak any node globals on the window object with nodeIntegration is disabled', async () => {
        let w = new BrowserWindow({
          webPreferences: {
            contextIsolation: false,
            nodeIntegration: false,
            preload: path.resolve(fixtures, 'module', 'empty.js')
          },
          show: false
        });
        w.loadFile(path.join(fixtures, 'api', 'globals.html'));
        const [, notIsolated] = await emittedOnce(ipcMain, 'leak-result');
        expect(notIsolated).to.have.property('globals');

        w.destroy();
        w = new BrowserWindow({
          webPreferences: {
            contextIsolation: true,
            nodeIntegration: false,
            preload: path.resolve(fixtures, 'module', 'empty.js')
          },
          show: false
        });
        w.loadFile(path.join(fixtures, 'api', 'globals.html'));
        const [, isolated] = await emittedOnce(ipcMain, 'leak-result');
        expect(isolated).to.have.property('globals');
        const notIsolatedGlobals = new Set(notIsolated.globals);
        for (const isolatedGlobal of isolated.globals) {
          notIsolatedGlobals.delete(isolatedGlobal);
        }
        expect([...notIsolatedGlobals]).to.deep.equal([], 'non-isoalted renderer should have no additional globals');
      });

      it('loads the script before other scripts in window', async () => {
        const preload = path.join(fixtures, 'module', 'set-global.js');
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            contextIsolation: false,
            preload
          }
        });
        w.loadFile(path.join(fixtures, 'api', 'preload.html'));
        const [, test] = await emittedOnce(ipcMain, 'answer');
        expect(test).to.eql('preload');
      });
      it('has synchronous access to all eventual window APIs', async () => {
        const preload = path.join(fixtures, 'module', 'access-blink-apis.js');
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            contextIsolation: false,
            preload
          }
        });
        w.loadFile(path.join(fixtures, 'api', 'preload.html'));
        const [, test] = await emittedOnce(ipcMain, 'answer');
        expect(test).to.be.an('object');
        expect(test.atPreload).to.be.an('array');
        expect(test.atLoad).to.be.an('array');
        expect(test.atPreload).to.deep.equal(test.atLoad, 'should have access to the same window APIs');
      });
    });

    describe('session preload scripts', function () {
      const preloads = [
        path.join(fixtures, 'module', 'set-global-preload-1.js'),
        path.join(fixtures, 'module', 'set-global-preload-2.js'),
        path.relative(process.cwd(), path.join(fixtures, 'module', 'set-global-preload-3.js'))
      ];
      const defaultSession = session.defaultSession;

      beforeEach(() => {
        expect(defaultSession.getPreloads()).to.deep.equal([]);
        defaultSession.setPreloads(preloads);
      });
      afterEach(() => {
        defaultSession.setPreloads([]);
      });

      it('can set multiple session preload script', () => {
        expect(defaultSession.getPreloads()).to.deep.equal(preloads);
      });

      const generateSpecs = (description: string, sandbox: boolean) => {
        describe(description, () => {
          it('loads the script before other scripts in window including normal preloads', async () => {
            const w = new BrowserWindow({
              show: false,
              webPreferences: {
                sandbox,
                preload: path.join(fixtures, 'module', 'get-global-preload.js'),
                contextIsolation: false
              }
            });
            w.loadURL('about:blank');
            const [, preload1, preload2, preload3] = await emittedOnce(ipcMain, 'vars');
            expect(preload1).to.equal('preload-1');
            expect(preload2).to.equal('preload-1-2');
            expect(preload3).to.be.undefined('preload 3');
          });
        });
      };

      generateSpecs('without sandbox', false);
      generateSpecs('with sandbox', true);
    });

    describe('"additionalArguments" option', () => {
      it('adds extra args to process.argv in the renderer process', async () => {
        const preload = path.join(fixtures, 'module', 'check-arguments.js');
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            preload,
            additionalArguments: ['--my-magic-arg']
          }
        });
        w.loadFile(path.join(fixtures, 'api', 'blank.html'));
        const [, argv] = await emittedOnce(ipcMain, 'answer');
        expect(argv).to.include('--my-magic-arg');
      });

      it('adds extra value args to process.argv in the renderer process', async () => {
        const preload = path.join(fixtures, 'module', 'check-arguments.js');
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            preload,
            additionalArguments: ['--my-magic-arg=foo']
          }
        });
        w.loadFile(path.join(fixtures, 'api', 'blank.html'));
        const [, argv] = await emittedOnce(ipcMain, 'answer');
        expect(argv).to.include('--my-magic-arg=foo');
      });
    });

    describe('"node-integration" option', () => {
      it('disables node integration by default', async () => {
        const preload = path.join(fixtures, 'module', 'send-later.js');
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            preload,
            contextIsolation: false
          }
        });
        w.loadFile(path.join(fixtures, 'api', 'blank.html'));
        const [, typeofProcess, typeofBuffer] = await emittedOnce(ipcMain, 'answer');
        expect(typeofProcess).to.equal('undefined');
        expect(typeofBuffer).to.equal('undefined');
      });
    });

    describe('"sandbox" option', () => {
      const preload = path.join(path.resolve(__dirname, 'fixtures'), 'module', 'preload-sandbox.js');

      let server: http.Server = null as unknown as http.Server;
      let serverUrl: string = null as unknown as string;

      before((done) => {
        server = http.createServer((request, response) => {
          switch (request.url) {
            case '/cross-site':
              response.end(`<html><body><h1>${request.url}</h1></body></html>`);
              break;
            default:
              throw new Error(`unsupported endpoint: ${request.url}`);
          }
        }).listen(0, '127.0.0.1', () => {
          serverUrl = 'http://127.0.0.1:' + (server.address() as AddressInfo).port;
          done();
        });
      });

      after(() => {
        server.close();
      });

      it('exposes ipcRenderer to preload script', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload,
            contextIsolation: false
          }
        });
        w.loadFile(path.join(fixtures, 'api', 'preload.html'));
        const [, test] = await emittedOnce(ipcMain, 'answer');
        expect(test).to.equal('preload');
      });

      it('exposes ipcRenderer to preload script (path has special chars)', async () => {
        const preloadSpecialChars = path.join(fixtures, 'module', 'preload-sandboxæø åü.js');
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload: preloadSpecialChars,
            contextIsolation: false
          }
        });
        w.loadFile(path.join(fixtures, 'api', 'preload.html'));
        const [, test] = await emittedOnce(ipcMain, 'answer');
        expect(test).to.equal('preload');
      });

      it('exposes "loaded" event to preload script', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload
          }
        });
        w.loadURL('about:blank');
        await emittedOnce(ipcMain, 'process-loaded');
      });

      it('exposes "exit" event to preload script', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload,
            contextIsolation: false
          }
        });
        const htmlPath = path.join(__dirname, 'fixtures', 'api', 'sandbox.html?exit-event');
        const pageUrl = 'file://' + htmlPath;
        w.loadURL(pageUrl);
        const [, url] = await emittedOnce(ipcMain, 'answer');
        const expectedUrl = process.platform === 'win32'
          ? 'file:///' + htmlPath.replace(/\\/g, '/')
          : pageUrl;
        expect(url).to.equal(expectedUrl);
      });

      it('should open windows in same domain with cross-scripting enabled', async () => {
        const w = new BrowserWindow({
          show: true,
          webPreferences: {
            sandbox: true,
            preload,
            contextIsolation: false
          }
        });

        w.webContents.setWindowOpenHandler(() => ({
          action: 'allow',
          overrideBrowserWindowOptions: {
            webPreferences: {
              preload
            }
          }
        }));

        const htmlPath = path.join(__dirname, 'fixtures', 'api', 'sandbox.html?window-open');
        const pageUrl = 'file://' + htmlPath;
        const answer = emittedOnce(ipcMain, 'answer');
        w.loadURL(pageUrl);
        const [, { url, frameName, options }] = await emittedOnce(w.webContents, 'did-create-window');
        const expectedUrl = process.platform === 'win32'
          ? 'file:///' + htmlPath.replace(/\\/g, '/')
          : pageUrl;
        expect(url).to.equal(expectedUrl);
        expect(frameName).to.equal('popup!');
        expect(options.width).to.equal(500);
        expect(options.height).to.equal(600);
        const [, html] = await answer;
        expect(html).to.equal('<h1>scripting from opener</h1>');
      });

      it('should open windows in another domain with cross-scripting disabled', async () => {
        const w = new BrowserWindow({
          show: true,
          webPreferences: {
            sandbox: true,
            preload,
            contextIsolation: false
          }
        });

        w.webContents.setWindowOpenHandler(() => ({
          action: 'allow',
          overrideBrowserWindowOptions: {
            webPreferences: {
              preload
            }
          }
        }));

        w.loadFile(
          path.join(__dirname, 'fixtures', 'api', 'sandbox.html'),
          { search: 'window-open-external' }
        );

        // Wait for a message from the main window saying that it's ready.
        await emittedOnce(ipcMain, 'opener-loaded');

        // Ask the opener to open a popup with window.opener.
        const expectedPopupUrl = `${serverUrl}/cross-site`; // Set in "sandbox.html".

        w.webContents.send('open-the-popup', expectedPopupUrl);

        // The page is going to open a popup that it won't be able to close.
        // We have to close it from here later.
        const [, popupWindow] = await emittedOnce(app, 'browser-window-created');

        // Ask the popup window for details.
        const detailsAnswer = emittedOnce(ipcMain, 'child-loaded');
        popupWindow.webContents.send('provide-details');
        const [, openerIsNull, , locationHref] = await detailsAnswer;
        expect(openerIsNull).to.be.false('window.opener is null');
        expect(locationHref).to.equal(expectedPopupUrl);

        // Ask the page to access the popup.
        const touchPopupResult = emittedOnce(ipcMain, 'answer');
        w.webContents.send('touch-the-popup');
        const [, popupAccessMessage] = await touchPopupResult;

        // Ask the popup to access the opener.
        const touchOpenerResult = emittedOnce(ipcMain, 'answer');
        popupWindow.webContents.send('touch-the-opener');
        const [, openerAccessMessage] = await touchOpenerResult;

        // We don't need the popup anymore, and its parent page can't close it,
        // so let's close it from here before we run any checks.
        await closeWindow(popupWindow, { assertNotWindows: false });

        expect(popupAccessMessage).to.be.a('string',
          'child\'s .document is accessible from its parent window');
        expect(popupAccessMessage).to.match(/^Blocked a frame with origin/);
        expect(openerAccessMessage).to.be.a('string',
          'opener .document is accessible from a popup window');
        expect(openerAccessMessage).to.match(/^Blocked a frame with origin/);
      });

      it('should inherit the sandbox setting in opened windows', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true
          }
        });

        const preloadPath = path.join(fixtures, 'api', 'new-window-preload.js');
        w.webContents.setWindowOpenHandler(() => ({ action: 'allow', overrideBrowserWindowOptions: { webPreferences: { preload: preloadPath } } }));
        w.loadFile(path.join(fixtures, 'api', 'new-window.html'));
        const [, { argv }] = await emittedOnce(ipcMain, 'answer');
        expect(argv).to.include('--enable-sandbox');
      });

      it('should open windows with the options configured via new-window event listeners', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true
          }
        });

        const preloadPath = path.join(fixtures, 'api', 'new-window-preload.js');
        w.webContents.setWindowOpenHandler(() => ({ action: 'allow', overrideBrowserWindowOptions: { webPreferences: { preload: preloadPath, foo: 'bar' } } }));
        w.loadFile(path.join(fixtures, 'api', 'new-window.html'));
        const [[, childWebContents]] = await Promise.all([
          emittedOnce(app, 'web-contents-created'),
          emittedOnce(ipcMain, 'answer')
        ]);
        const webPreferences = childWebContents.getLastWebPreferences();
        expect(webPreferences.foo).to.equal('bar');
      });

      it('should set ipc event sender correctly', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload,
            contextIsolation: false
          }
        });
        let childWc: WebContents | null = null;
        w.webContents.setWindowOpenHandler(() => ({ action: 'allow', overrideBrowserWindowOptions: { webPreferences: { preload, contextIsolation: false } } }));

        w.webContents.on('did-create-window', (win) => {
          childWc = win.webContents;
          expect(w.webContents).to.not.equal(childWc);
        });

        ipcMain.once('parent-ready', function (event) {
          expect(event.sender).to.equal(w.webContents, 'sender should be the parent');
          event.sender.send('verified');
        });
        ipcMain.once('child-ready', function (event) {
          expect(childWc).to.not.be.null('child webcontents should be available');
          expect(event.sender).to.equal(childWc, 'sender should be the child');
          event.sender.send('verified');
        });

        const done = Promise.all([
          'parent-answer',
          'child-answer'
        ].map(name => emittedOnce(ipcMain, name)));
        w.loadFile(path.join(__dirname, 'fixtures', 'api', 'sandbox.html'), { search: 'verify-ipc-sender' });
        await done;
      });

      describe('event handling', () => {
        let w: BrowserWindow = null as unknown as BrowserWindow;
        beforeEach(() => {
          w = new BrowserWindow({ show: false, webPreferences: { sandbox: true } });
        });
        it('works for window events', async () => {
          const pageTitleUpdated = emittedOnce(w, 'page-title-updated');
          w.loadURL('data:text/html,<script>document.title = \'changed\'</script>');
          await pageTitleUpdated;
        });

        it('works for stop events', async () => {
          const done = Promise.all([
            'did-navigate',
            'did-fail-load',
            'did-stop-loading'
          ].map(name => emittedOnce(w.webContents, name)));
          w.loadURL('data:text/html,<script>stop()</script>');
          await done;
        });

        it('works for web contents events', async () => {
          const done = Promise.all([
            'did-finish-load',
            'did-frame-finish-load',
            'did-navigate-in-page',
            'will-navigate',
            'did-start-loading',
            'did-stop-loading',
            'did-frame-finish-load',
            'dom-ready'
          ].map(name => emittedOnce(w.webContents, name)));
          w.loadFile(path.join(__dirname, 'fixtures', 'api', 'sandbox.html'), { search: 'webcontents-events' });
          await done;
        });
      });

      it('supports calling preventDefault on new-window events', (done) => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true
          }
        });
        const initialWebContents = webContents.getAllWebContents().map((i) => i.id);
        w.webContents.once('new-window', (e) => {
          e.preventDefault();
          // We need to give it some time so the windows get properly disposed (at least on OSX).
          setTimeout(() => {
            const currentWebContents = webContents.getAllWebContents().map((i) => i.id);
            try {
              expect(currentWebContents).to.deep.equal(initialWebContents);
              done();
            } catch (error) {
              done(e);
            }
          }, 100);
        });
        w.loadFile(path.join(fixtures, 'pages', 'window-open.html'));
      });

      it('validates process APIs access in sandboxed renderer', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload,
            contextIsolation: false
          }
        });
        w.webContents.once('preload-error', (event, preloadPath, error) => {
          throw error;
        });
        process.env.sandboxmain = 'foo';
        w.loadFile(path.join(fixtures, 'api', 'preload.html'));
        const [, test] = await emittedOnce(ipcMain, 'answer');
        expect(test.hasCrash).to.be.true('has crash');
        expect(test.hasHang).to.be.true('has hang');
        expect(test.heapStatistics).to.be.an('object');
        expect(test.blinkMemoryInfo).to.be.an('object');
        expect(test.processMemoryInfo).to.be.an('object');
        expect(test.systemVersion).to.be.a('string');
        expect(test.cpuUsage).to.be.an('object');
        expect(test.ioCounters).to.be.an('object');
        expect(test.uptime).to.be.a('number');
        expect(test.arch).to.equal(process.arch);
        expect(test.platform).to.equal(process.platform);
        expect(test.env).to.deep.equal(process.env);
        expect(test.execPath).to.equal(process.helperExecPath);
        expect(test.sandboxed).to.be.true('sandboxed');
        expect(test.contextIsolated).to.be.false('contextIsolated');
        expect(test.type).to.equal('renderer');
        expect(test.version).to.equal(process.version);
        expect(test.versions).to.deep.equal(process.versions);
        expect(test.contextId).to.be.a('string');

        if (process.platform === 'linux' && test.osSandbox) {
          expect(test.creationTime).to.be.null('creation time');
          expect(test.systemMemoryInfo).to.be.null('system memory info');
        } else {
          expect(test.creationTime).to.be.a('number');
          expect(test.systemMemoryInfo).to.be.an('object');
        }
      });

      it('webview in sandbox renderer', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            preload,
            webviewTag: true,
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

    describe('nativeWindowOpen option', () => {
      let w: BrowserWindow = null as unknown as BrowserWindow;

      beforeEach(() => {
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            nativeWindowOpen: true,
            // tests relies on preloads in opened windows
            nodeIntegrationInSubFrames: true,
            contextIsolation: false
          }
        });
      });

      it('opens window of about:blank with cross-scripting enabled', async () => {
        const answer = emittedOnce(ipcMain, 'answer');
        w.loadFile(path.join(fixtures, 'api', 'native-window-open-blank.html'));
        const [, content] = await answer;
        expect(content).to.equal('Hello');
      });
      it('opens window of same domain with cross-scripting enabled', async () => {
        const answer = emittedOnce(ipcMain, 'answer');
        w.loadFile(path.join(fixtures, 'api', 'native-window-open-file.html'));
        const [, content] = await answer;
        expect(content).to.equal('Hello');
      });
      it('blocks accessing cross-origin frames', async () => {
        const answer = emittedOnce(ipcMain, 'answer');
        w.loadFile(path.join(fixtures, 'api', 'native-window-open-cross-origin.html'));
        const [, content] = await answer;
        expect(content).to.equal('Blocked a frame with origin "file://" from accessing a cross-origin frame.');
      });
      it('opens window from <iframe> tags', async () => {
        const answer = emittedOnce(ipcMain, 'answer');
        w.loadFile(path.join(fixtures, 'api', 'native-window-open-iframe.html'));
        const [, content] = await answer;
        expect(content).to.equal('Hello');
      });
      ifit(!process.env.ELECTRON_SKIP_NATIVE_MODULE_TESTS)('loads native addons correctly after reload', async () => {
        w.loadFile(path.join(__dirname, 'fixtures', 'api', 'native-window-open-native-addon.html'));
        {
          const [, content] = await emittedOnce(ipcMain, 'answer');
          expect(content).to.equal('function');
        }
        w.reload();
        {
          const [, content] = await emittedOnce(ipcMain, 'answer');
          expect(content).to.equal('function');
        }
      });
      it('<webview> works in a scriptable popup', async () => {
        const preload = path.join(fixtures, 'api', 'new-window-webview-preload.js');

        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegrationInSubFrames: true,
            nativeWindowOpen: true,
            webviewTag: true,
            contextIsolation: false,
            preload
          }
        });
        w.webContents.setWindowOpenHandler(() => ({
          action: 'allow',
          overrideBrowserWindowOptions: { show: false, webPreferences: { contextIsolation: false, webviewTag: true, nativeWindowOpen: true, nodeIntegrationInSubFrames: true } }
        }));
        w.webContents.once('new-window', (event, url, frameName, disposition, options) => {
          options.show = false;
        });

        const webviewLoaded = emittedOnce(ipcMain, 'webview-loaded');
        w.loadFile(path.join(fixtures, 'api', 'new-window-webview.html'));
        await webviewLoaded;
      });
      it('should inherit the nativeWindowOpen setting in opened windows', async () => {
        const preloadPath = path.join(fixtures, 'api', 'new-window-preload.js');

        w.webContents.setWindowOpenHandler(() => ({
          action: 'allow',
          overrideBrowserWindowOptions: {
            webPreferences: {
              preload: preloadPath
            }
          }
        }));
        w.loadFile(path.join(fixtures, 'api', 'new-window.html'));
        const [, { nativeWindowOpen }] = await emittedOnce(ipcMain, 'answer');
        expect(nativeWindowOpen).to.be.true();
      });
      it('should open windows with the options configured via new-window event listeners', async () => {
        const preloadPath = path.join(fixtures, 'api', 'new-window-preload.js');
        w.webContents.setWindowOpenHandler(() => ({
          action: 'allow',
          overrideBrowserWindowOptions: {
            webPreferences: {
              preload: preloadPath,
              foo: 'bar'
            }
          }
        }));
        w.loadFile(path.join(fixtures, 'api', 'new-window.html'));
        const [[, childWebContents]] = await Promise.all([
          emittedOnce(app, 'web-contents-created'),
          emittedOnce(ipcMain, 'answer')
        ]);
        const webPreferences = childWebContents.getLastWebPreferences();
        expect(webPreferences.foo).to.equal('bar');
      });

      describe('window.location', () => {
        const protocols = [
          ['foo', path.join(fixtures, 'api', 'window-open-location-change.html')],
          ['bar', path.join(fixtures, 'api', 'window-open-location-final.html')]
        ];
        beforeEach(() => {
          for (const [scheme, path] of protocols) {
            protocol.registerBufferProtocol(scheme, (request, callback) => {
              callback({
                mimeType: 'text/html',
                data: fs.readFileSync(path)
              });
            });
          }
        });
        afterEach(() => {
          for (const [scheme] of protocols) {
            protocol.unregisterProtocol(scheme);
          }
        });
        it('retains the original web preferences when window.location is changed to a new origin', async () => {
          const w = new BrowserWindow({
            show: false,
            webPreferences: {
              nativeWindowOpen: true,
              // test relies on preloads in opened window
              nodeIntegrationInSubFrames: true,
              contextIsolation: false
            }
          });

          w.webContents.setWindowOpenHandler(() => ({
            action: 'allow',
            overrideBrowserWindowOptions: {
              webPreferences: {
                preload: path.join(fixtures, 'api', 'window-open-preload.js'),
                contextIsolation: false,
                nodeIntegrationInSubFrames: true
              }
            }
          }));

          w.loadFile(path.join(fixtures, 'api', 'window-open-location-open.html'));
          const [, { nodeIntegration, nativeWindowOpen, typeofProcess }] = await emittedOnce(ipcMain, 'answer');
          expect(nodeIntegration).to.be.false();
          expect(nativeWindowOpen).to.be.true();
          expect(typeofProcess).to.eql('undefined');
        });

        it('window.opener is not null when window.location is changed to a new origin', async () => {
          const w = new BrowserWindow({
            show: false,
            webPreferences: {
              nativeWindowOpen: true,
              // test relies on preloads in opened window
              nodeIntegrationInSubFrames: true
            }
          });

          w.webContents.setWindowOpenHandler(() => ({
            action: 'allow',
            overrideBrowserWindowOptions: {
              webPreferences: {
                preload: path.join(fixtures, 'api', 'window-open-preload.js')
              }
            }
          }));
          w.loadFile(path.join(fixtures, 'api', 'window-open-location-open.html'));
          const [, { windowOpenerIsNull }] = await emittedOnce(ipcMain, 'answer');
          expect(windowOpenerIsNull).to.be.false('window.opener is null');
        });
      });
    });

    describe('"disableHtmlFullscreenWindowResize" option', () => {
      it('prevents window from resizing when set', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            disableHtmlFullscreenWindowResize: true
          }
        });
        await w.loadURL('about:blank');
        const size = w.getSize();
        const enterHtmlFullScreen = emittedOnce(w.webContents, 'enter-html-full-screen');
        w.webContents.executeJavaScript('document.body.webkitRequestFullscreen()', true);
        await enterHtmlFullScreen;
        expect(w.getSize()).to.deep.equal(size);
      });
    });
  });

  describe('nativeWindowOpen + contextIsolation options', () => {
    afterEach(closeAllWindows);
    it('opens window with cross-scripting enabled from isolated context', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nativeWindowOpen: true,
          contextIsolation: true,
          preload: path.join(fixtures, 'api', 'native-window-open-isolated-preload.js')
        }
      });
      w.loadFile(path.join(fixtures, 'api', 'native-window-open-isolated.html'));
      const [, content] = await emittedOnce(ipcMain, 'answer');
      expect(content).to.equal('Hello');
    });
  });

  describe('beforeunload handler', function () {
    let w: BrowserWindow = null as unknown as BrowserWindow;
    beforeEach(() => {
      w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } });
    });
    afterEach(closeAllWindows);

    it('returning undefined would not prevent close', async () => {
      await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'beforeunload-undefined.html'));
      const wait = emittedOnce(w, 'closed');
      w.close();
      await wait;
    });

    it('returning false would prevent close', async () => {
      await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'beforeunload-false.html'));
      w.close();
      const [, proceed] = await emittedOnce(w.webContents, 'before-unload-fired');
      expect(proceed).to.equal(false);
    });

    it('returning empty string would prevent close', async () => {
      await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'beforeunload-empty-string.html'));
      w.close();
      const [, proceed] = await emittedOnce(w.webContents, 'before-unload-fired');
      expect(proceed).to.equal(false);
    });

    it('emits for each close attempt', async () => {
      await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'beforeunload-false-prevent3.html'));

      const destroyListener = () => { expect.fail('Close was not prevented'); };
      w.webContents.once('destroyed', destroyListener);

      w.webContents.executeJavaScript('installBeforeUnload(2)', true);
      // The renderer needs to report the status of beforeunload handler
      // back to main process, so wait for next console message, which means
      // the SuddenTerminationStatus message have been flushed.
      await emittedOnce(w.webContents, 'console-message');
      w.close();
      await emittedOnce(w.webContents, 'before-unload-fired');
      w.close();
      await emittedOnce(w.webContents, 'before-unload-fired');

      w.webContents.removeListener('destroyed', destroyListener);
      const wait = emittedOnce(w, 'closed');
      w.close();
      await wait;
    });

    it('emits for each reload attempt', async () => {
      await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'beforeunload-false-prevent3.html'));

      const navigationListener = () => { expect.fail('Reload was not prevented'); };
      w.webContents.once('did-start-navigation', navigationListener);

      w.webContents.executeJavaScript('installBeforeUnload(2)', true);
      // The renderer needs to report the status of beforeunload handler
      // back to main process, so wait for next console message, which means
      // the SuddenTerminationStatus message have been flushed.
      await emittedOnce(w.webContents, 'console-message');
      w.reload();
      // Chromium does not emit 'before-unload-fired' on WebContents for
      // navigations, so we have to use other ways to know if beforeunload
      // is fired.
      await emittedUntil(w.webContents, 'console-message', isBeforeUnload);
      w.reload();
      await emittedUntil(w.webContents, 'console-message', isBeforeUnload);

      w.webContents.removeListener('did-start-navigation', navigationListener);
      w.reload();
      await emittedOnce(w.webContents, 'did-finish-load');
    });

    it('emits for each navigation attempt', async () => {
      await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'beforeunload-false-prevent3.html'));

      const navigationListener = () => { expect.fail('Reload was not prevented'); };
      w.webContents.once('did-start-navigation', navigationListener);

      w.webContents.executeJavaScript('installBeforeUnload(2)', true);
      // The renderer needs to report the status of beforeunload handler
      // back to main process, so wait for next console message, which means
      // the SuddenTerminationStatus message have been flushed.
      await emittedOnce(w.webContents, 'console-message');
      w.loadURL('about:blank');
      // Chromium does not emit 'before-unload-fired' on WebContents for
      // navigations, so we have to use other ways to know if beforeunload
      // is fired.
      await emittedUntil(w.webContents, 'console-message', isBeforeUnload);
      w.loadURL('about:blank');
      await emittedUntil(w.webContents, 'console-message', isBeforeUnload);

      w.webContents.removeListener('did-start-navigation', navigationListener);
      await w.loadURL('about:blank');
    });
  });

  describe('document.visibilityState/hidden', () => {
    afterEach(closeAllWindows);

    it('visibilityState is initially visible despite window being hidden', async () => {
      const w = new BrowserWindow({
        show: false,
        width: 100,
        height: 100,
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false
        }
      });

      let readyToShow = false;
      w.once('ready-to-show', () => {
        readyToShow = true;
      });

      w.loadFile(path.join(fixtures, 'pages', 'visibilitychange.html'));

      const [, visibilityState, hidden] = await emittedOnce(ipcMain, 'pong');

      expect(readyToShow).to.be.false('ready to show');
      expect(visibilityState).to.equal('visible');
      expect(hidden).to.be.false('hidden');
    });

    // TODO(nornagon): figure out why this is failing on windows
    ifit(process.platform !== 'win32')('visibilityState changes when window is hidden', async () => {
      const w = new BrowserWindow({
        width: 100,
        height: 100,
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false
        }
      });

      w.loadFile(path.join(fixtures, 'pages', 'visibilitychange.html'));

      {
        const [, visibilityState, hidden] = await emittedOnce(ipcMain, 'pong');
        expect(visibilityState).to.equal('visible');
        expect(hidden).to.be.false('hidden');
      }

      w.hide();

      {
        const [, visibilityState, hidden] = await emittedOnce(ipcMain, 'pong');
        expect(visibilityState).to.equal('hidden');
        expect(hidden).to.be.true('hidden');
      }
    });

    // TODO(nornagon): figure out why this is failing on windows
    ifit(process.platform !== 'win32')('visibilityState changes when window is shown', async () => {
      const w = new BrowserWindow({
        width: 100,
        height: 100,
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false
        }
      });

      w.loadFile(path.join(fixtures, 'pages', 'visibilitychange.html'));
      if (process.platform === 'darwin') {
        // See https://github.com/electron/electron/issues/8664
        await emittedOnce(w, 'show');
      }
      w.hide();
      w.show();
      const [, visibilityState] = await emittedOnce(ipcMain, 'pong');
      expect(visibilityState).to.equal('visible');
    });

    ifit(process.platform !== 'win32')('visibilityState changes when window is shown inactive', async () => {
      const w = new BrowserWindow({
        width: 100,
        height: 100,
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false
        }
      });
      w.loadFile(path.join(fixtures, 'pages', 'visibilitychange.html'));
      if (process.platform === 'darwin') {
        // See https://github.com/electron/electron/issues/8664
        await emittedOnce(w, 'show');
      }
      w.hide();
      w.showInactive();
      const [, visibilityState] = await emittedOnce(ipcMain, 'pong');
      expect(visibilityState).to.equal('visible');
    });

    // TODO(nornagon): figure out why this is failing on windows
    ifit(process.platform === 'darwin')('visibilityState changes when window is minimized', async () => {
      const w = new BrowserWindow({
        width: 100,
        height: 100,
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false
        }
      });
      w.loadFile(path.join(fixtures, 'pages', 'visibilitychange.html'));

      {
        const [, visibilityState, hidden] = await emittedOnce(ipcMain, 'pong');
        expect(visibilityState).to.equal('visible');
        expect(hidden).to.be.false('hidden');
      }

      w.minimize();

      {
        const [, visibilityState, hidden] = await emittedOnce(ipcMain, 'pong');
        expect(visibilityState).to.equal('hidden');
        expect(hidden).to.be.true('hidden');
      }
    });

    // FIXME(MarshallOfSound): This test fails locally 100% of the time, on CI it started failing
    // when we introduced the compositor recycling patch.  Should figure out how to fix this
    it.skip('visibilityState remains visible if backgroundThrottling is disabled', async () => {
      const w = new BrowserWindow({
        show: false,
        width: 100,
        height: 100,
        webPreferences: {
          backgroundThrottling: false,
          nodeIntegration: true
        }
      });
      w.loadFile(path.join(fixtures, 'pages', 'visibilitychange.html'));
      {
        const [, visibilityState, hidden] = await emittedOnce(ipcMain, 'pong');
        expect(visibilityState).to.equal('visible');
        expect(hidden).to.be.false('hidden');
      }

      ipcMain.once('pong', (event, visibilityState, hidden) => {
        throw new Error(`Unexpected visibility change event. visibilityState: ${visibilityState} hidden: ${hidden}`);
      });
      try {
        const shown1 = emittedOnce(w, 'show');
        w.show();
        await shown1;
        const hidden = emittedOnce(w, 'hide');
        w.hide();
        await hidden;
        const shown2 = emittedOnce(w, 'show');
        w.show();
        await shown2;
      } finally {
        ipcMain.removeAllListeners('pong');
      }
    });
  });

  describe('new-window event', () => {
    afterEach(closeAllWindows);

    it('emits when window.open is called', (done) => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } });
      w.webContents.once('new-window', (e, url, frameName, disposition, options) => {
        e.preventDefault();
        try {
          expect(url).to.equal('http://host/');
          expect(frameName).to.equal('host');
          expect((options as any)['this-is-not-a-standard-feature']).to.equal(true);
          done();
        } catch (e) {
          done(e);
        }
      });
      w.loadFile(path.join(fixtures, 'pages', 'window-open.html'));
    });

    it('emits when window.open is called with no webPreferences', (done) => {
      const w = new BrowserWindow({ show: false });
      w.webContents.once('new-window', function (e, url, frameName, disposition, options) {
        e.preventDefault();
        try {
          expect(url).to.equal('http://host/');
          expect(frameName).to.equal('host');
          expect((options as any)['this-is-not-a-standard-feature']).to.equal(true);
          done();
        } catch (e) {
          done(e);
        }
      });
      w.loadFile(path.join(fixtures, 'pages', 'window-open.html'));
    });

    it('emits when link with target is called', (done) => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } });
      w.webContents.once('new-window', (e, url, frameName) => {
        e.preventDefault();
        try {
          expect(url).to.equal('http://host/');
          expect(frameName).to.equal('target');
          done();
        } catch (e) {
          done(e);
        }
      });
      w.loadFile(path.join(fixtures, 'pages', 'target-name.html'));
    });

    it('includes all properties', async () => {
      const w = new BrowserWindow({ show: false });

      const p = new Promise<{
        url: string,
        frameName: string,
        disposition: string,
        options: BrowserWindowConstructorOptions,
        additionalFeatures: string[],
        referrer: Electron.Referrer,
        postBody: Electron.PostBody
      }>((resolve) => {
        w.webContents.once('new-window', (e, url, frameName, disposition, options, additionalFeatures, referrer, postBody) => {
          e.preventDefault();
          resolve({ url, frameName, disposition, options, additionalFeatures, referrer, postBody });
        });
      });
      w.loadURL(`data:text/html,${encodeURIComponent(`
        <form target="_blank" method="POST" id="form" action="http://example.com/test">
          <input type="text" name="post-test-key" value="post-test-value"></input>
        </form>
        <script>form.submit()</script>
      `)}`);
      const { url, frameName, disposition, options, additionalFeatures, referrer, postBody } = await p;
      expect(url).to.equal('http://example.com/test');
      expect(frameName).to.equal('');
      expect(disposition).to.equal('foreground-tab');
      expect(options).to.be.an('object').not.null();
      expect(referrer.policy).to.equal('strict-origin-when-cross-origin');
      expect(referrer.url).to.equal('');
      expect(additionalFeatures).to.deep.equal([]);
      expect(postBody.data).to.have.length(1);
      expect(postBody.data[0].type).to.equal('rawData');
      expect((postBody.data[0] as any).bytes).to.deep.equal(Buffer.from('post-test-key=post-test-value'));
      expect(postBody.contentType).to.equal('application/x-www-form-urlencoded');
    });
  });

  ifdescribe(process.platform !== 'linux')('max/minimize events', () => {
    afterEach(closeAllWindows);
    it('emits an event when window is maximized', async () => {
      const w = new BrowserWindow({ show: false });
      const maximize = emittedOnce(w, 'maximize');
      w.show();
      w.maximize();
      await maximize;
    });

    it('emits an event when a transparent window is maximized', async () => {
      const w = new BrowserWindow({
        show: false,
        frame: false,
        transparent: true
      });

      const maximize = emittedOnce(w, 'maximize');
      w.show();
      w.maximize();
      await maximize;
    });

    it('emits only one event when frameless window is maximized', () => {
      const w = new BrowserWindow({ show: false, frame: false });
      let emitted = 0;
      w.on('maximize', () => emitted++);
      w.show();
      w.maximize();
      expect(emitted).to.equal(1);
    });

    it('emits an event when window is unmaximized', async () => {
      const w = new BrowserWindow({ show: false });
      const unmaximize = emittedOnce(w, 'unmaximize');
      w.show();
      w.maximize();
      w.unmaximize();
      await unmaximize;
    });

    it('emits an event when a transparent window is unmaximized', async () => {
      const w = new BrowserWindow({
        show: false,
        frame: false,
        transparent: true
      });

      const maximize = emittedOnce(w, 'maximize');
      const unmaximize = emittedOnce(w, 'unmaximize');
      w.show();
      w.maximize();
      await maximize;
      w.unmaximize();
      await unmaximize;
    });

    it('emits an event when window is minimized', async () => {
      const w = new BrowserWindow({ show: false });
      const minimize = emittedOnce(w, 'minimize');
      w.show();
      w.minimize();
      await minimize;
    });
  });

  describe('beginFrameSubscription method', () => {
    it('does not crash when callback returns nothing', (done) => {
      const w = new BrowserWindow({ show: false });
      w.loadFile(path.join(fixtures, 'api', 'frame-subscriber.html'));
      w.webContents.on('dom-ready', () => {
        w.webContents.beginFrameSubscription(function () {
          // Pending endFrameSubscription to next tick can reliably reproduce
          // a crash which happens when nothing is returned in the callback.
          setTimeout(() => {
            w.webContents.endFrameSubscription();
            done();
          });
        });
      });
    });

    it('subscribes to frame updates', (done) => {
      const w = new BrowserWindow({ show: false });
      let called = false;
      w.loadFile(path.join(fixtures, 'api', 'frame-subscriber.html'));
      w.webContents.on('dom-ready', () => {
        w.webContents.beginFrameSubscription(function (data) {
          // This callback might be called twice.
          if (called) return;
          called = true;

          try {
            expect(data.constructor.name).to.equal('NativeImage');
            expect(data.isEmpty()).to.be.false('data is empty');
            done();
          } catch (e) {
            done(e);
          } finally {
            w.webContents.endFrameSubscription();
          }
        });
      });
    });

    it('subscribes to frame updates (only dirty rectangle)', (done) => {
      const w = new BrowserWindow({ show: false });
      let called = false;
      let gotInitialFullSizeFrame = false;
      const [contentWidth, contentHeight] = w.getContentSize();
      w.webContents.on('did-finish-load', () => {
        w.webContents.beginFrameSubscription(true, (image, rect) => {
          if (image.isEmpty()) {
            // Chromium sometimes sends a 0x0 frame at the beginning of the
            // page load.
            return;
          }
          if (rect.height === contentHeight && rect.width === contentWidth &&
            !gotInitialFullSizeFrame) {
            // The initial frame is full-size, but we're looking for a call
            // with just the dirty-rect. The next frame should be a smaller
            // rect.
            gotInitialFullSizeFrame = true;
            return;
          }
          // This callback might be called twice.
          if (called) return;
          // We asked for just the dirty rectangle, so we expect to receive a
          // rect smaller than the full size.
          // TODO(jeremy): this is failing on windows currently; investigate.
          // assert(rect.width < contentWidth || rect.height < contentHeight)
          called = true;

          try {
            const expectedSize = rect.width * rect.height * 4;
            expect(image.getBitmap()).to.be.an.instanceOf(Buffer).with.lengthOf(expectedSize);
            done();
          } catch (e) {
            done(e);
          } finally {
            w.webContents.endFrameSubscription();
          }
        });
      });
      w.loadFile(path.join(fixtures, 'api', 'frame-subscriber.html'));
    });

    it('throws error when subscriber is not well defined', () => {
      const w = new BrowserWindow({ show: false });
      expect(() => {
        w.webContents.beginFrameSubscription(true, true as any);
        // TODO(zcbenz): gin is weak at guessing parameter types, we should
        // upstream native_mate's implementation to gin.
      }).to.throw('Error processing argument at index 1, conversion failure from ');
    });
  });

  describe('savePage method', () => {
    const savePageDir = path.join(fixtures, 'save_page');
    const savePageHtmlPath = path.join(savePageDir, 'save_page.html');
    const savePageJsPath = path.join(savePageDir, 'save_page_files', 'test.js');
    const savePageCssPath = path.join(savePageDir, 'save_page_files', 'test.css');

    afterEach(() => {
      closeAllWindows();

      try {
        fs.unlinkSync(savePageCssPath);
        fs.unlinkSync(savePageJsPath);
        fs.unlinkSync(savePageHtmlPath);
        fs.rmdirSync(path.join(savePageDir, 'save_page_files'));
        fs.rmdirSync(savePageDir);
      } catch {}
    });

    it('should throw when passing relative paths', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixtures, 'pages', 'save_page', 'index.html'));

      await expect(
        w.webContents.savePage('save_page.html', 'HTMLComplete')
      ).to.eventually.be.rejectedWith('Path must be absolute');

      await expect(
        w.webContents.savePage('save_page.html', 'HTMLOnly')
      ).to.eventually.be.rejectedWith('Path must be absolute');

      await expect(
        w.webContents.savePage('save_page.html', 'MHTML')
      ).to.eventually.be.rejectedWith('Path must be absolute');
    });

    it('should save page to disk with HTMLOnly', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixtures, 'pages', 'save_page', 'index.html'));
      await w.webContents.savePage(savePageHtmlPath, 'HTMLOnly');

      expect(fs.existsSync(savePageHtmlPath)).to.be.true('html path');
      expect(fs.existsSync(savePageJsPath)).to.be.false('js path');
      expect(fs.existsSync(savePageCssPath)).to.be.false('css path');
    });

    it('should save page to disk with MHTML', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixtures, 'pages', 'save_page', 'index.html'));
      await w.webContents.savePage(savePageHtmlPath, 'MHTML');

      expect(fs.existsSync(savePageHtmlPath)).to.be.true('html path');
      expect(fs.existsSync(savePageJsPath)).to.be.false('js path');
      expect(fs.existsSync(savePageCssPath)).to.be.false('css path');
    });

    it('should save page to disk with HTMLComplete', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixtures, 'pages', 'save_page', 'index.html'));
      await w.webContents.savePage(savePageHtmlPath, 'HTMLComplete');

      expect(fs.existsSync(savePageHtmlPath)).to.be.true('html path');
      expect(fs.existsSync(savePageJsPath)).to.be.true('js path');
      expect(fs.existsSync(savePageCssPath)).to.be.true('css path');
    });
  });

  describe('BrowserWindow options argument is optional', () => {
    afterEach(closeAllWindows);
    it('should create a window with default size (800x600)', () => {
      const w = new BrowserWindow();
      expect(w.getSize()).to.deep.equal([800, 600]);
    });
  });

  describe('BrowserWindow.restore()', () => {
    afterEach(closeAllWindows);
    it('should restore the previous window size', () => {
      const w = new BrowserWindow({
        minWidth: 800,
        width: 800
      });

      const initialSize = w.getSize();
      w.minimize();
      w.restore();
      expectBoundsEqual(w.getSize(), initialSize);
    });

    it('does not crash when restoring hidden minimized window', () => {
      const w = new BrowserWindow({});
      w.minimize();
      w.hide();
      w.show();
    });
  });

  describe('BrowserWindow.unmaximize()', () => {
    afterEach(closeAllWindows);
    it('should restore the previous window position', () => {
      const w = new BrowserWindow();

      const initialPosition = w.getPosition();
      w.maximize();
      w.unmaximize();
      expectBoundsEqual(w.getPosition(), initialPosition);
    });

    // TODO(dsanders11): Enable once minimize event works on Linux again.
    //                   See https://github.com/electron/electron/issues/28699
    ifit(process.platform !== 'linux')('should not restore a minimized window', async () => {
      const w = new BrowserWindow();
      const minimize = emittedOnce(w, 'minimize');
      w.minimize();
      await minimize;
      w.unmaximize();
      await delay(1000);
      expect(w.isMinimized()).to.be.true();
    });

    it('should not change the size or position of a normal window', async () => {
      const w = new BrowserWindow();

      const initialSize = w.getSize();
      const initialPosition = w.getPosition();
      w.unmaximize();
      await delay(1000);
      expectBoundsEqual(w.getSize(), initialSize);
      expectBoundsEqual(w.getPosition(), initialPosition);
    });
  });

  describe('setFullScreen(false)', () => {
    afterEach(closeAllWindows);

    // only applicable to windows: https://github.com/electron/electron/issues/6036
    ifdescribe(process.platform === 'win32')('on windows', () => {
      it('should restore a normal visible window from a fullscreen startup state', async () => {
        const w = new BrowserWindow({ show: false });
        await w.loadURL('about:blank');
        const shown = emittedOnce(w, 'show');
        // start fullscreen and hidden
        w.setFullScreen(true);
        w.show();
        await shown;
        const leftFullScreen = emittedOnce(w, 'leave-full-screen');
        w.setFullScreen(false);
        await leftFullScreen;
        expect(w.isVisible()).to.be.true('visible');
        expect(w.isFullScreen()).to.be.false('fullscreen');
      });
      it('should keep window hidden if already in hidden state', async () => {
        const w = new BrowserWindow({ show: false });
        await w.loadURL('about:blank');
        const leftFullScreen = emittedOnce(w, 'leave-full-screen');
        w.setFullScreen(false);
        await leftFullScreen;
        expect(w.isVisible()).to.be.false('visible');
        expect(w.isFullScreen()).to.be.false('fullscreen');
      });
    });

    ifdescribe(process.platform === 'darwin')('BrowserWindow.setFullScreen(false) when HTML fullscreen', () => {
      it('exits HTML fullscreen when window leaves fullscreen', async () => {
        const w = new BrowserWindow();
        await w.loadURL('about:blank');
        await w.webContents.executeJavaScript('document.body.webkitRequestFullscreen()', true);
        await emittedOnce(w, 'enter-full-screen');
        // Wait a tick for the full-screen state to 'stick'
        await delay();
        w.setFullScreen(false);
        await emittedOnce(w, 'leave-html-full-screen');
      });
    });
  });

  describe('parent window', () => {
    afterEach(closeAllWindows);

    ifit(process.platform === 'darwin')('sheet-begin event emits when window opens a sheet', async () => {
      const w = new BrowserWindow();
      const sheetBegin = emittedOnce(w, 'sheet-begin');
      // eslint-disable-next-line no-new
      new BrowserWindow({
        modal: true,
        parent: w
      });
      await sheetBegin;
    });

    ifit(process.platform === 'darwin')('sheet-end event emits when window has closed a sheet', async () => {
      const w = new BrowserWindow();
      const sheet = new BrowserWindow({
        modal: true,
        parent: w
      });
      const sheetEnd = emittedOnce(w, 'sheet-end');
      sheet.close();
      await sheetEnd;
    });

    describe('parent option', () => {
      it('sets parent window', () => {
        const w = new BrowserWindow({ show: false });
        const c = new BrowserWindow({ show: false, parent: w });
        expect(c.getParentWindow()).to.equal(w);
      });
      it('adds window to child windows of parent', () => {
        const w = new BrowserWindow({ show: false });
        const c = new BrowserWindow({ show: false, parent: w });
        expect(w.getChildWindows()).to.deep.equal([c]);
      });
      it('removes from child windows of parent when window is closed', async () => {
        const w = new BrowserWindow({ show: false });
        const c = new BrowserWindow({ show: false, parent: w });
        const closed = emittedOnce(c, 'closed');
        c.close();
        await closed;
        // The child window list is not immediately cleared, so wait a tick until it's ready.
        await delay();
        expect(w.getChildWindows().length).to.equal(0);
      });

      it('should not affect the show option', () => {
        const w = new BrowserWindow({ show: false });
        const c = new BrowserWindow({ show: false, parent: w });
        expect(c.isVisible()).to.be.false('child is visible');
        expect(c.getParentWindow()!.isVisible()).to.be.false('parent is visible');
      });
    });

    describe('win.setParentWindow(parent)', () => {
      it('sets parent window', () => {
        const w = new BrowserWindow({ show: false });
        const c = new BrowserWindow({ show: false });
        expect(w.getParentWindow()).to.be.null('w.parent');
        expect(c.getParentWindow()).to.be.null('c.parent');
        c.setParentWindow(w);
        expect(c.getParentWindow()).to.equal(w);
        c.setParentWindow(null);
        expect(c.getParentWindow()).to.be.null('c.parent');
      });
      it('adds window to child windows of parent', () => {
        const w = new BrowserWindow({ show: false });
        const c = new BrowserWindow({ show: false });
        expect(w.getChildWindows()).to.deep.equal([]);
        c.setParentWindow(w);
        expect(w.getChildWindows()).to.deep.equal([c]);
        c.setParentWindow(null);
        expect(w.getChildWindows()).to.deep.equal([]);
      });
      it('removes from child windows of parent when window is closed', async () => {
        const w = new BrowserWindow({ show: false });
        const c = new BrowserWindow({ show: false });
        const closed = emittedOnce(c, 'closed');
        c.setParentWindow(w);
        c.close();
        await closed;
        // The child window list is not immediately cleared, so wait a tick until it's ready.
        await delay();
        expect(w.getChildWindows().length).to.equal(0);
      });
    });

    describe('modal option', () => {
      it('does not freeze or crash', async () => {
        const parentWindow = new BrowserWindow();

        const createTwo = async () => {
          const two = new BrowserWindow({
            width: 300,
            height: 200,
            parent: parentWindow,
            modal: true,
            show: false
          });

          const twoShown = emittedOnce(two, 'show');
          two.show();
          await twoShown;
          setTimeout(() => two.close(), 500);

          await emittedOnce(two, 'closed');
        };

        const one = new BrowserWindow({
          width: 600,
          height: 400,
          parent: parentWindow,
          modal: true,
          show: false
        });

        const oneShown = emittedOnce(one, 'show');
        one.show();
        await oneShown;
        setTimeout(() => one.destroy(), 500);

        await emittedOnce(one, 'closed');
        await createTwo();
      });

      ifit(process.platform !== 'darwin')('disables parent window', () => {
        const w = new BrowserWindow({ show: false });
        const c = new BrowserWindow({ show: false, parent: w, modal: true });
        expect(w.isEnabled()).to.be.true('w.isEnabled');
        c.show();
        expect(w.isEnabled()).to.be.false('w.isEnabled');
      });

      ifit(process.platform !== 'darwin')('re-enables an enabled parent window when closed', async () => {
        const w = new BrowserWindow({ show: false });
        const c = new BrowserWindow({ show: false, parent: w, modal: true });
        const closed = emittedOnce(c, 'closed');
        c.show();
        c.close();
        await closed;
        expect(w.isEnabled()).to.be.true('w.isEnabled');
      });

      ifit(process.platform !== 'darwin')('does not re-enable a disabled parent window when closed', async () => {
        const w = new BrowserWindow({ show: false });
        const c = new BrowserWindow({ show: false, parent: w, modal: true });
        const closed = emittedOnce(c, 'closed');
        w.setEnabled(false);
        c.show();
        c.close();
        await closed;
        expect(w.isEnabled()).to.be.false('w.isEnabled');
      });

      ifit(process.platform !== 'darwin')('disables parent window recursively', () => {
        const w = new BrowserWindow({ show: false });
        const c = new BrowserWindow({ show: false, parent: w, modal: true });
        const c2 = new BrowserWindow({ show: false, parent: w, modal: true });
        c.show();
        expect(w.isEnabled()).to.be.false('w.isEnabled');
        c2.show();
        expect(w.isEnabled()).to.be.false('w.isEnabled');
        c.destroy();
        expect(w.isEnabled()).to.be.false('w.isEnabled');
        c2.destroy();
        expect(w.isEnabled()).to.be.true('w.isEnabled');
      });
    });
  });

  describe('window states', () => {
    afterEach(closeAllWindows);
    it('does not resize frameless windows when states change', () => {
      const w = new BrowserWindow({
        frame: false,
        width: 300,
        height: 200,
        show: false
      });

      w.minimizable = false;
      w.minimizable = true;
      expect(w.getSize()).to.deep.equal([300, 200]);

      w.resizable = false;
      w.resizable = true;
      expect(w.getSize()).to.deep.equal([300, 200]);

      w.maximizable = false;
      w.maximizable = true;
      expect(w.getSize()).to.deep.equal([300, 200]);

      w.fullScreenable = false;
      w.fullScreenable = true;
      expect(w.getSize()).to.deep.equal([300, 200]);

      w.closable = false;
      w.closable = true;
      expect(w.getSize()).to.deep.equal([300, 200]);
    });

    describe('resizable state', () => {
      it('with properties', () => {
        it('can be set with resizable constructor option', () => {
          const w = new BrowserWindow({ show: false, resizable: false });
          expect(w.resizable).to.be.false('resizable');

          if (process.platform === 'darwin') {
            expect(w.maximizable).to.to.true('maximizable');
          }
        });

        it('can be changed', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.resizable).to.be.true('resizable');
          w.resizable = false;
          expect(w.resizable).to.be.false('resizable');
          w.resizable = true;
          expect(w.resizable).to.be.true('resizable');
        });
      });

      it('with functions', () => {
        it('can be set with resizable constructor option', () => {
          const w = new BrowserWindow({ show: false, resizable: false });
          expect(w.isResizable()).to.be.false('resizable');

          if (process.platform === 'darwin') {
            expect(w.isMaximizable()).to.to.true('maximizable');
          }
        });

        it('can be changed', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.isResizable()).to.be.true('resizable');
          w.setResizable(false);
          expect(w.isResizable()).to.be.false('resizable');
          w.setResizable(true);
          expect(w.isResizable()).to.be.true('resizable');
        });
      });

      it('works for a frameless window', () => {
        const w = new BrowserWindow({ show: false, frame: false });
        expect(w.resizable).to.be.true('resizable');

        if (process.platform === 'win32') {
          const w = new BrowserWindow({ show: false, thickFrame: false });
          expect(w.resizable).to.be.false('resizable');
        }
      });

      // On Linux there is no "resizable" property of a window.
      ifit(process.platform !== 'linux')('does affect maximizability when disabled and enabled', () => {
        const w = new BrowserWindow({ show: false });
        expect(w.resizable).to.be.true('resizable');

        expect(w.maximizable).to.be.true('maximizable');
        w.resizable = false;
        expect(w.maximizable).to.be.false('not maximizable');
        w.resizable = true;
        expect(w.maximizable).to.be.true('maximizable');
      });

      ifit(process.platform === 'win32')('works for a window smaller than 64x64', () => {
        const w = new BrowserWindow({
          show: false,
          frame: false,
          resizable: false,
          transparent: true
        });
        w.setContentSize(60, 60);
        expectBoundsEqual(w.getContentSize(), [60, 60]);
        w.setContentSize(30, 30);
        expectBoundsEqual(w.getContentSize(), [30, 30]);
        w.setContentSize(10, 10);
        expectBoundsEqual(w.getContentSize(), [10, 10]);
      });
    });

    describe('loading main frame state', () => {
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

      it('is true when the main frame is loading', async () => {
        const w = new BrowserWindow({ show: false });

        const didStartLoading = emittedOnce(w.webContents, 'did-start-loading');
        w.webContents.loadURL(serverUrl);
        await didStartLoading;

        expect(w.webContents.isLoadingMainFrame()).to.be.true('isLoadingMainFrame');
      });

      it('is false when only a subframe is loading', async () => {
        const w = new BrowserWindow({ show: false });

        const didStopLoading = emittedOnce(w.webContents, 'did-stop-loading');
        w.webContents.loadURL(serverUrl);
        await didStopLoading;

        expect(w.webContents.isLoadingMainFrame()).to.be.false('isLoadingMainFrame');

        const didStartLoading = emittedOnce(w.webContents, 'did-start-loading');
        w.webContents.executeJavaScript(`
          var iframe = document.createElement('iframe')
          iframe.src = '${serverUrl}/page2'
          document.body.appendChild(iframe)
        `);
        await didStartLoading;

        expect(w.webContents.isLoadingMainFrame()).to.be.false('isLoadingMainFrame');
      });

      it('is true when navigating to pages from the same origin', async () => {
        const w = new BrowserWindow({ show: false });

        const didStopLoading = emittedOnce(w.webContents, 'did-stop-loading');
        w.webContents.loadURL(serverUrl);
        await didStopLoading;

        expect(w.webContents.isLoadingMainFrame()).to.be.false('isLoadingMainFrame');

        const didStartLoading = emittedOnce(w.webContents, 'did-start-loading');
        w.webContents.loadURL(`${serverUrl}/page2`);
        await didStartLoading;

        expect(w.webContents.isLoadingMainFrame()).to.be.true('isLoadingMainFrame');
      });
    });
  });

  ifdescribe(process.platform !== 'linux')('window states (excluding Linux)', () => {
    // Not implemented on Linux.
    afterEach(closeAllWindows);

    describe('movable state', () => {
      it('with properties', () => {
        it('can be set with movable constructor option', () => {
          const w = new BrowserWindow({ show: false, movable: false });
          expect(w.movable).to.be.false('movable');
        });

        it('can be changed', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.movable).to.be.true('movable');
          w.movable = false;
          expect(w.movable).to.be.false('movable');
          w.movable = true;
          expect(w.movable).to.be.true('movable');
        });
      });

      it('with functions', () => {
        it('can be set with movable constructor option', () => {
          const w = new BrowserWindow({ show: false, movable: false });
          expect(w.isMovable()).to.be.false('movable');
        });

        it('can be changed', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.isMovable()).to.be.true('movable');
          w.setMovable(false);
          expect(w.isMovable()).to.be.false('movable');
          w.setMovable(true);
          expect(w.isMovable()).to.be.true('movable');
        });
      });
    });

    describe('visibleOnAllWorkspaces state', () => {
      it('with properties', () => {
        it('can be changed', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.visibleOnAllWorkspaces).to.be.false();
          w.visibleOnAllWorkspaces = true;
          expect(w.visibleOnAllWorkspaces).to.be.true();
        });
      });

      it('with functions', () => {
        it('can be changed', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.isVisibleOnAllWorkspaces()).to.be.false();
          w.setVisibleOnAllWorkspaces(true);
          expect(w.isVisibleOnAllWorkspaces()).to.be.true();
        });
      });
    });

    ifdescribe(process.platform === 'darwin')('documentEdited state', () => {
      it('with properties', () => {
        it('can be changed', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.documentEdited).to.be.false();
          w.documentEdited = true;
          expect(w.documentEdited).to.be.true();
        });
      });

      it('with functions', () => {
        it('can be changed', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.isDocumentEdited()).to.be.false();
          w.setDocumentEdited(true);
          expect(w.isDocumentEdited()).to.be.true();
        });
      });
    });

    ifdescribe(process.platform === 'darwin')('representedFilename', () => {
      it('with properties', () => {
        it('can be changed', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.representedFilename).to.eql('');
          w.representedFilename = 'a name';
          expect(w.representedFilename).to.eql('a name');
        });
      });

      it('with functions', () => {
        it('can be changed', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.getRepresentedFilename()).to.eql('');
          w.setRepresentedFilename('a name');
          expect(w.getRepresentedFilename()).to.eql('a name');
        });
      });
    });

    describe('native window title', () => {
      it('with properties', () => {
        it('can be set with title constructor option', () => {
          const w = new BrowserWindow({ show: false, title: 'mYtItLe' });
          expect(w.title).to.eql('mYtItLe');
        });

        it('can be changed', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.title).to.eql('Electron Test Main');
          w.title = 'NEW TITLE';
          expect(w.title).to.eql('NEW TITLE');
        });
      });

      it('with functions', () => {
        it('can be set with minimizable constructor option', () => {
          const w = new BrowserWindow({ show: false, title: 'mYtItLe' });
          expect(w.getTitle()).to.eql('mYtItLe');
        });

        it('can be changed', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.getTitle()).to.eql('Electron Test Main');
          w.setTitle('NEW TITLE');
          expect(w.getTitle()).to.eql('NEW TITLE');
        });
      });
    });

    describe('minimizable state', () => {
      it('with properties', () => {
        it('can be set with minimizable constructor option', () => {
          const w = new BrowserWindow({ show: false, minimizable: false });
          expect(w.minimizable).to.be.false('minimizable');
        });

        it('can be changed', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.minimizable).to.be.true('minimizable');
          w.minimizable = false;
          expect(w.minimizable).to.be.false('minimizable');
          w.minimizable = true;
          expect(w.minimizable).to.be.true('minimizable');
        });
      });

      it('with functions', () => {
        it('can be set with minimizable constructor option', () => {
          const w = new BrowserWindow({ show: false, minimizable: false });
          expect(w.isMinimizable()).to.be.false('movable');
        });

        it('can be changed', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.isMinimizable()).to.be.true('isMinimizable');
          w.setMinimizable(false);
          expect(w.isMinimizable()).to.be.false('isMinimizable');
          w.setMinimizable(true);
          expect(w.isMinimizable()).to.be.true('isMinimizable');
        });
      });
    });

    describe('maximizable state (property)', () => {
      it('with properties', () => {
        it('can be set with maximizable constructor option', () => {
          const w = new BrowserWindow({ show: false, maximizable: false });
          expect(w.maximizable).to.be.false('maximizable');
        });

        it('can be changed', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.maximizable).to.be.true('maximizable');
          w.maximizable = false;
          expect(w.maximizable).to.be.false('maximizable');
          w.maximizable = true;
          expect(w.maximizable).to.be.true('maximizable');
        });

        it('is not affected when changing other states', () => {
          const w = new BrowserWindow({ show: false });
          w.maximizable = false;
          expect(w.maximizable).to.be.false('maximizable');
          w.minimizable = false;
          expect(w.maximizable).to.be.false('maximizable');
          w.closable = false;
          expect(w.maximizable).to.be.false('maximizable');

          w.maximizable = true;
          expect(w.maximizable).to.be.true('maximizable');
          w.closable = true;
          expect(w.maximizable).to.be.true('maximizable');
          w.fullScreenable = false;
          expect(w.maximizable).to.be.true('maximizable');
        });
      });

      it('with functions', () => {
        it('can be set with maximizable constructor option', () => {
          const w = new BrowserWindow({ show: false, maximizable: false });
          expect(w.isMaximizable()).to.be.false('isMaximizable');
        });

        it('can be changed', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.isMaximizable()).to.be.true('isMaximizable');
          w.setMaximizable(false);
          expect(w.isMaximizable()).to.be.false('isMaximizable');
          w.setMaximizable(true);
          expect(w.isMaximizable()).to.be.true('isMaximizable');
        });

        it('is not affected when changing other states', () => {
          const w = new BrowserWindow({ show: false });
          w.setMaximizable(false);
          expect(w.isMaximizable()).to.be.false('isMaximizable');
          w.setMinimizable(false);
          expect(w.isMaximizable()).to.be.false('isMaximizable');
          w.setClosable(false);
          expect(w.isMaximizable()).to.be.false('isMaximizable');

          w.setMaximizable(true);
          expect(w.isMaximizable()).to.be.true('isMaximizable');
          w.setClosable(true);
          expect(w.isMaximizable()).to.be.true('isMaximizable');
          w.setFullScreenable(false);
          expect(w.isMaximizable()).to.be.true('isMaximizable');
        });
      });
    });

    ifdescribe(process.platform === 'win32')('maximizable state', () => {
      it('with properties', () => {
        it('is reset to its former state', () => {
          const w = new BrowserWindow({ show: false });
          w.maximizable = false;
          w.resizable = false;
          w.resizable = true;
          expect(w.maximizable).to.be.false('maximizable');
          w.maximizable = true;
          w.resizable = false;
          w.resizable = true;
          expect(w.maximizable).to.be.true('maximizable');
        });
      });

      it('with functions', () => {
        it('is reset to its former state', () => {
          const w = new BrowserWindow({ show: false });
          w.setMaximizable(false);
          w.setResizable(false);
          w.setResizable(true);
          expect(w.isMaximizable()).to.be.false('isMaximizable');
          w.setMaximizable(true);
          w.setResizable(false);
          w.setResizable(true);
          expect(w.isMaximizable()).to.be.true('isMaximizable');
        });
      });
    });

    ifdescribe(process.platform !== 'darwin')('menuBarVisible state', () => {
      describe('with properties', () => {
        it('can be changed', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.menuBarVisible).to.be.true();
          w.menuBarVisible = false;
          expect(w.menuBarVisible).to.be.false();
          w.menuBarVisible = true;
          expect(w.menuBarVisible).to.be.true();
        });
      });

      describe('with functions', () => {
        it('can be changed', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.isMenuBarVisible()).to.be.true('isMenuBarVisible');
          w.setMenuBarVisibility(false);
          expect(w.isMenuBarVisible()).to.be.false('isMenuBarVisible');
          w.setMenuBarVisibility(true);
          expect(w.isMenuBarVisible()).to.be.true('isMenuBarVisible');
        });
      });
    });

    ifdescribe(process.platform === 'darwin')('fullscreenable state', () => {
      it('with functions', () => {
        it('can be set with fullscreenable constructor option', () => {
          const w = new BrowserWindow({ show: false, fullscreenable: false });
          expect(w.isFullScreenable()).to.be.false('isFullScreenable');
        });

        it('can be changed', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.isFullScreenable()).to.be.true('isFullScreenable');
          w.setFullScreenable(false);
          expect(w.isFullScreenable()).to.be.false('isFullScreenable');
          w.setFullScreenable(true);
          expect(w.isFullScreenable()).to.be.true('isFullScreenable');
        });
      });
    });

    // fullscreen events are dispatched eagerly and twiddling things too fast can confuse poor Electron

    ifdescribe(process.platform === 'darwin')('kiosk state', () => {
      it('with properties', () => {
        it('can be set with a constructor property', () => {
          const w = new BrowserWindow({ kiosk: true });
          expect(w.kiosk).to.be.true();
        });

        it('can be changed ', async () => {
          const w = new BrowserWindow();
          const enterFullScreen = emittedOnce(w, 'enter-full-screen');
          w.kiosk = true;
          expect(w.isKiosk()).to.be.true('isKiosk');
          await enterFullScreen;

          await delay();
          const leaveFullScreen = emittedOnce(w, 'leave-full-screen');
          w.kiosk = false;
          expect(w.isKiosk()).to.be.false('isKiosk');
          await leaveFullScreen;
        });
      });

      it('with functions', () => {
        it('can be set with a constructor property', () => {
          const w = new BrowserWindow({ kiosk: true });
          expect(w.isKiosk()).to.be.true();
        });

        it('can be changed ', async () => {
          const w = new BrowserWindow();
          const enterFullScreen = emittedOnce(w, 'enter-full-screen');
          w.setKiosk(true);
          expect(w.isKiosk()).to.be.true('isKiosk');
          await enterFullScreen;

          await delay();
          const leaveFullScreen = emittedOnce(w, 'leave-full-screen');
          w.setKiosk(false);
          expect(w.isKiosk()).to.be.false('isKiosk');
          await leaveFullScreen;
        });
      });
    });

    ifdescribe(process.platform === 'darwin')('fullscreen state with resizable set', () => {
      it('resizable flag should be set to true and restored', async () => {
        const w = new BrowserWindow({ resizable: false });
        const enterFullScreen = emittedOnce(w, 'enter-full-screen');
        w.setFullScreen(true);
        await enterFullScreen;
        expect(w.resizable).to.be.true('resizable');

        await delay();
        const leaveFullScreen = emittedOnce(w, 'leave-full-screen');
        w.setFullScreen(false);
        await leaveFullScreen;
        expect(w.resizable).to.be.false('resizable');
      });
    });

    ifdescribe(process.platform === 'darwin')('fullscreen state', () => {
      it('should not cause a crash if called when exiting fullscreen', async () => {
        const w = new BrowserWindow();

        const enterFullScreen = emittedOnce(w, 'enter-full-screen');
        w.setFullScreen(true);
        await enterFullScreen;

        await delay();

        const leaveFullScreen = emittedOnce(w, 'leave-full-screen');
        w.setFullScreen(false);
        await leaveFullScreen;
      });

      it('can be changed with setFullScreen method', async () => {
        const w = new BrowserWindow();
        const enterFullScreen = emittedOnce(w, 'enter-full-screen');
        w.setFullScreen(true);
        await enterFullScreen;
        expect(w.isFullScreen()).to.be.true('isFullScreen');

        await delay();
        const leaveFullScreen = emittedOnce(w, 'leave-full-screen');
        w.setFullScreen(false);
        await leaveFullScreen;
        expect(w.isFullScreen()).to.be.false('isFullScreen');
      });

      it('handles several transitions starting with fullscreen', async () => {
        const w = new BrowserWindow({ fullscreen: true, show: true });

        expect(w.isFullScreen()).to.be.true('not fullscreen');

        w.setFullScreen(false);
        w.setFullScreen(true);

        const enterFullScreen = emittedNTimes(w, 'enter-full-screen', 2);
        await enterFullScreen;

        expect(w.isFullScreen()).to.be.true('not fullscreen');

        await delay();
        const leaveFullScreen = emittedOnce(w, 'leave-full-screen');
        w.setFullScreen(false);
        await leaveFullScreen;

        expect(w.isFullScreen()).to.be.false('is fullscreen');
      });

      it('handles several transitions in close proximity', async () => {
        const w = new BrowserWindow();

        expect(w.isFullScreen()).to.be.false('is fullscreen');

        w.setFullScreen(true);
        w.setFullScreen(false);
        w.setFullScreen(true);

        const enterFullScreen = emittedNTimes(w, 'enter-full-screen', 2);
        await enterFullScreen;

        expect(w.isFullScreen()).to.be.true('not fullscreen');
      });

      it('does not crash when exiting simpleFullScreen (properties)', async () => {
        const w = new BrowserWindow();
        w.setSimpleFullScreen(true);

        await delay(1000);

        w.setFullScreen(!w.isFullScreen());
      });

      it('does not crash when exiting simpleFullScreen (functions)', async () => {
        const w = new BrowserWindow();
        w.simpleFullScreen = true;

        await delay(1000);

        w.setFullScreen(!w.isFullScreen());
      });

      it('should not be changed by setKiosk method', async () => {
        const w = new BrowserWindow();
        const enterFullScreen = emittedOnce(w, 'enter-full-screen');
        w.setFullScreen(true);
        await enterFullScreen;
        expect(w.isFullScreen()).to.be.true('isFullScreen');
        await delay();
        w.setKiosk(true);
        await delay();
        w.setKiosk(false);
        expect(w.isFullScreen()).to.be.true('isFullScreen');
        const leaveFullScreen = emittedOnce(w, 'leave-full-screen');
        w.setFullScreen(false);
        await leaveFullScreen;
        expect(w.isFullScreen()).to.be.false('isFullScreen');
      });

      // FIXME: https://github.com/electron/electron/issues/30140
      xit('multiple windows inherit correct fullscreen state', async () => {
        const w = new BrowserWindow();
        const enterFullScreen = emittedOnce(w, 'enter-full-screen');
        w.setFullScreen(true);
        await enterFullScreen;
        expect(w.isFullScreen()).to.be.true('isFullScreen');
        await delay();
        const w2 = new BrowserWindow({ show: false });
        const enterFullScreen2 = emittedOnce(w2, 'enter-full-screen');
        w2.show();
        await enterFullScreen2;
        expect(w2.isFullScreen()).to.be.true('isFullScreen');
      });
    });

    describe('closable state', () => {
      it('with properties', () => {
        it('can be set with closable constructor option', () => {
          const w = new BrowserWindow({ show: false, closable: false });
          expect(w.closable).to.be.false('closable');
        });

        it('can be changed', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.closable).to.be.true('closable');
          w.closable = false;
          expect(w.closable).to.be.false('closable');
          w.closable = true;
          expect(w.closable).to.be.true('closable');
        });
      });

      it('with functions', () => {
        it('can be set with closable constructor option', () => {
          const w = new BrowserWindow({ show: false, closable: false });
          expect(w.isClosable()).to.be.false('isClosable');
        });

        it('can be changed', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.isClosable()).to.be.true('isClosable');
          w.setClosable(false);
          expect(w.isClosable()).to.be.false('isClosable');
          w.setClosable(true);
          expect(w.isClosable()).to.be.true('isClosable');
        });
      });
    });

    describe('hasShadow state', () => {
      it('with properties', () => {
        it('returns a boolean on all platforms', () => {
          const w = new BrowserWindow({ show: false });
          expect(w.shadow).to.be.a('boolean');
        });

        // On Windows there's no shadow by default & it can't be changed dynamically.
        it('can be changed with hasShadow option', () => {
          const hasShadow = process.platform !== 'darwin';
          const w = new BrowserWindow({ show: false, hasShadow });
          expect(w.shadow).to.equal(hasShadow);
        });

        it('can be changed with setHasShadow method', () => {
          const w = new BrowserWindow({ show: false });
          w.shadow = false;
          expect(w.shadow).to.be.false('hasShadow');
          w.shadow = true;
          expect(w.shadow).to.be.true('hasShadow');
          w.shadow = false;
          expect(w.shadow).to.be.false('hasShadow');
        });
      });

      describe('with functions', () => {
        it('returns a boolean on all platforms', () => {
          const w = new BrowserWindow({ show: false });
          const hasShadow = w.hasShadow();
          expect(hasShadow).to.be.a('boolean');
        });

        // On Windows there's no shadow by default & it can't be changed dynamically.
        it('can be changed with hasShadow option', () => {
          const hasShadow = process.platform !== 'darwin';
          const w = new BrowserWindow({ show: false, hasShadow });
          expect(w.hasShadow()).to.equal(hasShadow);
        });

        it('can be changed with setHasShadow method', () => {
          const w = new BrowserWindow({ show: false });
          w.setHasShadow(false);
          expect(w.hasShadow()).to.be.false('hasShadow');
          w.setHasShadow(true);
          expect(w.hasShadow()).to.be.true('hasShadow');
          w.setHasShadow(false);
          expect(w.hasShadow()).to.be.false('hasShadow');
        });
      });
    });
  });

  describe('window.getMediaSourceId()', () => {
    afterEach(closeAllWindows);
    it('returns valid source id', async () => {
      const w = new BrowserWindow({ show: false });
      const shown = emittedOnce(w, 'show');
      w.show();
      await shown;

      // Check format 'window:1234:0'.
      const sourceId = w.getMediaSourceId();
      expect(sourceId).to.match(/^window:\d+:\d+$/);
    });
  });

  ifdescribe(!process.env.ELECTRON_SKIP_NATIVE_MODULE_TESTS)('window.getNativeWindowHandle()', () => {
    afterEach(closeAllWindows);
    it('returns valid handle', () => {
      const w = new BrowserWindow({ show: false });
      // The module's source code is hosted at
      // https://github.com/electron/node-is-valid-window
      const isValidWindow = require('is-valid-window');
      expect(isValidWindow(w.getNativeWindowHandle())).to.be.true('is valid window');
    });
  });

  ifdescribe(process.platform === 'darwin')('previewFile', () => {
    afterEach(closeAllWindows);
    it('opens the path in Quick Look on macOS', () => {
      const w = new BrowserWindow({ show: false });
      expect(() => {
        w.previewFile(__filename);
        w.closeFilePreview();
      }).to.not.throw();
    });
  });

  describe('contextIsolation option with and without sandbox option', () => {
    const expectedContextData = {
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
    };

    afterEach(closeAllWindows);

    it('separates the page context from the Electron/preload context', async () => {
      const iw = new BrowserWindow({
        show: false,
        webPreferences: {
          contextIsolation: true,
          preload: path.join(fixtures, 'api', 'isolated-preload.js')
        }
      });
      const p = emittedOnce(ipcMain, 'isolated-world');
      iw.loadFile(path.join(fixtures, 'api', 'isolated.html'));
      const [, data] = await p;
      expect(data).to.deep.equal(expectedContextData);
    });
    it('recreates the contexts on reload', async () => {
      const iw = new BrowserWindow({
        show: false,
        webPreferences: {
          contextIsolation: true,
          preload: path.join(fixtures, 'api', 'isolated-preload.js')
        }
      });
      await iw.loadFile(path.join(fixtures, 'api', 'isolated.html'));
      const isolatedWorld = emittedOnce(ipcMain, 'isolated-world');
      iw.webContents.reload();
      const [, data] = await isolatedWorld;
      expect(data).to.deep.equal(expectedContextData);
    });
    it('enables context isolation on child windows', async () => {
      const iw = new BrowserWindow({
        show: false,
        webPreferences: {
          contextIsolation: true,
          preload: path.join(fixtures, 'api', 'isolated-preload.js')
        }
      });
      const browserWindowCreated = emittedOnce(app, 'browser-window-created');
      iw.loadFile(path.join(fixtures, 'pages', 'window-open.html'));
      const [, window] = await browserWindowCreated;
      expect(window.webContents.getLastWebPreferences().contextIsolation).to.be.true('contextIsolation');
    });
    it('separates the page context from the Electron/preload context with sandbox on', async () => {
      const ws = new BrowserWindow({
        show: false,
        webPreferences: {
          sandbox: true,
          contextIsolation: true,
          preload: path.join(fixtures, 'api', 'isolated-preload.js')
        }
      });
      const p = emittedOnce(ipcMain, 'isolated-world');
      ws.loadFile(path.join(fixtures, 'api', 'isolated.html'));
      const [, data] = await p;
      expect(data).to.deep.equal(expectedContextData);
    });
    it('recreates the contexts on reload with sandbox on', async () => {
      const ws = new BrowserWindow({
        show: false,
        webPreferences: {
          sandbox: true,
          contextIsolation: true,
          preload: path.join(fixtures, 'api', 'isolated-preload.js')
        }
      });
      await ws.loadFile(path.join(fixtures, 'api', 'isolated.html'));
      const isolatedWorld = emittedOnce(ipcMain, 'isolated-world');
      ws.webContents.reload();
      const [, data] = await isolatedWorld;
      expect(data).to.deep.equal(expectedContextData);
    });
    it('supports fetch api', async () => {
      const fetchWindow = new BrowserWindow({
        show: false,
        webPreferences: {
          contextIsolation: true,
          preload: path.join(fixtures, 'api', 'isolated-fetch-preload.js')
        }
      });
      const p = emittedOnce(ipcMain, 'isolated-fetch-error');
      fetchWindow.loadURL('about:blank');
      const [, error] = await p;
      expect(error).to.equal('Failed to fetch');
    });
    it('doesn\'t break ipc serialization', async () => {
      const iw = new BrowserWindow({
        show: false,
        webPreferences: {
          contextIsolation: true,
          preload: path.join(fixtures, 'api', 'isolated-preload.js')
        }
      });
      const p = emittedOnce(ipcMain, 'isolated-world');
      iw.loadURL('about:blank');
      iw.webContents.executeJavaScript(`
        const opened = window.open()
        openedLocation = opened.location.href
        opened.close()
        window.postMessage({openedLocation}, '*')
      `);
      const [, data] = await p;
      expect(data.pageContext.openedLocation).to.equal('about:blank');
    });
    it('reports process.contextIsolated', async () => {
      const iw = new BrowserWindow({
        show: false,
        webPreferences: {
          contextIsolation: true,
          preload: path.join(fixtures, 'api', 'isolated-process.js')
        }
      });
      const p = emittedOnce(ipcMain, 'context-isolation');
      iw.loadURL('about:blank');
      const [, contextIsolation] = await p;
      expect(contextIsolation).to.be.true('contextIsolation');
    });
  });

  it('reloading does not cause Node.js module API hangs after reload', (done) => {
    const w = new BrowserWindow({
      show: false,
      webPreferences: {
        nodeIntegration: true,
        contextIsolation: false
      }
    });

    let count = 0;
    ipcMain.on('async-node-api-done', () => {
      if (count === 3) {
        ipcMain.removeAllListeners('async-node-api-done');
        done();
      } else {
        count++;
        w.reload();
      }
    });

    w.loadFile(path.join(fixtures, 'pages', 'send-after-node.html'));
  });

  describe('window.webContents.focus()', () => {
    afterEach(closeAllWindows);
    it('focuses window', async () => {
      const w1 = new BrowserWindow({ x: 100, y: 300, width: 300, height: 200 });
      w1.loadURL('about:blank');
      const w2 = new BrowserWindow({ x: 300, y: 300, width: 300, height: 200 });
      w2.loadURL('about:blank');
      w1.webContents.focus();
      // Give focus some time to switch to w1
      await delay();
      expect(w1.webContents.isFocused()).to.be.true('focuses window');
    });
  });

  ifdescribe(features.isOffscreenRenderingEnabled())('offscreen rendering', () => {
    let w: BrowserWindow;
    beforeEach(function () {
      w = new BrowserWindow({
        width: 100,
        height: 100,
        show: false,
        webPreferences: {
          backgroundThrottling: false,
          offscreen: true
        }
      });
    });
    afterEach(closeAllWindows);

    it('creates offscreen window with correct size', async () => {
      const paint = emittedOnce(w.webContents, 'paint');
      w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'));
      const [,, data] = await paint;
      expect(data.constructor.name).to.equal('NativeImage');
      expect(data.isEmpty()).to.be.false('data is empty');
      const size = data.getSize();
      const { scaleFactor } = screen.getPrimaryDisplay();
      expect(size.width).to.be.closeTo(100 * scaleFactor, 2);
      expect(size.height).to.be.closeTo(100 * scaleFactor, 2);
    });

    it('does not crash after navigation', () => {
      w.webContents.loadURL('about:blank');
      w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'));
    });

    describe('window.webContents.isOffscreen()', () => {
      it('is true for offscreen type', () => {
        w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'));
        expect(w.webContents.isOffscreen()).to.be.true('isOffscreen');
      });

      it('is false for regular window', () => {
        const c = new BrowserWindow({ show: false });
        expect(c.webContents.isOffscreen()).to.be.false('isOffscreen');
        c.destroy();
      });
    });

    describe('window.webContents.isPainting()', () => {
      it('returns whether is currently painting', async () => {
        const paint = emittedOnce(w.webContents, 'paint');
        w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'));
        await paint;
        expect(w.webContents.isPainting()).to.be.true('isPainting');
      });
    });

    describe('window.webContents.stopPainting()', () => {
      it('stops painting', async () => {
        const domReady = emittedOnce(w.webContents, 'dom-ready');
        w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'));
        await domReady;

        w.webContents.stopPainting();
        expect(w.webContents.isPainting()).to.be.false('isPainting');
      });
    });

    describe('window.webContents.startPainting()', () => {
      it('starts painting', async () => {
        const domReady = emittedOnce(w.webContents, 'dom-ready');
        w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'));
        await domReady;

        w.webContents.stopPainting();
        w.webContents.startPainting();

        await emittedOnce(w.webContents, 'paint');
        expect(w.webContents.isPainting()).to.be.true('isPainting');
      });
    });

    describe('frameRate APIs', () => {
      it('has default frame rate (function)', async () => {
        w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'));
        await emittedOnce(w.webContents, 'paint');
        expect(w.webContents.getFrameRate()).to.equal(60);
      });

      it('has default frame rate (property)', async () => {
        w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'));
        await emittedOnce(w.webContents, 'paint');
        expect(w.webContents.frameRate).to.equal(60);
      });

      it('sets custom frame rate (function)', async () => {
        const domReady = emittedOnce(w.webContents, 'dom-ready');
        w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'));
        await domReady;

        w.webContents.setFrameRate(30);

        await emittedOnce(w.webContents, 'paint');
        expect(w.webContents.getFrameRate()).to.equal(30);
      });

      it('sets custom frame rate (property)', async () => {
        const domReady = emittedOnce(w.webContents, 'dom-ready');
        w.loadFile(path.join(fixtures, 'api', 'offscreen-rendering.html'));
        await domReady;

        w.webContents.frameRate = 30;

        await emittedOnce(w.webContents, 'paint');
        expect(w.webContents.frameRate).to.equal(30);
      });
    });
  });

  describe('"transparent" option', () => {
    afterEach(closeAllWindows);

    // Only applicable on Windows where transparent windows can't be maximized.
    ifit(process.platform === 'win32')('can show maximized frameless window', async () => {
      const display = screen.getPrimaryDisplay();

      const w = new BrowserWindow({
        ...display.bounds,
        frame: false,
        transparent: true,
        show: true
      });

      w.loadURL('about:blank');
      await emittedOnce(w, 'ready-to-show');

      expect(w.isMaximized()).to.be.true();

      // Fails when the transparent HWND is in an invalid maximized state.
      expect(w.getBounds()).to.deep.equal(display.workArea);

      const newBounds = { width: 256, height: 256, x: 0, y: 0 };
      w.setBounds(newBounds);
      expect(w.getBounds()).to.deep.equal(newBounds);
    });
  });
});

import { BrowserWindow, webContents, app } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';
import * as fs from 'node:fs';
import * as http from 'node:http';
import { AddressInfo } from 'node:net';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';

import { ifdescribe, defer, listen } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';

const fixturesPath = path.resolve(__dirname, 'fixtures');
const features = process._linkedBinding('electron_common_features');

describe('webContents3 module', () => {
  describe('webrtc ip policy api', () => {
    afterEach(closeAllWindows);
    it('can set and get webrtc ip policies', () => {
      const w = new BrowserWindow({ show: false });
      const policies = [
        'default',
        'default_public_interface_only',
        'default_public_and_private_interfaces',
        'disable_non_proxied_udp'
      ] as const;
      for (const policy of policies) {
        w.webContents.setWebRTCIPHandlingPolicy(policy);
        expect(w.webContents.getWebRTCIPHandlingPolicy()).to.equal(policy);
      }
    });
  });

  describe('webrtc udp port range policy api', () => {
    let w: BrowserWindow;
    beforeEach(() => {
      w = new BrowserWindow({ show: false });
    });

    afterEach(closeAllWindows);

    it('check default webrtc udp port range is { min: 0, max: 0 }', () => {
      const settings = w.webContents.getWebRTCUDPPortRange();
      expect(settings).to.deep.equal({ min: 0, max: 0 });
    });

    it('can set and get webrtc udp port range policy with correct arguments', () => {
      w.webContents.setWebRTCUDPPortRange({ min: 1, max: 65535 });
      const settings = w.webContents.getWebRTCUDPPortRange();
      expect(settings).to.deep.equal({ min: 1, max: 65535 });
    });

    it('can not set webrtc udp port range policy with invalid arguments', () => {
      expect(() => {
        w.webContents.setWebRTCUDPPortRange({ min: 0, max: 65535 });
      }).to.throw("'min' and 'max' must be in the (0, 65535] range or [0, 0]");
      expect(() => {
        w.webContents.setWebRTCUDPPortRange({ min: 1, max: 65536 });
      }).to.throw("'min' and 'max' must be in the (0, 65535] range or [0, 0]");
      expect(() => {
        w.webContents.setWebRTCUDPPortRange({ min: 60000, max: 56789 });
      }).to.throw("'max' must be greater than or equal to 'min'");
    });

    it('can reset webrtc udp port range policy to default with { min: 0, max: 0 }', () => {
      w.webContents.setWebRTCUDPPortRange({ min: 1, max: 65535 });
      const settings = w.webContents.getWebRTCUDPPortRange();
      expect(settings).to.deep.equal({ min: 1, max: 65535 });
      w.webContents.setWebRTCUDPPortRange({ min: 0, max: 0 });
      const defaultSetting = w.webContents.getWebRTCUDPPortRange();
      expect(defaultSetting).to.deep.equal({ min: 0, max: 0 });
    });
  });

  describe('opener api', () => {
    afterEach(closeAllWindows);
    it('can get opener with window.open()', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { sandbox: true } });
      await w.loadURL('about:blank');
      const childPromise = once(w.webContents, 'did-create-window') as Promise<[BrowserWindow, Electron.DidCreateWindowDetails]>;
      w.webContents.executeJavaScript('window.open("about:blank")', true);
      const [childWindow] = await childPromise;
      expect(childWindow.webContents.opener).to.equal(w.webContents.mainFrame);
    });
    it('has no opener when using "noopener"', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { sandbox: true } });
      await w.loadURL('about:blank');
      const childPromise = once(w.webContents, 'did-create-window') as Promise<[BrowserWindow, Electron.DidCreateWindowDetails]>;
      w.webContents.executeJavaScript('window.open("about:blank", undefined, "noopener")', true);
      const [childWindow] = await childPromise;
      expect(childWindow.webContents.opener).to.be.null();
    });
    it('can get opener with a[target=_blank][rel=opener]', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { sandbox: true } });
      await w.loadURL('about:blank');
      const childPromise = once(w.webContents, 'did-create-window') as Promise<[BrowserWindow, Electron.DidCreateWindowDetails]>;
      w.webContents.executeJavaScript(`(function() {
        const a = document.createElement('a');
        a.target = '_blank';
        a.rel = 'opener';
        a.href = 'about:blank';
        a.click();
      }())`, true);
      const [childWindow] = await childPromise;
      expect(childWindow.webContents.opener).to.equal(w.webContents.mainFrame);
    });
    it('has no opener with a[target=_blank][rel=noopener]', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { sandbox: true } });
      await w.loadURL('about:blank');
      const childPromise = once(w.webContents, 'did-create-window') as Promise<[BrowserWindow, Electron.DidCreateWindowDetails]>;
      w.webContents.executeJavaScript(`(function() {
        const a = document.createElement('a');
        a.target = '_blank';
        a.rel = 'noopener';
        a.href = 'about:blank';
        a.click();
      }())`, true);
      const [childWindow] = await childPromise;
      expect(childWindow.webContents.opener).to.be.null();
    });
  });

  describe('render view deleted events', () => {
    let server: http.Server;
    let serverUrl: string;
    let crossSiteUrl: string;

    before(async () => {
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
        setTimeout().then(respond);
      });
      serverUrl = (await listen(server)).url;
      crossSiteUrl = serverUrl.replace('127.0.0.1', 'localhost');
    });

    after(() => {
      server.close();
      server = null as unknown as http.Server;
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
      const destroyed = once(w.webContents, 'destroyed');
      w.loadURL(`${serverUrl}/redirect-cross-site`);
      await destroyed;
      expect(currentRenderViewDeletedEmitted).to.be.false('current-render-view-deleted was emitted');
    });

    it('does not emit current-render-view-deleted when speculative RVHs are deleted', async () => {
      const parentWindow = new BrowserWindow({ show: false });
      let currentRenderViewDeletedEmitted = false;
      let childWindow: BrowserWindow | null = null;
      const destroyed = once(parentWindow.webContents, 'destroyed');
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
      const destroyed = once(w.webContents, 'destroyed');
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
      const destroyed = once(w.webContents, 'destroyed');
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
        const crashEvent = once(w.webContents, 'render-process-gone') as Promise<[any, Electron.RenderProcessGoneDetails]>;
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
            done(new Error('unsupported endpoint'));
        }
      });
      listen(server).then(({ url }) => {
        serverUrl = url;
        done();
      });
    });

    after(() => {
      server.close();
      server = null as unknown as http.Server;
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

        const contents = (webContents as typeof ElectronInternal.WebContents).create();
        const originalEmit = contents.emit.bind(contents);
        contents.emit = (...args) => { return originalEmit(...args); };
        contents.once(e.name as any, () => contents.destroy());
        const destroyed = once(contents, 'destroyed');
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
      w.webContents.on('console-message', (e) => {
        // Don't just assert as Chromium might emit other logs that we should ignore.
        if (e.message === 'a') {
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

      const [, channel, message] = await once(w.webContents, 'ipc-message');
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
          event.returnValue = 'foobar';
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
      let server = http.createServer((req, res) => {
        if (req.url === '/should_have_referrer') {
          try {
            expect(req.headers.referer).to.equal(`http://127.0.0.1:${(server.address() as AddressInfo).port}/`);
            return done();
          } catch (e) {
            return done(e);
          }
        }
        res.end('<a id="a" href="/should_have_referrer" target="_blank">link</a>');
      });
      defer(() => {
        server.close();
        server = null as unknown as http.Server;
      });
      listen(server).then(({ url }) => {
        w.webContents.once('did-finish-load', () => {
          w.webContents.setWindowOpenHandler(details => {
            expect(details.referrer.url).to.equal(url + '/');
            expect(details.referrer.policy).to.equal('strict-origin-when-cross-origin');
            return { action: 'allow' };
          });
          w.webContents.executeJavaScript('a.click()');
        });
        w.loadURL(url);
      });
    });

    it('propagates referrer information to windows opened with window.open', (done) => {
      const w = new BrowserWindow({ show: false });
      let server = http.createServer((req, res) => {
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
      defer(() => {
        server.close();
        server = null as unknown as http.Server;
      });
      listen(server).then(({ url }) => {
        w.webContents.once('did-finish-load', () => {
          w.webContents.setWindowOpenHandler(details => {
            expect(details.referrer.url).to.equal(url + '/');
            expect(details.referrer.policy).to.equal('strict-origin-when-cross-origin');
            return { action: 'allow' };
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

          const promise = once(w.webContents, 'preload-error') as Promise<[any, string, Error]>;
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

          const promise = once(w.webContents, 'preload-error') as Promise<[any, string, Error]>;
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

          const promise = once(w.webContents, 'preload-error') as Promise<[any, string, Error]>;
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
        } catch {
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

      const badPath = path.join('i', 'am', 'a', 'super', 'bad', 'path');
      const promise = w.webContents.takeHeapSnapshot(badPath);
      return expect(promise).to.be.eventually.rejectedWith(Error, `Failed to take heap snapshot with invalid file path ${badPath}`);
    });

    it('fails with invalid render process', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          sandbox: true
        }
      });

      const filePath = path.join(app.getPath('temp'), 'test.heapsnapshot');

      w.webContents.destroy();
      const promise = w.webContents.takeHeapSnapshot(filePath);
      return expect(promise).to.be.eventually.rejectedWith(Error, 'Failed to take heap snapshot with nonexistent render frame');
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

      w.setBackgroundThrottling(true);
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

      w.setBackgroundThrottling(false);
      expect(w.getBackgroundThrottling()).to.equal(false);

      w.setBackgroundThrottling(true);
      expect(w.getBackgroundThrottling()).to.equal(true);
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
});

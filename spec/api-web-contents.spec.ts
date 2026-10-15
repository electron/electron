import {
  BrowserWindow,
  ipcMain,
  webContents,
  session,
  app,
  BrowserView,
  type WebContents,
  BaseWindow,
  WebContentsView,
  Menu
} from 'electron/main';

import { afterAll, afterEach, assert, beforeAll, beforeEach, describe, expect, it } from 'vitest';

import { once } from 'node:events';
import * as fs from 'node:fs';
import * as http from 'node:http';
import * as os from 'node:os';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';
import * as url from 'node:url';
import * as vm from 'node:vm';

import { captureWithTabSourceId } from './lib/media-helpers.ts';
import { containsText, readPDF, readPDFs } from './lib/pdf-helpers.ts';
import {
  ifdescribe,
  defer,
  waitUntil,
  listen,
  ifit,
  isTestingBindingAvailable,
  startRemoteControlApp
} from './lib/spec-helpers.ts';
import { WebmGenerator } from './lib/video-helpers.js';
import { cleanupWebContents, closeAllWindows } from './lib/window-helpers.ts';

import type { AddressInfo } from 'node:net';

const fixturesPath = path.resolve(import.meta.dirname, 'fixtures');
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

      (await once(w.webContents, 'did-attach-webview')) as [any, WebContents];

      w.webContents.openDevTools();

      await once(w.webContents, 'devtools-opened');

      const all = webContents.getAllWebContents().sort((a, b) => {
        return a.id - b.id;
      });

      expect(all).toHaveLength(3);
      expect(all[0].getType()).toBe('window');
      expect(all[all.length - 2].getType()).toBe('webview');
      expect(all[all.length - 1].getType()).toBe('remote');
    });
  });

  describe('webContents properties', () => {
    afterEach(closeAllWindows);

    it('has expected additional enumerable properties', () => {
      const w = new BrowserWindow({ show: false });
      const properties = Object.getOwnPropertyNames(w.webContents);
      expect(properties).toContain('ipc');
      expect(properties).toContain('navigationHistory');
    });

    it('throws when a method is called with a receiver of another native type', () => {
      const w = new BrowserWindow({ show: false });
      const isLoading = w.webContents.isLoading as () => boolean;
      expect(() => isLoading.call(session.defaultSession)).toThrow(TypeError);
      expect(() => isLoading.call({})).toThrow(TypeError);
    });
  });

  describe('fromId()', () => {
    it('returns undefined for an unknown id', () => {
      expect(webContents.fromId(12345)).toBeUndefined();
    });
  });

  describe('fromFrame()', () => {
    afterEach(cleanupWebContents);
    it('returns WebContents for mainFrame', () => {
      const contents = (webContents as typeof ElectronInternal.WebContents).create();
      expect(webContents.fromFrame(contents.mainFrame)).toBe(contents);
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
      expect(errored).toBe(true);
    });
  });

  describe('fromDevToolsTargetId()', () => {
    afterEach(closeAllWindows);
    it('returns WebContents for attached DevTools target', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');
      try {
        await w.webContents.debugger.attach('1.3');
        const { targetInfo } = await w.webContents.debugger.sendCommand('Target.getTargetInfo');
        expect(webContents.fromDevToolsTargetId(targetInfo.targetId)).toBe(w.webContents);
      } finally {
        await w.webContents.debugger.detach();
      }
    });

    it('returns undefined for an unknown id', () => {
      expect(webContents.fromDevToolsTargetId('nope')).toBeUndefined();
    });
  });

  describe('will-prevent-unload event', () => {
    afterEach(async () => {
      await closeAllWindows();
      await cleanupWebContents();
    });

    it('does not emit if beforeunload returns undefined in a BrowserWindow', async () => {
      const w = new BrowserWindow({ show: false });
      w.webContents.once('will-prevent-unload', () => {
        expect.fail('should not have fired');
      });
      await w.loadFile(path.join(import.meta.dirname, 'fixtures', 'api', 'beforeunload-undefined.html'));
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

      await view.webContents.loadFile(path.join(import.meta.dirname, 'fixtures', 'api', 'beforeunload-undefined.html'));
      const wait = once(w, 'closed');
      w.close();
      await wait;
    });

    it('emits if beforeunload returns false in a BrowserWindow', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(import.meta.dirname, 'fixtures', 'api', 'beforeunload-false.html'));
      w.close();
      await once(w.webContents, 'will-prevent-unload');
    });

    it('emits if beforeunload returns false in a BrowserView', async () => {
      const w = new BrowserWindow({ show: false });
      const view = new BrowserView();
      w.setBrowserView(view);
      view.setBounds(w.getBounds());

      await view.webContents.loadFile(path.join(import.meta.dirname, 'fixtures', 'api', 'beforeunload-false.html'));
      w.close();
      await once(view.webContents, 'will-prevent-unload');
    });

    // The beforeunload prompt blocks the renderer; a window that is
    // re-activated while blocked used to keep aura focus without Blink focus.
    ifit(process.platform === 'win32')(
      'keeps the page focused when the window is re-activated while the prompt is pending',
      { tags: ['serial'] },
      async () => {
        const w = new BrowserWindow({ show: true });
        await w.loadFile(path.join(import.meta.dirname, 'fixtures', 'api', 'beforeunload-false.html'));
        w.webContents.focus();
        const other = new BrowserWindow({ show: true });
        w.focus();
        w.webContents.once('will-prevent-unload', () => {
          other.focus();
          w.focus();
        });
        // A navigation only prompts after a user gesture; close() always does.
        w.close();
        await once(w.webContents, 'will-prevent-unload');
        // Re-activation is asynchronous, so wait for focus instead of a fixed time.
        await waitUntil(
          async () => w.webContents.isFocused() && (await w.webContents.executeJavaScript('document.hasFocus()'))
        );
      }
    );

    it('supports calling preventDefault on will-prevent-unload events in a BrowserWindow', async () => {
      const w = new BrowserWindow({ show: false });
      w.webContents.once('will-prevent-unload', (event) => event.preventDefault());
      await w.loadFile(path.join(import.meta.dirname, 'fixtures', 'api', 'beforeunload-false.html'));
      const wait = once(w, 'closed');
      w.close();
      await wait;
    });

    it('fails loading a subsequent page after beforeunload is not prevented', async () => {
      const w = new BrowserWindow({ show: false });

      const didFailLoad = once(w.webContents, 'did-fail-load');
      await w.loadFile(path.join(import.meta.dirname, 'fixtures', 'api', 'beforeunload-false.html'));
      await w.webContents.executeJavaScript("console.log('gesture')", true);

      w.loadFile(path.join(import.meta.dirname, 'fixtures', 'pages', 'a.html'));
      const [, code, , validatedURL] = await didFailLoad;
      expect(code).toBe(-3); // ERR_ABORTED
      const { href: expectedURL } = url.pathToFileURL(path.join(import.meta.dirname, 'fixtures', 'pages', 'a.html'));
      expect(validatedURL).toBe(expectedURL);
    });

    it('allows loading a subsequent page after beforeunload is prevented', async () => {
      const w = new BrowserWindow({ show: false });
      w.webContents.once('will-prevent-unload', (event) => event.preventDefault());

      await w.loadFile(path.join(import.meta.dirname, 'fixtures', 'api', 'beforeunload-false.html'));
      await w.webContents.executeJavaScript("console.log('gesture')", true);
      await w.loadFile(path.join(import.meta.dirname, 'fixtures', 'pages', 'a.html'));
      const pageTitle = await w.webContents.executeJavaScript('document.title');
      expect(pageTitle).toBe('test');

      const wait = once(w, 'closed');
      w.close();
      await wait;
    });
  });

  describe('webContents.sendToFrame(frameId, channel, args...)', () => {
    afterEach(closeAllWindows);
    it('only addresses frames that belong to this webContents', async () => {
      const preload = path.join(fixturesPath, 'module', 'preload-ipc-ping-pong.js');
      const w1 = new BrowserWindow({
        show: false,
        webPreferences: { preload, sandbox: false, contextIsolation: false }
      });
      const w2 = new BrowserWindow({
        show: false,
        webPreferences: { preload, sandbox: false, contextIsolation: false }
      });
      await w1.loadURL('about:blank');
      await w2.loadURL('about:blank');
      const received: number[] = [];
      ipcMain.on('pong', (e) => {
        received.push(e.sender.id);
      });
      defer(() => ipcMain.removeAllListeners('pong'));
      const other = w2.webContents.mainFrame;
      expect(w1.webContents.sendToFrame([other.processId, other.routingId], 'ping')).toBe(false);
      const own = w1.webContents.mainFrame;
      expect(w1.webContents.sendToFrame([own.processId, own.routingId], 'ping')).toBe(true);
      await waitUntil(() => received.length > 0);
      await setTimeout(200);
      expect(received).toEqual([w1.webContents.id]);
    });
  });

  describe('webContents.send(channel, args...)', () => {
    afterEach(closeAllWindows);
    it('throws an error when the channel is missing', () => {
      const w = new BrowserWindow({ show: false });
      expect(() => {
        (w.webContents.send as any)();
      }).toThrow('Missing required channel argument');

      expect(() => {
        w.webContents.send(null as any);
      }).toThrow('Missing required channel argument');
    });

    it('does not block node async APIs when sent before document is ready', () =>
      new Promise<void>((resolve) => {
        // Please reference https://github.com/electron/electron/issues/19368 if
        // this test fails.
        ipcMain.once('async-node-api-done', () => {
          resolve();
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
      }));
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
      }).not.toThrow();

      expect(() => {
        w.webContents.print(undefined);
      }).not.toThrow();
    });

    it('does not throw when options object is empty', () => {
      expect(() => {
        w.webContents.print({});
      }).not.toThrow();
    });

    it('throws when invalid settings are passed', () => {
      expect(() => {
        // @ts-ignore this line is intentionally incorrect
        w.webContents.print(true);
      }).toThrow('webContents.print(): Invalid print settings specified.');

      expect(() => {
        // @ts-ignore this line is intentionally incorrect
        w.webContents.print(null);
      }).toThrow('webContents.print(): Invalid print settings specified.');
    });

    it('throws when an invalid pageSize is passed', () => {
      const badSize = 5;
      expect(() => {
        // @ts-ignore this line is intentionally incorrect
        w.webContents.print({ pageSize: badSize });
      }).toThrow(`Unsupported pageSize: ${badSize}`);
    });

    it('throws when a user passes both pageSize and usePrinterDefaultPageSize', () => {
      expect(() => {
        w.webContents.print({ pageSize: 'A4', usePrinterDefaultPageSize: true });
      }).toThrow('usePrinterDefaultPageSize cannot be combined with pageSize');
    });

    it('throws when an invalid callback is passed', () => {
      expect(() => {
        // @ts-ignore this line is intentionally incorrect
        w.webContents.print({}, true);
      }).toThrow('webContents.print(): Invalid optional callback provided.');
    });

    it('fails when an invalid deviceName is passed', () =>
      new Promise<void>((resolve) => {
        w.webContents.print({ deviceName: 'i-am-a-nonexistent-printer' }, (success, reason) => {
          expect(success).toBe(false);
          expect(reason).toMatch(/Invalid deviceName provided/);
          resolve();
        });
      }));

    it('throws when an invalid pageSize is passed', () => {
      expect(() => {
        // @ts-ignore this line is intentionally incorrect
        w.webContents.print({ pageSize: 'i-am-a-bad-pagesize' }, () => {});
      }).toThrow('Unsupported pageSize: i-am-a-bad-pagesize');
    });

    it('throws when an invalid custom pageSize is passed', () => {
      expect(() => {
        w.webContents.print({
          pageSize: {
            width: 100,
            height: 200
          }
        });
      }).toThrow('height and width properties must be minimum 352 microns.');
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
      }).not.toThrow();
    });
  });

  // Exercises a real silent print with options against a virtual printer
  // provisioned by script/spec-runner.js and exposed via
  // ELECTRON_TEST_PRINTER_NAME.
  // Self-skips when no such printer is available.
  ifdescribe(features.isPrintingEnabled())('webContents.print() settings', () => {
    let w: BrowserWindow;
    const deviceName = process.env.ELECTRON_TEST_PRINTER_NAME ?? null;

    const printerVisible = async (name: string) => {
      const probe = new BrowserWindow({ show: false });
      try {
        await probe.loadURL('about:blank');
        return await waitUntil(
          async () => {
            const printers = await probe.webContents.getPrintersAsync();
            return printers.some((p) => p.name === name);
          },
          { timeout: 10000 }
        )
          .then(() => true)
          .catch(() => false);
      } finally {
        probe.destroy();
      }
    };

    let canPrint = false;
    beforeAll(async () => {
      canPrint = !!deviceName && (await printerVisible(deviceName));
    }, 30000);

    beforeEach(() => {
      w = new BrowserWindow({ show: false });
    });
    afterEach(closeAllWindows);

    it('resolves settings for a silent print with options', { timeout: 60000 }, async (ctx) => {
      if (!canPrint || !deviceName) return ctx.skip();

      await w.loadURL('data:text/html,<h1>print test</h1>');

      const printResult = new Promise<[boolean, string]>((resolve) => {
        w.webContents.print({ silent: true, deviceName, printBackground: true }, (success, failureReason) =>
          resolve([success, failureReason])
        );
      });

      // Guard against environments where silent printing surfaces a native
      // dialog (which would block the callback) — skip rather than hang.
      const result = await Promise.race([printResult, setTimeout(30000).then(() => 'timeout' as const)]);
      if (result === 'timeout') return ctx.skip();

      const [success, failureReason] = result;
      // Regression guard for #52266: non-empty print settings must never again
      // be rejected up front during settings resolution with "Invalid printer
      // settings".
      expect(failureReason, `settings resolution failed: ${failureReason}`).not.toMatch(/Invalid printer settings/);
      expect(success, `print failed: ${failureReason}`).toBe(true);
    });
  });

  describe('webContents.executeJavaScript', () => {
    describe('in about:blank', () => {
      const expected = 'hello, world!';
      const expectedErrorMsg = 'woops!';
      const code = `(() => "${expected}")()`;
      const asyncCode = `(() => new Promise(r => setTimeout(() => r("${expected}"), 500)))()`;
      const badAsyncCode = `(() => new Promise((r, e) => setTimeout(() => e("${expectedErrorMsg}"), 500)))()`;
      const errorTypes = new Set([Error, ReferenceError, EvalError, RangeError, SyntaxError, TypeError, URIError]);
      let w: BrowserWindow;

      beforeEach(async () => {
        w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: false } });
        await w.loadURL('about:blank');
      });
      afterEach(async () => {
        await closeAllWindows();
      });

      it('resolves the returned promise with the result', async () => {
        const result = await w.webContents.executeJavaScript(code);
        expect(result).toBe(expected);
      });
      it('resolves the returned promise with the result if the code returns an asynchronous promise', async () => {
        const result = await w.webContents.executeJavaScript(asyncCode);
        expect(result).toBe(expected);
      });
      it('rejects the returned promise if an async error is thrown', async () => {
        await expect(w.webContents.executeJavaScript(badAsyncCode)).rejects.toThrow(expectedErrorMsg);
      });
      it('rejects the returned promise with an error if an Error.prototype is thrown', async () => {
        for (const error of errorTypes) {
          await expect(
            w.webContents.executeJavaScript(`Promise.reject(new ${error.name}("Wamp-wamp"))`)
          ).rejects.toThrow(error);
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

      beforeAll(async () => {
        server = http.createServer((request, response) => {
          response.end();
        });
        serverUrl = (await listen(server)).url;
      });

      afterAll(() => {
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
        await w.webContents.executeJavaScript("console.log('hello')");
      });

      it('executes after page load', async () => {
        const executeJavaScript = w.webContents.executeJavaScript('(() => "test")()');
        w.loadURL(serverUrl);
        const result = await executeJavaScript;
        expect(result).toBe('test');
      });
    });
  });

  describe('webContents.executeJavaScriptInIsolatedWorld', () => {
    let w: BrowserWindow;

    beforeAll(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
      await w.loadURL('about:blank');
    });

    afterAll(() => w.close());

    it('resolves the returned promise with the result', async () => {
      await w.webContents.executeJavaScriptInIsolatedWorld(999, [{ code: 'window.X = 123' }]);
      const isolatedResult = await w.webContents.executeJavaScriptInIsolatedWorld(999, [{ code: 'window.X' }]);
      const mainWorldResult = await w.webContents.executeJavaScript('window.X');
      expect(isolatedResult).toBe(123);
      expect(mainWorldResult).toBe(undefined);
    });

    it('rejects when worldId is not an integer', async () => {
      await expect(w.webContents.executeJavaScriptInIsolatedWorld('1234' as any, [{ code: '1+1' }])).rejects.toThrow(
        'worldId must be an integer'
      );
    });
  });

  describe('loadURL() promise API', () => {
    let w: BrowserWindow;
    let s: http.Server;

    beforeAll(() => {
      session
        .fromPartition('loadurl-webcontents-spec')
        .setPermissionRequestHandler((webContents, permission, callback) => {
          if (permission === 'openExternal') {
            return callback(false);
          }
          callback(true);
        });
    });

    afterEach(() => {
      if (s) {
        s.close();
        s = null as unknown as http.Server;
      }
    });

    beforeEach(async () => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          partition: 'loadurl-webcontents-spec'
        }
      });
    });
    afterEach(closeAllWindows);

    afterAll(async () => {
      session.fromPartition('loadurl-webcontents-spec').setPermissionRequestHandler(null);
    });

    it('resolves when done loading', async () => {
      await w.loadURL('about:blank');
    });

    it('resolves when done loading a file URL', async () => {
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));
    });

    it('resolves when navigating within the page', async () => {
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));
      await setTimeout();
      await w.loadURL(w.getURL() + '#foo');
    });

    it('resolves after browser initiated navigation', async () => {
      let finishedLoading = false;
      w.webContents.on('did-finish-load', function () {
        finishedLoading = true;
      });

      await w.loadFile(path.join(fixturesPath, 'pages', 'navigate_in_page_and_wait.html'));
      expect(finishedLoading).toBe(true);
    });

    it('rejects when failing to load a file URL', async () => {
      await expect(w.loadURL('file:non-existent')).rejects.toHaveProperty('code', 'ERR_FILE_NOT_FOUND');
    });

    it('rejects when loading fails due to DNS not resolved', async () => {
      await expect(w.loadURL('https://notfound.localhost')).rejects.toHaveProperty('code', 'ERR_NAME_NOT_RESOLVED');
    });

    it('rejects when navigation is cancelled due to a bad scheme', async () => {
      await expect(w.loadURL('bad-scheme://foo')).rejects.toHaveProperty('code', 'ERR_FAILED');
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
          await w.webContents.debugger.sendCommand('Emulation.setDeviceMetricsOverride', deviceMetrics);
        }
      };

      try {
        await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
        await setEmulation();
        await w.loadURL('data:text/html,<h1>HELLO</h1>');
        await setEmulation();
      } catch (e) {
        expect((e as Error).message).toMatch(/Debugger is already attached to the target/);
      }
    });

    it('fails if loadURL is called inside did-start-loading', async () => {
      const result = Promise.all([once(w.webContents, 'did-fail-load'), once(w.webContents, 'did-stop-loading')]);

      w.webContents.once('did-start-loading', () => {
        w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      });

      w.loadURL('data:text/html,<h1>HELLO</h1>');

      const [[, , , validatedURL]] = await result;
      expect(validatedURL).toContain('blank.html');
    });

    it('fails if loadurl is called after the navigation is ready to commit', async () => {
      const result = Promise.all([once(w.webContents, 'did-fail-load'), once(w.webContents, 'did-stop-loading')]);

      // @ts-expect-error internal-only event.
      w.webContents.once('-ready-to-commit-navigation', () => {
        w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      });

      w.loadURL('data:text/html,<h1>HELLO</h1>');

      const [[, , , validatedURL]] = await result;
      expect(validatedURL).toContain('blank.html');
    });

    it('fails if loadURL is called inside did-redirect-navigation', async () => {
      const server = http.createServer((req, res) => {
        if (req.url === '/302') {
          res.statusCode = 302;
          res.setHeader('Location', '/200');
          res.end();
        } else if (req.url === '/200') {
          res.end('ok');
        } else {
          res.end();
        }
      });

      const { url } = await listen(server);

      const result = Promise.all([once(w.webContents, 'did-fail-load'), once(w.webContents, 'did-stop-loading')]);

      w.webContents.once('did-redirect-navigation', () => {
        w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      });
      w.loadURL(`${url}/302`);

      const [[, , , validatedURL]] = await result;
      server.close();
      expect(validatedURL).toContain('blank.html');
    });

    it('sets appropriate error information on rejection', async () => {
      let err: any;
      try {
        await w.loadURL('file:non-existent');
      } catch (e) {
        err = e;
      }
      expect(err).not.toBeNull();
      expect(err.code).toEqual('ERR_FILE_NOT_FOUND');
      expect(err.errno).toEqual(-6);
      expect(err.url).toEqual(process.platform === 'win32' ? 'file://non-existent/' : 'file:///non-existent');
    });

    it('rejects if the load is aborted', async () => {
      s = http.createServer(() => {
        /* never complete the request */
      });
      const { port } = await listen(s);
      const p = expect(w.loadURL(`http://127.0.0.1:${port}`)).rejects.toThrow(/ERR_ABORTED/);
      // load a different file before the first load completes, causing the
      // first load to be aborted.
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));
      await p;
    });

    it("doesn't reject when a subframe fails to load", async () => {
      let resp = null as unknown as http.ServerResponse;
      s = http.createServer((req, res) => {
        res.writeHead(200, { 'Content-Type': 'text/html' });
        res.write('<iframe src="http://notfound.localhost"></iframe>');
        resp = res;
        // don't end the response yet
      });
      const { port } = await listen(s);
      const p = new Promise<void>((resolve) => {
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
      const p = new Promise<void>((resolve) => {
        w.webContents.on('did-frame-finish-load', (event, isMainFrame) => {
          if (!isMainFrame) {
            resolve();
          }
        });
      });
      const main = w.loadURL(`http://127.0.0.1:${port}`);
      await p;
      resp.destroy(); // cause the main request to fail
      await expect(main).rejects.toHaveProperty('errno', -355); // ERR_INCOMPLETE_CHUNKED_ENCODING
    });

    it('subsequent load failures reject each time', async () => {
      await expect(w.loadURL('file:non-existent')).rejects.toThrow();
      await expect(w.loadURL('file:non-existent')).rejects.toThrow();
    });

    it('invalid URL load rejects', async () => {
      await expect(w.loadURL('invalidURL')).rejects.toThrow();
    });
  });

  describe('navigationHistory', () => {
    let w: BrowserWindow;
    const urlPage1 =
      'data:text/html,<html><head><script>document.title = "Page 1";</script></head><body></body></html>';
    const urlPage2 =
      'data:text/html,<html><head><script>document.title = "Page 2";</script></head><body></body></html>';
    const urlPage3 =
      'data:text/html,<html><head><script>document.title = "Page 3";</script></head><body></body></html>';

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
        expect(wasRemoved).toBe(true);
        expect(newLength).toBe(initialLength - 1);
      });

      it('should not remove the current active navigation entry', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        const activeIndex = w.webContents.navigationHistory.getActiveIndex();
        const wasRemoved = w.webContents.navigationHistory.removeEntryAtIndex(activeIndex);
        expect(wasRemoved).toBe(false);
      });

      it('should return false given an invalid index larger than history length', async () => {
        await w.loadURL(urlPage1);
        const wasRemoved = w.webContents.navigationHistory.removeEntryAtIndex(5); // Index larger than history length
        expect(wasRemoved).toBe(false);
      });

      it('should return false given an invalid negative index', async () => {
        await w.loadURL(urlPage1);
        const wasRemoved = w.webContents.navigationHistory.removeEntryAtIndex(-1); // Negative index
        expect(wasRemoved).toBe(false);
      });
    });

    describe('navigationHistory.canGoBack and navigationHistory.goBack API', () => {
      it('should not be able to go back if history is empty', async () => {
        expect(w.webContents.navigationHistory.canGoBack()).toBe(false);
      });

      it('should be able to go back if history is not empty', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        expect(w.webContents.navigationHistory.getActiveIndex()).toBe(1);
        expect(w.webContents.navigationHistory.canGoBack()).toBe(true);
        w.webContents.navigationHistory.goBack();
        expect(w.webContents.navigationHistory.getActiveIndex()).toBe(0);
      });

      it('should have the same window title if navigating back within the page', async () => {
        const title = 'Test';
        w.webContents.on('did-finish-load', () => {
          w.setTitle(title);
          w.loadURL(`file://${fixturesPath}/pages/navigation-history-anchor-in-page.html#next`);
        });
        await w.loadURL(`file://${fixturesPath}/pages/navigation-history-anchor-in-page.html`);
        w.webContents.navigationHistory.goBack();
        expect(w.getTitle()).toBe(title);
      });

      it('should update the title when navigating back or forward without browserWindow.setTitle()', async () => {
        const page = `
<html>
<head><meta charset="UTF-8"><title>Document</title></head>
<body>
<script>
  const setTitle = () => document.title = location.hash.slice(1) || 'Document';
  addEventListener('popstate', setTitle);
  setTitle(location.hash);
  window.navigate = name => {
        history.pushState(null, '', '#' + name);
        setTitle();
  };
</script>
<button id="btn1" onclick="navigate('path1')">Link 1</button>
<button id="btn2" onclick="navigate('path2')">Link 2</button>

</body>
</html>`.trim();

        w = new BrowserWindow({ show: false });

        await w.loadURL(`data:text/html;charset=utf-8,${encodeURIComponent(page)}`);

        const nav1 = once(w.webContents, 'did-navigate-in-page');
        await w.webContents.executeJavaScript('document.getElementById("btn1").click()', true);
        await nav1;
        await waitUntil(() => w.getTitle() === 'path1');

        const nav2 = once(w.webContents, 'did-navigate-in-page');
        await w.webContents.executeJavaScript('document.getElementById("btn2").click()', true);
        await nav2;
        await waitUntil(() => w.getTitle() === 'path2');

        expect(w.webContents.navigationHistory.length()).toBe(3);
        expect(w.webContents.navigationHistory.getActiveIndex()).toBe(2);
        expect(w.webContents.navigationHistory.canGoBack()).toBe(true);

        const back = once(w.webContents, 'did-navigate-in-page');
        w.webContents.navigationHistory.goBack();
        await back;
        await waitUntil(() => w.getTitle() === 'path1');

        const forward = once(w.webContents, 'did-navigate-in-page');
        w.webContents.navigationHistory.goForward();
        await forward;
        await waitUntil(() => w.getTitle() === 'path2');

        w.setTitle('My own Title');
        const back2 = once(w.webContents, 'did-navigate-in-page');
        w.webContents.navigationHistory.goBack();
        await back2;
        expect(w.getTitle()).toBe('My own Title');
      });

      it('should not update page title if there was no title update and window was created with title on back nav', async () => {
        // page has no <title> tag to preserve the one from BrowserWindow options
        const page = `<html><head></head><body>Test</body></html>`;
        w = new BrowserWindow({ show: false, title: 'Our custom title' });

        const pushed = once(w.webContents, 'did-navigate-in-page');
        await w.loadURL(`data:text/html;charset=utf-8,${encodeURIComponent(page)}`);
        await w.webContents.executeJavaScript("history.pushState({}, '', '#test')", true);
        await pushed;

        // we need to wait for both events to make sure all the relevant
        // code runs for the BaseWindow and title logic
        const back = once(w.webContents, 'did-navigate-in-page');
        const titleUpdated = once(w.webContents, 'page-title-updated');

        w.webContents.navigationHistory.goBack();

        await Promise.all([back, titleUpdated]);

        expect(w.getTitle()).toBe('Our custom title');
      });
    });

    describe('navigationHistory.canGoForward and navigationHistory.goForward API', () => {
      it('should not be able to go forward if history is empty', async () => {
        expect(w.webContents.navigationHistory.canGoForward()).toBe(false);
      });

      it('should not be able to go forward if current index is same as history length', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        expect(w.webContents.navigationHistory.canGoForward()).toBe(false);
      });

      it('should be able to go forward if history is not empty and active index is less than history length', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        w.webContents.navigationHistory.goBack();
        expect(w.webContents.navigationHistory.getActiveIndex()).toBe(0);
        expect(w.webContents.navigationHistory.canGoForward()).toBe(true);
        w.webContents.navigationHistory.goForward();
        expect(w.webContents.navigationHistory.getActiveIndex()).toBe(1);
      });

      it('should have the same window title if navigating forward within the page', async () => {
        const title = 'Test';
        w.webContents.on('did-finish-load', () => {
          w.setTitle(title);
          w.loadURL(`file://${fixturesPath}/pages/navigation-history-anchor-in-page.html#next`);
        });
        await w.loadURL(`file://${fixturesPath}/pages/navigation-history-anchor-in-page.html`);
        expect(w.getTitle()).toBe(title);
      });
    });

    describe('navigationHistory.canGoToOffset(index) and navigationHistory.goToOffset(index) API', () => {
      it('should not be able to go to invalid offset', async () => {
        expect(w.webContents.navigationHistory.canGoToOffset(-1)).toBe(false);
        expect(w.webContents.navigationHistory.canGoToOffset(10)).toBe(false);
      });

      it('should be able to go to valid negative offset', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);
        expect(w.webContents.navigationHistory.canGoToOffset(-2)).toBe(true);
        expect(w.webContents.navigationHistory.getActiveIndex()).toBe(2);
        w.webContents.navigationHistory.goToOffset(-2);
        expect(w.webContents.navigationHistory.getActiveIndex()).toBe(0);
      });

      it('should be able to go to valid positive offset', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);

        w.webContents.navigationHistory.goBack();
        expect(w.webContents.navigationHistory.canGoToOffset(1)).toBe(true);
        expect(w.webContents.navigationHistory.getActiveIndex()).toBe(1);
        w.webContents.navigationHistory.goToOffset(1);
        expect(w.webContents.navigationHistory.getActiveIndex()).toBe(2);
      });
    });

    describe('navigationHistory.goToIndex(index) API', () => {
      beforeEach(async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);

        expect(w.webContents.navigationHistory.getActiveIndex()).toBe(2);
      });

      it('should be able to go to a valid earlier index', async () => {
        w.webContents.navigationHistory.goToIndex(0);

        await waitUntil(() => w.webContents.navigationHistory.getActiveIndex() === 0);
        await waitUntil(() => w.webContents.getTitle() === 'Page 1');
      });

      it('should be able to go to a valid later index', async () => {
        w.webContents.navigationHistory.goToIndex(0);
        await waitUntil(() => w.webContents.navigationHistory.getActiveIndex() === 0);
        await waitUntil(() => w.webContents.getTitle() === 'Page 1');

        w.webContents.navigationHistory.goToIndex(2);

        await waitUntil(() => w.webContents.navigationHistory.getActiveIndex() === 2);
        await waitUntil(() => w.webContents.getTitle() === 'Page 3');
      });

      const expectNoEffect = async (fn: () => void) => {
        const activeIndex = w.webContents.navigationHistory.getActiveIndex();
        const title = w.webContents.getTitle();
        let didStartNavigationCount = 0;
        let didFinishLoadCount = 0;

        w.webContents.on('did-start-navigation', () => didStartNavigationCount++);
        w.webContents.on('did-finish-load', () => didFinishLoadCount++);

        fn();

        await setTimeout();

        expect(w.webContents.navigationHistory.getActiveIndex()).toBe(activeIndex);
        expect(w.webContents.getTitle()).toBe(title);
        expect(didStartNavigationCount).toBe(0);
        expect(didFinishLoadCount).toBe(0);
      };

      it('should do nothing when given the current active index', async () => {
        const activeIndex = w.webContents.navigationHistory.getActiveIndex();
        await expectNoEffect(() => w.webContents.navigationHistory.goToIndex(activeIndex));
      });

      it('should do nothing when given an invalid index', async () => {
        await expectNoEffect(() => {
          w.webContents.navigationHistory.goToIndex(-1);
          w.webContents.navigationHistory.goToIndex(w.webContents.navigationHistory.length());
        });
      });
    });

    describe('navigationHistory.clear API', () => {
      it('should be able clear history', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);

        expect(w.webContents.navigationHistory.length()).toBe(3);
        w.webContents.navigationHistory.clear();
        expect(w.webContents.navigationHistory.length()).toBe(1);
      });
    });

    describe('navigationHistory.getEntryAtIndex(index) API ', () => {
      it('should fetch default navigation entry when no urls are loaded', async () => {
        const result = w.webContents.navigationHistory.getEntryAtIndex(0);
        expect(result.url).toBe('');
        expect(result.title).toBe('');
      });
      it('should fetch navigation entry given a valid index', async () => {
        await w.loadURL(urlPage1);
        const result = w.webContents.navigationHistory.getEntryAtIndex(0);
        expect(result.url).toBe(urlPage1);
        expect(result.title).toBe('Page 1');
      });
      it('should return null given an invalid index larger than history length', async () => {
        await w.loadURL(urlPage1);
        const result = w.webContents.navigationHistory.getEntryAtIndex(5);
        expect(result).toBeNull();
      });
      it('should return null given an invalid negative index', async () => {
        await w.loadURL(urlPage1);
        const result = w.webContents.navigationHistory.getEntryAtIndex(-1);
        expect(result).toBeNull();
      });
    });

    describe('navigationHistory.getActiveIndex() API', () => {
      it('should return valid active index after a single page visit', async () => {
        await w.loadURL(urlPage1);
        expect(w.webContents.navigationHistory.getActiveIndex()).toBe(0);
      });

      it('should return valid active index after a multiple page visits', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);

        expect(w.webContents.navigationHistory.getActiveIndex()).toBe(2);
      });

      it('should return valid active index given no page visits', async () => {
        expect(w.webContents.navigationHistory.getActiveIndex()).toBe(0);
      });
    });

    describe('navigationHistory.length() API', () => {
      it('should return valid history length after a single page visit', async () => {
        await w.loadURL(urlPage1);
        expect(w.webContents.navigationHistory.length()).toBe(1);
      });

      it('should return valid history length after a multiple page visits', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);

        expect(w.webContents.navigationHistory.length()).toBe(3);
      });

      it('should return valid history length given no page visits', async () => {
        // Note: Even if no navigation has committed, the history list will always start with an initial navigation entry
        // Ref: https://source.chromium.org/chromium/chromium/src/+/main:ceontent/public/browser/navigation_controller.h;l=381
        expect(w.webContents.navigationHistory.length()).toBe(1);
      });
    });

    describe('navigationHistory.getAllEntries() API', () => {
      it('should return all navigation entries as an array of NavigationEntry objects', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);
        const entries = w.webContents.navigationHistory.getAllEntries().map((entry) => ({
          url: entry.url,
          title: entry.title
        }));
        expect(entries.length).toBe(3);
        expect(entries[0]).toEqual({ url: urlPage1, title: 'Page 1' });
        expect(entries[1]).toEqual({ url: urlPage2, title: 'Page 2' });
        expect(entries[2]).toEqual({ url: urlPage3, title: 'Page 3' });
      });

      it('should return an empty array when there is no navigation history', async () => {
        const entries = w.webContents.navigationHistory.getAllEntries();
        expect(entries.length).toBe(0);
      });

      it('should create a NavigationEntry with PageState that can be serialized/deserialized with JSON', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);

        const entries = w.webContents.navigationHistory.getAllEntries();
        const serialized = JSON.stringify(entries);
        const deserialized = JSON.parse(serialized);
        expect(deserialized).toEqual(entries);
      });
    });

    describe('navigationHistory.restore({ index, entries }) API', () => {
      let server: http.Server;
      let serverUrl: string;

      beforeAll(async () => {
        server = http.createServer((req, res) => {
          res.setHeader('Content-Type', 'text/html');
          res.end(
            '<html><head><title>Form</title></head><body><form><input type="text" value="value" /></form></body></html>'
          );
        });
        serverUrl = (await listen(server)).url;
      });

      afterAll(async () => {
        if (server) await new Promise((resolve) => server.close(resolve));
        server = null as any;
      });

      it('should restore navigation history with PageState', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(serverUrl);

        // Fill out the form on the page
        await w.webContents.executeJavaScript('document.querySelector("input").value = "Hi!";');

        // Wait for the renderer's delayed PageState sync (1s visible, 5s hidden)
        // while the form page is still current. The update sent on unload is
        // racy: the browser only briefly waits for the old frame to unload.
        // Form state is serialized as UTF-16 inside the PageState.
        const hasFormValue = (pageState?: string) =>
          !!pageState && Buffer.from(pageState, 'base64').includes(Buffer.from('Hi!', 'utf16le'));
        await waitUntil(() => hasFormValue(w.webContents.navigationHistory.getEntryAtIndex(2)?.pageState), {
          timeout: 20000
        });

        await w.loadURL(urlPage3);

        // Save the navigation state
        const entries = w.webContents.navigationHistory.getAllEntries();

        // Close the window, make a new one
        w.close();
        w = new BrowserWindow();

        // restore() resolves on did-finish-load, by which point blink has
        // restored the form controls (FormController::RestoreImmediately runs
        // before the load event).
        await w.webContents.navigationHistory.restore({ index: 2, entries });
        const formValue = await w.webContents.executeJavaScript('document.querySelector("input").value');
        expect(formValue).toBe('Hi!');
      });

      it('should handle invalid base64 pageState', async () => {
        await w.loadURL(urlPage1);
        await w.loadURL(urlPage2);
        await w.loadURL(urlPage3);

        const brokenEntries = w.webContents.navigationHistory.getAllEntries().map((entry) => ({
          ...entry,
          pageState: 'invalid base64'
        }));

        // Close the window, make a new one
        w.close();
        w = new BrowserWindow();
        await w.webContents.navigationHistory.restore({ index: 2, entries: brokenEntries });

        const entries = w.webContents.navigationHistory.getAllEntries();

        // Check that we used the original url and titles but threw away the broken
        // pageState
        entries.forEach((entry, index) => {
          expect(entry.url).toBe(brokenEntries[index].url);
          expect(entry.title).toBe(brokenEntries[index].title);
          expect(entry.pageState?.length).toBeGreaterThanOrEqual(100);
        });
      });
    });

    it('should restore an overridden user agent', async () => {
      const partition = 'persist:wcvtest';
      const testUA = 'MyCustomUA';

      const ses = session.fromPartition(partition);
      ses.setUserAgent(testUA);

      const wcv = new WebContentsView({
        webPreferences: { partition }
      });

      wcv.webContents.navigationHistory.restore({
        entries: [
          {
            url: urlPage1,
            title: 'url1'
          }
        ],
        index: 0
      });

      const ua = wcv.webContents.getUserAgent();
      const wcvua = await wcv.webContents.executeJavaScript('navigator.userAgent');

      expect(ua).toBe(wcvua);
    });
  });

  describe('getFocusedWebContents() API', { tags: ['serial'] }, () => {
    afterEach(closeAllWindows);

    // FIXME
    ifit(!(process.platform === 'win32' && process.arch === 'arm64'))('returns the focused web contents', async () => {
      const w = new BrowserWindow({ show: true });
      await w.loadFile(path.join(import.meta.dirname, 'fixtures', 'blank.html'));
      expect(webContents.getFocusedWebContents()?.id).toBe(w.webContents.id);

      const devToolsOpened = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools();
      await devToolsOpened;
      expect(webContents.getFocusedWebContents()?.id).toBe(w.webContents.devToolsWebContents!.id);
      const devToolsClosed = once(w.webContents, 'devtools-closed');
      w.webContents.closeDevTools();
      await devToolsClosed;
      expect(webContents.getFocusedWebContents()?.id).toBe(w.webContents.id);
    });

    it('does not crash when called on a detached dev tools window', async () => {
      const w = new BrowserWindow({ show: true });

      w.webContents.openDevTools({ mode: 'detach' });
      w.webContents.inspectElement(100, 100);

      // For some reason we have to wait for two focused events...?
      await once(w.webContents, 'devtools-focused');

      expect(() => {
        webContents.getFocusedWebContents();
      }).not.toThrow();

      // Work around https://github.com/electron/electron/issues/19985
      await setTimeout();

      const devToolsClosed = once(w.webContents, 'devtools-closed');
      w.webContents.closeDevTools();
      await devToolsClosed;
      expect(() => {
        webContents.getFocusedWebContents();
      }).not.toThrow();
    });

    it('Inspect activates detached devtools window', async () => {
      const window = new BrowserWindow({ show: true });
      await window.loadURL('about:blank');
      const webContentsBeforeOpenedDevtools = webContents.getAllWebContents();

      const windowWasBlurred = once(window, 'blur');
      window.webContents.openDevTools({ mode: 'detach' });
      await windowWasBlurred;

      let devToolsWebContents = null;
      for (const newWebContents of webContents.getAllWebContents()) {
        const oldWebContents = webContentsBeforeOpenedDevtools.find((oldWebContents) => {
          return newWebContents.id === oldWebContents.id;
        });
        if (oldWebContents !== null) {
          devToolsWebContents = newWebContents;
          break;
        }
      }
      assert(devToolsWebContents !== null);

      const windowFocused = once(window, 'focus');
      const devToolsBlurred = once(devToolsWebContents, 'blur');
      window.focus();
      await Promise.all([windowFocused, devToolsBlurred]);

      expect(devToolsWebContents.isFocused()).toBe(false);
      const devToolsWebContentsFocused = once(devToolsWebContents, 'focus');
      const windowBlurred = once(window, 'blur');
      window.webContents.inspectElement(100, 100);
      await Promise.all([devToolsWebContentsFocused, windowBlurred]);

      expect(devToolsWebContents.isFocused()).toBe(true);
      expect(window.isFocused()).toBe(false);
    });
  });

  describe('setDevToolsWebContents() API', () => {
    afterEach(closeAllWindows);
    it('sets arbitrary webContents as devtools', async () => {
      const w = new BrowserWindow({ show: false });
      const devtools = new BrowserWindow({ show: false });
      const promise = once(devtools.webContents, 'dom-ready');
      w.webContents.setDevToolsWebContents(devtools.webContents);
      w.webContents.openDevTools();
      await promise;
      expect(devtools.webContents.getURL().startsWith('devtools://devtools')).toBe(true);
      const result = await devtools.webContents.executeJavaScript('InspectorFrontendHost.constructor.name');
      expect(result).toBe('InspectorFrontendHostImpl');
      devtools.destroy();
    });

    it('falls back to the built-in devtools when the assigned webContents has been destroyed', async () => {
      const w = new BrowserWindow({ show: false });
      const devtools = new BrowserWindow({ show: false });
      w.webContents.setDevToolsWebContents(devtools.webContents);
      devtools.webContents.destroy();
      await once(devtools.webContents, 'destroyed');
      await setTimeout(50);
      const opened = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools({ mode: 'detach' });
      await opened;
      expect(w.webContents.isDevToolsOpened()).toBe(true);
      expect(w.webContents.devToolsWebContents).not.toBeNull();
    });
  });

  describe('isFocused() API', { tags: ['serial'] }, () => {
    afterEach(closeAllWindows);
    it('returns false when the window is hidden', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');
      expect(w.isVisible()).toBe(false);
      expect(w.webContents.isFocused()).toBe(false);
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
      let p = once(w.webContents, 'audio-state-changed');
      w.webContents.executeJavaScript('context.resume()');
      await p;
      expect(w.webContents.isCurrentlyAudible()).toBe(true);
      p = once(w.webContents, 'audio-state-changed');
      w.webContents.executeJavaScript('oscillator.stop()');
      await p;
      expect(w.webContents.isCurrentlyAudible()).toBe(false);
    });
  });

  describe('openDevTools() API', { tags: ['serial'] }, () => {
    afterEach(closeAllWindows);

    async function getViewportSize(w: BrowserWindow) {
      return await w.webContents.executeJavaScript('({ width: window.innerWidth, height: window.innerHeight })');
    }

    it('can show window with activation', async () => {
      const w = new BrowserWindow({ show: false });
      const focused = once(w, 'focus');
      w.show();
      await focused;
      expect(w.isFocused()).toBe(true);
      const blurred = once(w, 'blur');
      w.webContents.openDevTools({ mode: 'detach', activate: true });
      await Promise.all([once(w.webContents, 'devtools-opened'), once(w.webContents, 'devtools-focused')]);
      await blurred;
      expect(w.isFocused()).toBe(false);
    });

    it('can show window without activation', async () => {
      const w = new BrowserWindow({ show: false });
      const devtoolsOpened = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools({ mode: 'detach', activate: false });
      await devtoolsOpened;
      expect(w.webContents.isDevToolsOpened()).toBe(true);
    });

    // Regression test for https://github.com/electron/electron/issues/52158.
    // The api::WebContents wrapping the DevTools WebContents is created as soon
    // as openDevTools() is called but only referenced again once the frontend
    // has finished loading. A GC in between used to collect its wrapper, after
    // which `devtools-opened` would either crash the main process (if the
    // collected object had not been freed yet) or create a second
    // api::WebContents for the same DevTools WebContents.
    it('keeps the DevTools WebContents alive across a garbage collection while the frontend is loading', async () => {
      const gc = vm.runInNewContext('gc');
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');

      const created: number[] = [];
      const onCreated = (_: any, wc: WebContents) => {
        created.push(wc.id);
      };
      app.on('web-contents-created', onCreated);
      try {
        const devtoolsOpened = once(w.webContents, 'devtools-opened');
        w.webContents.openDevTools({ mode: 'detach', activate: false });
        gc({ type: 'major', execution: 'sync' });
        await devtoolsOpened;
      } finally {
        app.removeListener('web-contents-created', onCreated);
      }

      const devtools = w.webContents.devToolsWebContents;
      expect(devtools).not.toBeNull();
      expect(devtools!.isDestroyed()).toBe(false);
      // Exactly one WebContents (the DevTools one) was created, and it is the
      // one we ended up with.
      expect(created).toEqual([devtools!.id]);
    });

    it('updates and restores the inspected page viewport for right-docked DevTools', async () => {
      const w = new BrowserWindow({ show: false, width: 800, height: 600 });
      await w.loadURL('about:blank');

      // wait for it to be shown, visible
      const shown = once(w, 'show');
      w.show();
      await shown;
      await waitUntil(async () => (await w.webContents.executeJavaScript('document.visibilityState')) === 'visible');

      const initial = await getViewportSize(w);

      const devtoolsOpened = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools({ mode: 'right', activate: false });
      await devtoolsOpened;

      await waitUntil(async () => {
        const viewport = await getViewportSize(w);
        return viewport.width < initial.width;
      });

      const dockedRight = await getViewportSize(w);
      expect(dockedRight.width).toBeLessThan(initial.width);
      expect(Math.abs(dockedRight.height - initial.height)).toBeLessThanOrEqual(50);

      const devtoolsClosed = once(w.webContents, 'devtools-closed');
      w.webContents.closeDevTools();
      await devtoolsClosed;

      await waitUntil(async () => {
        const restoredViewport = await getViewportSize(w);
        return restoredViewport.width === initial.width && restoredViewport.height === initial.height;
      });

      const restoredViewport = await getViewportSize(w);
      expect(restoredViewport).toEqual(initial);
    });

    it('can show a DevTools window with custom title', async () => {
      const w = new BrowserWindow({ show: false });
      const devtoolsOpened = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools({ mode: 'detach', activate: false, title: 'myTitle' });
      await devtoolsOpened;
      expect(w.webContents.getDevToolsTitle()).toBe('myTitle');
    });

    it('can re-open devtools', async () => {
      const w = new BrowserWindow({ show: false });
      const devtoolsOpened = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools({ mode: 'detach', activate: true });
      await devtoolsOpened;
      expect(w.webContents.isDevToolsOpened()).toBe(true);

      const devtoolsClosed = once(w.webContents, 'devtools-closed');
      w.webContents.closeDevTools();
      await devtoolsClosed;
      expect(w.webContents.isDevToolsOpened()).toBe(false);

      const devtoolsOpened2 = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools({ mode: 'detach', activate: true });
      await devtoolsOpened2;
      expect(w.webContents.isDevToolsOpened()).toBe(true);
    });

    it('does not crash when closing DevTools immediately after opening', async () => {
      const w = new BrowserWindow({ show: true });
      await w.loadURL('about:blank');

      const devToolsFocused = once(w.webContents, 'devtools-focused');
      w.webContents.openDevTools({ mode: 'detach' });
      w.webContents.inspectElement(100, 100);
      await devToolsFocused;

      const devtoolsClosed = once(w.webContents, 'devtools-closed');
      w.webContents.closeDevTools();
      await devtoolsClosed;

      expect(w.webContents.isDevToolsOpened()).toBe(false);
    });
  });

  describe('setDevToolsTitle() API', () => {
    afterEach(closeAllWindows);
    it('can set devtools title with function', async () => {
      const w = new BrowserWindow({ show: false });
      const devtoolsOpened = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools({ mode: 'detach', activate: false });
      await devtoolsOpened;
      expect(w.webContents.isDevToolsOpened()).toBe(true);
      w.webContents.setDevToolsTitle('newTitle');
      expect(w.webContents.getDevToolsTitle()).toBe('newTitle');
    });
  });

  describe('DevTools showItemInFolder embedder message', () => {
    afterEach(closeAllWindows);

    async function openDevTools(w: BrowserWindow) {
      await w.loadURL('about:blank');
      const devtoolsOpened = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools({ mode: 'detach', activate: false });
      await devtoolsOpened;
      await waitUntil(() => w.webContents.devToolsWebContents!.executeJavaScript('typeof DevToolsAPI !== "undefined"'));
    }

    async function sendShowItemInFolder(w: BrowserWindow, target: string) {
      await w.webContents.devToolsWebContents!.executeJavaScript(
        `DevToolsAPI.sendMessageToEmbedder('showItemInFolder', [${JSON.stringify(target)}], null)`
      );
    }

    it('does not open or execute paths outside registered workspace folders', async () => {
      const w = new BrowserWindow({ show: false });
      await openDevTools(w);

      const candidates =
        process.platform === 'win32'
          ? ['C:\\Windows\\win.ini\\x', 'C:\\Windows\\System32\\drivers\\etc\\hosts']
          : ['/bin/ls/x', '/usr/bin/env'];

      for (const target of candidates) {
        await sendShowItemInFolder(w, target);
      }

      // The embedder handler must early-return for non-workspace paths and
      // must never call platform_util::OpenPath. Reaching this point without
      // the test runner being killed by a spawned process is the assertion;
      // additionally verify the renderer is still responsive.
      const alive = await w.webContents.executeJavaScript('true');
      expect(alive).toBe(true);
    });

    // On Linux without a DBus FileManager1 session, ShowItemInFolder falls
    // back to OpenFolder() which does a blocking DirectoryExists() on the UI
    // thread (pre-existing behavior). Workspace-gating is covered by the test
    // above.
    ifit(process.platform !== 'linux')(
      'reveals paths under a registered workspace folder without executing them',
      // Opens a real Finder/Explorer window, which takes focus and stays open.
      { tags: ['serial'] },
      async () => {
        const w = new BrowserWindow({ show: false });
        await openDevTools(w);

        const workspace = fs.mkdtempSync(path.join(os.tmpdir(), 'devtools-workspace-'));
        const target = path.join(workspace, 'file.txt');
        fs.writeFileSync(target, 'hello');
        defer(() => fs.rmSync(workspace, { recursive: true, force: true }));

        w.webContents.addWorkSpace(workspace);
        await sendShowItemInFolder(w, target);

        const alive = await w.webContents.executeJavaScript('true');
        expect(alive).toBe(true);
      }
    );
  });

  describe('DevTools native integration', () => {
    afterEach(closeAllWindows);

    async function openDevTools(w: BrowserWindow) {
      await w.loadURL('about:blank');
      const devtoolsOpened = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools({ mode: 'detach', activate: false });
      await devtoolsOpened;
      await waitUntil(() => w.webContents.devToolsWebContents!.executeJavaScript('typeof DevToolsAPI !== "undefined"'));
    }

    // The DevTools frontend should route context menus through the native
    // DevToolsHost binding rather than a JS override injected by Electron.
    function usesNativeShowContextMenu(devtoolsContents: Electron.WebContents): Promise<boolean> {
      return devtoolsContents.executeJavaScript(
        'typeof DevToolsHost !== "undefined" && InspectorFrontendHost.showContextMenuAtPoint.toString().includes("DevToolsHost")'
      );
    }

    // Triggers InspectorFrontendHost.showContextMenuAtPoint with an empty
    // item list at a non-editable point. No menu UI is shown, but a correctly
    // routed request still round-trips through the browser, which reports
    // the menu closed back to the frontend as DevToolsAPI.contextMenuCleared.
    // A request swallowed by the generic context-menu path never resolves.
    function devToolsMenuRequestRoundTrips(devtoolsContents: Electron.WebContents): Promise<boolean> {
      return devtoolsContents.executeJavaScript(`new Promise(resolve => {
        const timeout = setTimeout(() => resolve(false), 5000);
        const original = DevToolsAPI.contextMenuCleared.bind(DevToolsAPI);
        DevToolsAPI.contextMenuCleared = () => {
          clearTimeout(timeout);
          DevToolsAPI.contextMenuCleared = original;
          resolve(true);
          original();
        };
        InspectorFrontendHost.showContextMenuAtPoint(10, 10, [], document);
      })`);
    }

    it('uses the native showContextMenuAtPoint implementation', async () => {
      const w = new BrowserWindow({ show: false });
      await openDevTools(w);

      expect(await usesNativeShowContextMenu(w.webContents.devToolsWebContents!)).toBe(true);
    });

    it('uses the native window.confirm implementation', async () => {
      const w = new BrowserWindow({ show: false });
      await openDevTools(w);

      // window.confirm should be the platform implementation (backed by the
      // DevTools JavaScript dialog manager), not a JS override.
      const confirmIsNative = await w.webContents.devToolsWebContents!.executeJavaScript(
        'window.confirm.toString().includes("[native code]")'
      );
      expect(confirmIsNative).toBe(true);
    });

    it('steps zoom in and out through the browser zoom presets', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { partition: 'devtools-zoom' } });
      await openDevTools(w);
      const devtools = w.webContents.devToolsWebContents!;
      const zoomPercent = () => Math.round(1.2 ** devtools.getZoomLevel() * 100);
      const zoom = async (method: 'zoomIn' | 'zoomOut' | 'resetZoom', expected: number) => {
        await devtools.executeJavaScript(`InspectorFrontendHost.${method}()`);
        await waitUntil(() => zoomPercent() === expected, { timeout: 2000 }).catch(() => {
          expect(zoomPercent(), `after ${method}()`).toBe(expected);
        });
      };
      expect(zoomPercent()).toBe(100);
      await zoom('zoomIn', 110);
      await zoom('zoomIn', 125);
      await zoom('zoomOut', 110);
      await zoom('zoomOut', 100);
      await zoom('zoomOut', 90);
      await zoom('zoomOut', 80);
      await zoom('resetZoom', 100);
    });

    // Baseline for the setDevToolsWebContents() regression test below: the
    // managed (built-in) DevTools route via InspectableWebContents.
    it('routes context menu requests through the native menu path', async () => {
      const w = new BrowserWindow({ show: false });
      await openDevTools(w);

      const roundTripped = await devToolsMenuRequestRoundTrips(w.webContents.devToolsWebContents!);
      expect(roundTripped).toBe(true);
    });

    describe('with setDevToolsWebContents()', () => {
      async function openCustomDevTools(w: BrowserWindow, devtools: BrowserWindow) {
        await w.loadURL('about:blank');
        w.webContents.setDevToolsWebContents(devtools.webContents);
        const devtoolsReady = once(devtools.webContents, 'dom-ready');
        w.webContents.openDevTools();
        await devtoolsReady;
        await waitUntil(() => devtools.webContents.executeJavaScript('typeof DevToolsAPI !== "undefined"'));
        // The browser only delivers context-menu-closed notifications to a
        // focused frame; focus the frontend like a user interacting with it.
        devtools.webContents.focus();
        await waitUntil(() => devtools.webContents.executeJavaScript('document.hasFocus()'));
      }

      it('uses the native showContextMenuAtPoint implementation', async () => {
        const w = new BrowserWindow({ show: false });
        const devtools = new BrowserWindow({ show: false });
        await openCustomDevTools(w, devtools);

        expect(await usesNativeShowContextMenu(devtools.webContents)).toBe(true);
      });

      // Regression test for https://github.com/electron/electron/issues/51962:
      // a DevTools frontend hosted in a user-provided WebContents must show
      // its popup menus via the native DevTools menu path rather than having
      // the request swallowed by the generic 'context-menu' event plumbing.
      it('routes context menu requests through the native menu path', async () => {
        const w = new BrowserWindow({ show: false });
        const devtools = new BrowserWindow({ show: false });
        await openCustomDevTools(w, devtools);

        let emittedContextMenu = false;
        devtools.webContents.once('context-menu', () => {
          emittedContextMenu = true;
        });

        const roundTripped = await devToolsMenuRequestRoundTrips(devtools.webContents);
        expect(roundTripped).toBe(true);
        expect(emittedContextMenu).toBe(false);
      });
    });
  });

  describe('before-mouse-event event', () => {
    afterEach(closeAllWindows);
    it('can prevent document mouse events', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      await w.loadFile(path.join(fixturesPath, 'pages', 'mouse-events.html'));
      const mouseDown = new Promise((resolve) => {
        ipcMain.once('mousedown', (event, button) => resolve(button));
      });
      w.webContents.once('before-mouse-event', (event, input) => {
        if (input.button === 'left') event.preventDefault();
      });
      w.webContents.sendInputEvent({ type: 'mouseDown', button: 'left', x: 100, y: 100 });
      w.webContents.sendInputEvent({ type: 'mouseDown', button: 'right', x: 100, y: 100 });
      expect(await mouseDown).toBe(2); // Right button is 2
    });

    it('has the correct properties', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));
      const testBeforeMouse = async (opts: Electron.MouseInputEvent) => {
        const p = once(w.webContents, 'before-mouse-event');
        w.webContents.sendInputEvent({
          type: opts.type,
          button: opts.button,
          x: opts.x,
          y: opts.y,
          globalX: opts.globalX,
          globalY: opts.globalY,
          clickCount: opts.clickCount
        });
        const [, input] = await p;

        expect(input.type).toBe(opts.type);
        expect(input.button).toBe(opts.button);
        expect(input.x).toBe(opts.x);
        expect(input.y).toBe(opts.y);
        expect(input.globalX).toBe(opts.globalX);
        expect(input.globalY).toBe(opts.globalY);
        expect(input.clickCount).toBe(opts.clickCount);
      };
      await testBeforeMouse({
        type: 'mouseDown',
        button: 'left',
        x: 100,
        y: 100,
        globalX: 200,
        globalY: 200,
        clickCount: 1
      });
      await testBeforeMouse({
        type: 'mouseUp',
        button: 'right',
        x: 150,
        y: 150,
        globalX: 250,
        globalY: 250,
        clickCount: 2
      });
      await testBeforeMouse({
        type: 'mouseMove',
        button: 'middle',
        x: 200,
        y: 200,
        globalX: 300,
        globalY: 300,
        clickCount: 0
      });
    });
  });

  describe('before-input-event event', () => {
    afterEach(closeAllWindows);
    it('can prevent document keyboard events', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      await w.loadFile(path.join(fixturesPath, 'pages', 'key-events.html'));
      const keyDown = new Promise((resolve) => {
        ipcMain.once('keydown', (event, key) => resolve(key));
      });
      w.webContents.once('before-input-event', (event, input) => {
        if (input.key === 'a') event.preventDefault();
      });
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'a' });
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'b' });
      expect(await keyDown).toBe('b');
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

        const p = once(w.webContents, 'before-input-event') as Promise<[any, Electron.Input]>;
        w.webContents.sendInputEvent({
          type: opts.type,
          keyCode: opts.keyCode,
          modifiers: modifiers as any
        });
        const [, input] = await p;

        expect(input.type).toBe(opts.type);
        expect(input.key).toBe(opts.key);
        expect(input.code).toBe(opts.code);
        expect(input.isAutoRepeat).toBe(opts.isAutoRepeat);
        expect(input.shift).toBe(opts.shift);
        expect(input.control).toBe(opts.control);
        expect(input.alt).toBe(opts.alt);
        expect(input.meta).toBe(opts.meta);
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

        const [, zoomDirection] = (await once(w.webContents, 'zoom-changed')) as [any, string];
        expect(zoomDirection).toBe('in');
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

        const [, zoomDirection] = (await once(w.webContents, 'zoom-changed')) as [any, string];
        expect(zoomDirection).toBe('out');
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
      const keydown = once(ipcMain, 'keydown');
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'A' });
      const [, key, code, keyCode, shiftKey, ctrlKey, altKey] = await keydown;
      expect(key).toBe('a');
      expect(code).toBe('KeyA');
      expect(keyCode).toBe(65);
      expect(shiftKey).toBe(false);
      expect(ctrlKey).toBe(false);
      expect(altKey).toBe(false);
    });

    it('can send keydown events with modifiers', async () => {
      const keydown = once(ipcMain, 'keydown');
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Z', modifiers: ['shift', 'ctrl'] });
      const [, key, code, keyCode, shiftKey, ctrlKey, altKey] = await keydown;
      expect(key).toBe('Z');
      expect(code).toBe('KeyZ');
      expect(keyCode).toBe(90);
      expect(shiftKey).toBe(true);
      expect(ctrlKey).toBe(true);
      expect(altKey).toBe(false);
    });

    it('can send keydown events with special keys', async () => {
      const keydown = once(ipcMain, 'keydown');
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Tab', modifiers: ['alt'] });
      const [, key, code, keyCode, shiftKey, ctrlKey, altKey] = await keydown;
      expect(key).toBe('Tab');
      expect(code).toBe('Tab');
      expect(keyCode).toBe(9);
      expect(shiftKey).toBe(false);
      expect(ctrlKey).toBe(false);
      expect(altKey).toBe(true);
    });

    it('can send char events', async () => {
      const keypress = once(ipcMain, 'keypress');
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'A' });
      w.webContents.sendInputEvent({ type: 'char', keyCode: 'A' });
      const [, key, code, keyCode, shiftKey, ctrlKey, altKey] = await keypress;
      expect(key).toBe('a');
      expect(code).toBe('KeyA');
      expect(keyCode).toBe(65);
      expect(shiftKey).toBe(false);
      expect(ctrlKey).toBe(false);
      expect(altKey).toBe(false);
    });

    it('can correctly convert accelerators to key codes', async () => {
      await w.webContents.executeJavaScript('document.getElementById("input").focus()');
      const keyup = once(ipcMain, 'keyup');
      w.webContents.sendInputEvent({ keyCode: 'Plus', type: 'char' });
      w.webContents.sendInputEvent({ keyCode: 'Space', type: 'char' });
      w.webContents.sendInputEvent({ keyCode: 'Plus', type: 'char' });
      w.webContents.sendInputEvent({ keyCode: 'Space', type: 'char' });
      w.webContents.sendInputEvent({ keyCode: 'Plus', type: 'char' });
      w.webContents.sendInputEvent({ keyCode: 'Plus', type: 'keyUp' });

      await keyup;
      const inputText = await w.webContents.executeJavaScript('document.getElementById("input").value');
      expect(inputText).toBe('+ + +');
    });

    it('can send char events with modifiers', async () => {
      const keypress = once(ipcMain, 'keypress');
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Z' });
      w.webContents.sendInputEvent({ type: 'char', keyCode: 'Z', modifiers: ['shift', 'ctrl'] });
      const [, key, code, keyCode, shiftKey, ctrlKey, altKey] = await keypress;
      expect(key).toBe('Z');
      expect(code).toBe('KeyZ');
      expect(keyCode).toBe(90);
      expect(shiftKey).toBe(true);
      expect(ctrlKey).toBe(true);
      expect(altKey).toBe(false);
    });
  });

  describe('insertCSS', () => {
    afterEach(closeAllWindows);
    it('supports inserting CSS', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadURL('about:blank');
      await w.webContents.insertCSS('body { background-repeat: round; }');
      const result = await w.webContents.executeJavaScript(
        'window.getComputedStyle(document.body).getPropertyValue("background-repeat")'
      );
      expect(result).toBe('round');
    });

    it('supports removing inserted CSS', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadURL('about:blank');
      const key = await w.webContents.insertCSS('body { background-repeat: round; }');
      await w.webContents.removeInsertedCSS(key);
      const result = await w.webContents.executeJavaScript(
        'window.getComputedStyle(document.body).getPropertyValue("background-repeat")'
      );
      expect(result).toBe('repeat');
    });
  });

  describe('inspectElement()', () => {
    afterEach(closeAllWindows);
    it('supports inspecting an element in the devtools', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadURL('about:blank');
      const event = once(w.webContents, 'devtools-opened');
      w.webContents.inspectElement(10, 10);
      await event;
    });
  });

  describe('startDrag({file, icon})', () => {
    afterEach(closeAllWindows);
    it('throws errors for a missing file or a missing/empty icon', () => {
      const w = new BrowserWindow({ show: false });
      expect(() => {
        w.webContents.startDrag({ icon: path.join(fixturesPath, 'assets', 'logo.png') } as any);
      }).toThrow("Must specify either 'file' or 'files' option");

      expect(() => {
        w.webContents.startDrag({ file: import.meta.filename } as any);
      }).toThrow("'icon' parameter is required");

      expect(() => {
        w.webContents.startDrag({ file: import.meta.filename, icon: path.join(fixturesPath, 'blank.png') });
      }).toThrow(/Failed to load image from path (.+)/);
    });
  });

  describe('focus APIs', { tags: ['serial'] }, () => {
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
        expect(currentFocused).toBe(true);
        expect(childFocused).toBe(false);
      });

      it('does not crash when focusing a WebView webContents', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            webviewTag: true
          }
        });

        w.show();
        await w.loadURL('data:text/html,<webview src="data:text/html,hi"></webview>');

        const wc = webContents.getAllWebContents().find((wc) => wc.getType() === 'webview')!;
        expect(() => wc.focus()).not.toThrow();
      });
    });

    const moveFocusToDevTools = async (win: BrowserWindow) => {
      const devToolsOpened = once(win.webContents, 'devtools-opened');
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
        const focusPromise = once(w.webContents, 'focus');
        w.webContents.focus();
        await focusPromise;
      });
    });

    describe('blur event', () => {
      afterEach(closeAllWindows);
      it('is triggered when web contents is blurred', async () => {
        const w = new BrowserWindow({ show: true });
        await w.loadURL('about:blank');
        w.webContents.focus();
        const blurPromise = once(w.webContents, 'blur');
        await moveFocusToDevTools(w);
        await blurPromise;
      });
    });

    describe('focusOnNavigation webPreference', () => {
      afterEach(closeAllWindows);

      it('focuses the webContents on navigation by default', async () => {
        const w = new BrowserWindow({ show: true });
        await once(w, 'focus');
        await w.loadURL('about:blank');
        await moveFocusToDevTools(w);
        expect(w.webContents.isFocused()).toBe(false);
        await w.loadURL('data:text/html,<body>test</body>');
        expect(w.webContents.isFocused()).toBe(true);
      });

      it('does not focus the webContents on navigation when focusOnNavigation is false', async () => {
        const w = new BrowserWindow({
          show: true,
          webPreferences: {
            focusOnNavigation: false
          }
        });
        await once(w, 'focus');
        await w.loadURL('about:blank');
        await moveFocusToDevTools(w);
        expect(w.webContents.isFocused()).toBe(false);
        await w.loadURL('data:text/html,<body>test</body>');
        expect(w.webContents.isFocused()).toBe(false);
      });
    });
  });

  describe('getOSProcessId()', () => {
    afterEach(closeAllWindows);
    it('returns a valid process id', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');
      expect(w.webContents.getOSProcessId()).toBeGreaterThan(0);
    });

    it('returns 0 before a renderer has been assigned', () => {
      const w = new BrowserWindow({ show: false, webPreferences: { sandbox: false } });
      expect(w.webContents.getOSProcessId()).toBe(0);
    });

    describe('pre-warmed renderer', () => {
      // Chromium drops a spare renderer to background priority as soon as it
      // launches, since it hosts no visible page;
      // on Windows that is IDLE_PRIORITY_CLASS.
      // While a parallel spec worker keeps every CPU busy,
      // the spare can then take longer than the 10 s wait below to start up,
      // and is still missing from app.getAppMetrics() when the wait ends.
      // These tests check which window gets the spare, not its priority.
      const startAppWithoutBackgrounding = (extraArgs: string[] = []) =>
        startRemoteControlApp(['--disable-renderer-backgrounding', ...extraArgs]);

      const tabPidsAroundFirstWindow = (
        rc: Awaited<ReturnType<typeof startRemoteControlApp>>,
        firstPrefs: Electron.WebPreferences,
        { expectReplacement = false } = {}
      ) => {
        const options = { prefs: firstPrefs, expectReplacement };
        return rc.remotely(async ({ prefs, expectReplacement }: typeof options) => {
          const { app, BrowserWindow, session } = require('electron');
          const tabPids = () =>
            app
              .getAppMetrics()
              .filter((m: Electron.ProcessMetric) => m.type === 'Tab')
              .map((m: Electron.ProcessMetric) => m.pid);
          const waitForTabPids = async (done: (pids: number[]) => boolean) => {
            const deadline = Date.now() + 10000;
            while (!done(tabPids()) && Date.now() < deadline) {
              await new Promise((resolve) => global.setTimeout(resolve, 20));
            }
          };
          session.defaultSession.getUserAgent();
          await waitForTabPids((pids) => pids.length > 0);
          const warmed = tabPids();
          const first = new BrowserWindow({ show: false, webPreferences: prefs });
          const firstPidBeforeLoad = first.webContents.getOSProcessId();
          await first.loadURL('about:blank');
          if (expectReplacement) {
            // Taking the warmed renderer starts a replacement spare.
            const firstPid = first.webContents.getOSProcessId();
            await waitForTabPids((pids) => pids.some((pid) => pid !== firstPid));
          } else {
            // Leave time for a replacement spare that should not exist.
            await new Promise((resolve) => global.setTimeout(resolve, 500));
          }
          const tabsBeforeSecond = tabPids();
          const second = new BrowserWindow({ show: false });
          const secondPidBeforeLoad = second.webContents.getOSProcessId();
          await second.loadURL('about:blank');
          const result = {
            warmed,
            firstPidBeforeLoad,
            firstPid: first.webContents.getOSProcessId(),
            tabsBeforeSecond,
            secondPidBeforeLoad,
            secondPid: second.webContents.getOSProcessId(),
            tabsAfter: tabPids()
          };
          first.destroy();
          second.destroy();
          return result;
        }, options);
      };

      it('is handed to the first sandboxed window only', async () => {
        const rc = await startAppWithoutBackgrounding();
        const r = await tabPidsAroundFirstWindow(rc, {});
        expect(r.warmed).toHaveLength(1);
        expect(r.firstPidBeforeLoad).toBe(r.warmed[0]);
        expect(r.firstPid).toBe(r.warmed[0]);
        expect(r.secondPidBeforeLoad).toBe(0);
      });

      it('is discarded when the first window cannot use it', async () => {
        const rc = await startAppWithoutBackgrounding();
        const r = await tabPidsAroundFirstWindow(rc, { sandbox: false });
        expect(r.warmed).toHaveLength(1);
        expect(r.firstPidBeforeLoad).toBe(0);
        expect(r.firstPid).not.toBe(r.warmed[0]);
        expect(r.tabsAfter).not.toContain(r.warmed[0]);
        expect(r.secondPidBeforeLoad).toBe(0);
      });

      it('keeps serving later windows with SpareRendererForSitePerProcess enabled', async () => {
        const rc = await startAppWithoutBackgrounding(['--enable-features=SpareRendererForSitePerProcess']);
        const r = await tabPidsAroundFirstWindow(rc, {}, { expectReplacement: true });
        expect(r.warmed).toHaveLength(1);
        expect(r.firstPidBeforeLoad).toBe(r.warmed[0]);
        expect(r.firstPid).toBe(r.warmed[0]);
        expect(r.secondPidBeforeLoad).not.toBe(0);
        expect(r.secondPidBeforeLoad).not.toBe(r.firstPid);
        expect(r.tabsBeforeSecond).toContain(r.secondPidBeforeLoad);
        expect(r.secondPid).toBe(r.secondPidBeforeLoad);
      });
    });
  });

  describe('clone()', () => {
    afterEach(closeAllWindows);
    afterEach(() => {
      webContents.getAllWebContents().forEach((wc) => {
        wc.destroy();
      });
    });

    let server: http.Server;
    let serverUrl: string;

    beforeAll(async () => {
      server = http.createServer((req, res) => {
        res.setHeader('Content-Type', 'text/html');
        res.end('<title>clone</title>');
      });
      serverUrl = (await listen(server)).url;
    });

    afterAll(() => {
      server.close();
    });

    it('web-contents-created event will be emitted for cloned WebContents', async () => {
      const w = new BrowserWindow({
        show: false
      });

      const webContentsCreated = once(app, 'web-contents-created') as Promise<[any, WebContents]>;
      const clonedContents = w.webContents.clone();
      const [, createdContents] = await webContentsCreated;

      expect(clonedContents).toBe(createdContents);
      expect(createdContents).not.toBe(w.webContents);
    });

    it('clones a WebContents instance', async () => {
      const w = new BrowserWindow({
        show: false
      });
      const clonedContents = w.webContents.clone();
      expect(clonedContents).not.toBeUndefined();
      expect(clonedContents).not.toBe(w.webContents);
      await clonedContents.loadURL('about:blank');
      expect(clonedContents.getOSProcessId()).toBeTypeOf('number');
      expect(clonedContents.getOSProcessId()).toBeGreaterThan(0);
    });

    it('cloned and original WebContents have different process IDs when loading same URL', async () => {
      const w = new BrowserWindow({
        show: false
      });
      const clonedContents = w.webContents.clone();
      expect(clonedContents).not.toBeUndefined();

      // Load the same URL in both original and cloned WebContents
      await w.webContents.loadURL(serverUrl);
      await clonedContents.loadURL(serverUrl);

      // The clone shares the original's renderer process, so they have the same PID
      const originalPID = w.webContents.getOSProcessId();
      const clonedPID = clonedContents.getOSProcessId();

      expect(originalPID).toBeGreaterThan(0);
      expect(clonedPID).toBeGreaterThan(0);
      expect(originalPID).toBe(clonedPID);

      // Create a new BrowserWindow with same URL
      const w2 = new BrowserWindow({
        show: false
      });
      await w2.webContents.loadURL(serverUrl);
      const newWindowPID = w2.webContents.getOSProcessId();

      // New window should also have a different process ID
      expect(newWindowPID).toBeGreaterThan(0);
      expect(newWindowPID).not.toBe(originalPID);
    });

    it('node integration and ipc message work in both original and cloned WebContents', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          sandbox: false,
          nodeIntegration: true,
          contextIsolation: false
        }
      });

      // Load a simple page
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));

      // Keep track of messages received
      let message = '';
      const onOriginalMessage = (_event: any, msg: string) => {
        message = msg;
      };
      const onClonedMessage = (_event: any, msg: string) => {
        message = msg;
      };

      ipcMain.once('test-node-integration-original', onOriginalMessage);

      // Test original WebContents can use require('electron') and send ipc
      await w.webContents.executeJavaScript(`
        const { ipcRenderer } = require('electron');
        ipcRenderer.send('test-node-integration-original', 'message from original');
      `);

      // Wait for message to be processed
      await setTimeout(100);
      expect(message).toBe('message from original');
      const clonedContents = w.webContents.clone();
      expect(clonedContents).not.toBeUndefined();

      await clonedContents.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));

      ipcMain.once('test-node-integration-cloned', onClonedMessage);

      // Test cloned WebContents can use require('electron') and send ipc
      await clonedContents.executeJavaScript(`
        const { ipcRenderer } = require('electron');
        ipcRenderer.send('test-node-integration-cloned', 'message from cloned');
      `);

      // Wait for message to be processed
      await setTimeout(100);
      expect(message).toBe('message from cloned');
    });

    it('cloned WebContents has independent lifecycle from original', async () => {
      const w = new BrowserWindow({
        show: false
      });

      // Load URL to ensure WebContents is initialized
      await w.loadURL('about:blank');

      // Clone the WebContents
      const clonedContents = w.webContents.clone();
      expect(clonedContents).not.toBeUndefined();

      await clonedContents.loadURL('about:blank');

      // Both should not be destroyed initially
      expect(w.webContents.isDestroyed()).toBe(false);
      expect(clonedContents.isDestroyed()).toBe(false);

      const origWebContents = w.webContents;

      // Destroy the original WebContents
      w.webContents.destroy();

      await setTimeout();

      // Original should be destroyed, but cloned should still be alive
      expect(origWebContents.isDestroyed()).toBe(true);
      expect(clonedContents.isDestroyed()).toBe(false);

      // Cloned WebContents should still be usable
      const url = clonedContents.getURL();
      expect(url).toBe('about:blank');
    });
  });

  describe('getMediaSourceId()', () => {
    afterEach(closeAllWindows);
    let server: http.Server;
    let serverUrl: string;

    beforeAll(async () => {
      server = http.createServer((req, res) => {
        res.setHeader('Content-Type', 'text/html');
        res.end('');
      });
      serverUrl = (await listen(server)).url;
    });

    afterAll(() => {
      server.close();
    });

    it('returns a stream id that can be used by the registered requester', async () => {
      const sourceWindow = new BrowserWindow({ show: false });
      const requesterWindow = new BrowserWindow({ show: false });
      await Promise.all([sourceWindow.loadURL(serverUrl), requesterWindow.loadURL(serverUrl)]);

      const streamId = sourceWindow.webContents.getMediaSourceId(requesterWindow.webContents);
      const { ok, message, origin, videoTrackCount } = await captureWithTabSourceId(requesterWindow, streamId);

      expect(streamId).toBeTypeOf('string');
      expect(streamId).not.toHaveLength(0);
      expect(ok, message).toBe(true);
      expect(origin).toBe(new url.URL(serverUrl).origin);
      expect(videoTrackCount).toBe(1);
    });
  });

  describe('getOrCreateDevToolsTargetId()', () => {
    afterEach(closeAllWindows);
    it('returns the devtools target id', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');
      const devToolsId = w.webContents.getOrCreateDevToolsTargetId();
      expect(devToolsId).toBeTypeOf('string');
      expect(devToolsId).not.toHaveLength(0);
      // Verify it's the inverse of fromDevToolsTargetId
      expect(webContents.fromDevToolsTargetId(devToolsId)).toBe(w.webContents);
    });
  });

  describe('userAgent APIs', () => {
    afterEach(closeAllWindows);
    it('is not empty by default', () => {
      const w = new BrowserWindow({ show: false });
      const userAgent = w.webContents.getUserAgent();
      expect(userAgent).toBeTypeOf('string');
      expect(userAgent).not.toHaveLength(0);
    });

    it('can set the user agent (functions)', () => {
      const w = new BrowserWindow({ show: false });
      const userAgent = w.webContents.getUserAgent();

      w.webContents.setUserAgent('my-user-agent');
      expect(w.webContents.getUserAgent()).toBe('my-user-agent');

      w.webContents.setUserAgent(userAgent);
      expect(w.webContents.getUserAgent()).toBe(userAgent);
    });

    it('can set the user agent (properties)', () => {
      const w = new BrowserWindow({ show: false });
      const userAgent = w.webContents.userAgent;

      w.webContents.userAgent = 'my-user-agent';
      expect(w.webContents.userAgent).toBe('my-user-agent');

      w.webContents.userAgent = userAgent;
      expect(w.webContents.userAgent).toBe(userAgent);
    });
  });

  describe('audioMuted APIs', () => {
    afterEach(closeAllWindows);
    it('can set the audio mute level (functions)', () => {
      const w = new BrowserWindow({ show: false });

      w.webContents.setAudioMuted(true);
      expect(w.webContents.isAudioMuted()).toBe(true);

      w.webContents.setAudioMuted(false);
      expect(w.webContents.isAudioMuted()).toBe(false);
    });

    it('can set the audio mute level (functions)', () => {
      const w = new BrowserWindow({ show: false });

      w.webContents.audioMuted = true;
      expect(w.webContents.audioMuted).toBe(true);

      w.webContents.audioMuted = false;
      expect(w.webContents.audioMuted).toBe(false);
    });
  });

  describe('caretBrowsingEnabled', () => {
    afterEach(closeAllWindows);

    const platformCaretBrowsing = () =>
      process._linkedBinding('electron_common_testing').isPlatformCaretBrowsingEnabled();

    it('defaults to false', () => {
      const w = new BrowserWindow({ show: false });
      expect(w.webContents.caretBrowsingEnabled).toBe(false);
    });

    it('can be toggled via the property', () => {
      const w = new BrowserWindow({ show: false });

      w.webContents.caretBrowsingEnabled = true;
      expect(w.webContents.caretBrowsingEnabled).toBe(true);

      w.webContents.caretBrowsingEnabled = false;
      expect(w.webContents.caretBrowsingEnabled).toBe(false);
    });

    it('stays enabled when set to the same value repeatedly', () => {
      const w = new BrowserWindow({ show: false });

      w.webContents.caretBrowsingEnabled = true;
      w.webContents.caretBrowsingEnabled = true;
      expect(w.webContents.caretBrowsingEnabled).toBe(true);
    });

    // Test that repeated calls aren't incorrectly incrementing the underlying refcount
    it('can be enabled repeatedly without poisoning future calls', () => {
      const w = new BrowserWindow({ show: false });

      w.webContents.caretBrowsingEnabled = true;
      w.webContents.caretBrowsingEnabled = true;
      w.webContents.caretBrowsingEnabled = true;

      w.webContents.caretBrowsingEnabled = false;
      expect(w.webContents.caretBrowsingEnabled).toBe(false);
    });

    // Test that repeated calls aren't incorrectly decrementing the underlying refcount
    it('can be disabled repeatedly without poisoning future calls', () => {
      const w = new BrowserWindow({ show: false });

      w.webContents.caretBrowsingEnabled = true;

      w.webContents.caretBrowsingEnabled = false;
      w.webContents.caretBrowsingEnabled = false;
      w.webContents.caretBrowsingEnabled = false;

      w.webContents.caretBrowsingEnabled = true;
      expect(w.webContents.caretBrowsingEnabled).toBe(true);
    });

    it('persists across navigation', async () => {
      const w = new BrowserWindow({ show: false });
      w.webContents.caretBrowsingEnabled = true;

      await w.loadURL('about:blank');

      expect(w.webContents.caretBrowsingEnabled).toBe(true);
    });

    it('tracks each WebContents independently', () => {
      const w1 = new BrowserWindow({ show: false });
      const w2 = new BrowserWindow({ show: false });

      w1.webContents.caretBrowsingEnabled = true;
      w2.webContents.caretBrowsingEnabled = true;

      w1.webContents.caretBrowsingEnabled = false;

      expect(w1.webContents.caretBrowsingEnabled).toBe(false);
      expect(w2.webContents.caretBrowsingEnabled).toBe(true);

      w2.webContents.caretBrowsingEnabled = false;
      expect(w2.webContents.caretBrowsingEnabled).toBe(false);
    });

    it('can be enabled again after a WebContents is destroyed with it enabled', async () => {
      const w1 = new BrowserWindow({ show: false });
      w1.webContents.caretBrowsingEnabled = true;

      const destroyed = once(w1.webContents, 'destroyed');
      w1.close();
      await destroyed;

      const w2 = new BrowserWindow({ show: false });
      w2.webContents.caretBrowsingEnabled = true;
      expect(w2.webContents.caretBrowsingEnabled).toBe(true);

      w2.webContents.caretBrowsingEnabled = false;
      expect(w2.webContents.caretBrowsingEnabled).toBe(false);
    });

    // These depend on a binding that is only available when DCHECK_IS_ON.
    ifdescribe(isTestingBindingAvailable())('process-wide accessibility notification', () => {
      it('stays on while another WebContents still has caret browsing enabled', () => {
        const w1 = new BrowserWindow({ show: false });
        const w2 = new BrowserWindow({ show: false });

        expect(platformCaretBrowsing()).toBe(false);

        w1.webContents.caretBrowsingEnabled = true;
        w2.webContents.caretBrowsingEnabled = true;
        expect(platformCaretBrowsing()).toBe(true);

        w1.webContents.caretBrowsingEnabled = false;
        expect(platformCaretBrowsing()).toBe(true);

        w2.webContents.caretBrowsingEnabled = false;
        expect(platformCaretBrowsing()).toBe(false);
      });

      it('takes at most one reference per WebContents however often it is set', () => {
        const w = new BrowserWindow({ show: false });

        expect(platformCaretBrowsing()).toBe(false);

        w.webContents.caretBrowsingEnabled = true;
        w.webContents.caretBrowsingEnabled = true;
        w.webContents.caretBrowsingEnabled = true;
        expect(platformCaretBrowsing()).toBe(true);

        w.webContents.caretBrowsingEnabled = false;
        expect(platformCaretBrowsing()).toBe(false);
      });

      it('is withdrawn once an enabled WebContents is destroyed', async () => {
        const w = new BrowserWindow({ show: false });
        const contents = w.webContents;
        contents.caretBrowsingEnabled = true;
        expect(platformCaretBrowsing()).toBe(true);

        const destroyed = once(contents, 'destroyed');
        w.close();
        await destroyed;

        expect(platformCaretBrowsing()).toBe(false);
      });

      it('is not left on by a will-destroy listener that re-enables caret browsing', async () => {
        const contents = (webContents as typeof ElectronInternal.WebContents).create();

        expect(platformCaretBrowsing()).toBe(false);

        // will-destroy is emitted from the destructor, after it has released
        // this WebContents' reference, while the wrapper is still dispatchable.
        (contents as any).on('will-destroy', () => {
          contents.caretBrowsingEnabled = true;
        });

        const destroyed = once(contents, 'destroyed');
        contents.destroy();
        await destroyed;

        expect(platformCaretBrowsing()).toBe(false);
      });
    });

    describe('renderer-side caret movement', () => {
      const pressRight = (w: BrowserWindow, times: number) => {
        for (let i = 0; i < times; i++) {
          w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Right' });
          w.webContents.sendInputEvent({ type: 'keyUp', keyCode: 'Right' });
        }
      };

      // Blink needs a starting selection to move away from
      const collapseSelectionToStart = (w: BrowserWindow) =>
        w.webContents.executeJavaScript(`
        {
          const textNode = document.getElementById('prose').firstChild;
          getSelection().collapse(textNode, 0);
        }
      `);

      it('moves the caret through non-editable text only while enabled', async () => {
        const w = new BrowserWindow({ show: true });
        await w.loadFile(path.join(fixturesPath, 'pages', 'caret-browsing.html'));
        w.focus();
        w.webContents.focus();

        // Initialize to something neither phase expects so an unrun poll can't pass
        let offset = -1;
        // Renderer preferences propagate asynchronously, so poll for each state
        const pollForOffset = (expected: number) =>
          waitUntil(
            async () => {
              await collapseSelectionToStart(w);
              pressRight(w, 3);
              offset = await w.webContents.executeJavaScript('getSelection().anchorOffset');
              return offset === expected;
            },
            { rate: 100, timeout: 3000 }
          );

        w.webContents.caretBrowsingEnabled = true;
        await pollForOffset(3);
        expect(offset).toBe(3);

        // Now that the input pipeline is known to reach the page, a caret that
        // stays put is attributable to the preference
        w.webContents.caretBrowsingEnabled = false;
        await pollForOffset(0);
        expect(offset).toBe(0);
      });
    });

    describe('guest preference inheritance', () => {
      it('inherits caretBrowsingEnabled in a guest webview', async () => {
        const w = new BrowserWindow({ show: false, webPreferences: { webviewTag: true } });
        w.webContents.caretBrowsingEnabled = true;

        const created = once(app, 'web-contents-created') as Promise<[any, WebContents]>;
        w.loadURL('data:text/html,<webview src="data:text/html,hi"></webview>');
        const [, guest] = await created;

        expect(guest.getType()).toBe('webview');
        expect(guest.caretBrowsingEnabled).toBe(true);
      });

      ifit(isTestingBindingAvailable())(
        'releases exactly one reference when an inherited preference is disabled',
        async () => {
          const w = new BrowserWindow({ show: false, webPreferences: { webviewTag: true } });
          w.webContents.caretBrowsingEnabled = true;

          const created = once(app, 'web-contents-created') as Promise<[any, WebContents]>;
          w.loadURL('data:text/html,<webview src="data:text/html,hi"></webview>');
          const [, guest] = await created;

          expect(guest.caretBrowsingEnabled).toBe(true);

          // An inherited preference has to come with its own reference, otherwise
          // the guest keeps caret browsing on with nothing holding the count up.
          w.webContents.caretBrowsingEnabled = false;
          expect(platformCaretBrowsing()).toBe(true);

          guest.caretBrowsingEnabled = false;
          expect(platformCaretBrowsing()).toBe(false);
        }
      );

      ifit(isTestingBindingAvailable())('releases an inherited reference when the guest is destroyed', async () => {
        const w = new BrowserWindow({ show: false, webPreferences: { webviewTag: true } });
        w.webContents.caretBrowsingEnabled = true;

        const created = once(app, 'web-contents-created') as Promise<[any, WebContents]>;
        w.loadURL('data:text/html,<webview src="data:text/html,hi"></webview>');
        const [, guest] = await created;

        expect(guest.caretBrowsingEnabled).toBe(true);
        expect(platformCaretBrowsing()).toBe(true);

        // An attached guest's WebContents is owned by its embedder frame, so the
        // wrapper holding the reference is only deleted at garbage collection.
        const destroyed = once(guest, 'destroyed');
        w.close();
        await destroyed;

        expect(platformCaretBrowsing()).toBe(false);
      });
    });
  });

  describe('zoom api', () => {
    const hostZoomMap: Record<string, number> = {
      host1: 0.3,
      host2: 0.7,
      host3: 0.2
    };

    beforeAll(() => {
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

    afterAll(() => {
      const protocol = session.defaultSession.protocol;
      protocol.unregisterProtocol(standardScheme);
    });

    afterEach(closeAllWindows);

    it('throws on an invalid zoomFactor', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');

      expect(() => {
        w.webContents.setZoomFactor(0.0);
      }).toThrow(/'zoomFactor' must be a double greater than 0.0/);

      expect(() => {
        w.webContents.setZoomFactor(-2.0);
      }).toThrow(/'zoomFactor' must be a double greater than 0.0/);
    });

    it('can set the correct zoom level (functions)', async () => {
      const w = new BrowserWindow({ show: false });
      try {
        await w.loadURL('about:blank');
        const zoomLevel = w.webContents.getZoomLevel();
        expect(zoomLevel).toEqual(0.0);
        w.webContents.setZoomLevel(0.5);
        const newZoomLevel = w.webContents.getZoomLevel();
        expect(newZoomLevel).toEqual(0.5);
      } finally {
        w.webContents.setZoomLevel(0);
      }
    });

    it('keeps the zoom level of a file: page across a fragment navigation', async () => {
      const w = new BrowserWindow({ show: false });
      try {
        await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
        w.webContents.setZoomLevel(2);
        await w.webContents.executeJavaScript("location.hash = 'section'; new Promise(r => setTimeout(r))");
        expect(w.webContents.getURL()).toMatch(/#section$/);
        expect(w.webContents.getZoomLevel()).toBe(2);
      } finally {
        w.webContents.setZoomLevel(0);
      }
    });

    it('can set the correct zoom level (properties)', async () => {
      const w = new BrowserWindow({ show: false });
      try {
        await w.loadURL('about:blank');
        const zoomLevel = w.webContents.zoomLevel;
        expect(zoomLevel).toEqual(0.0);
        w.webContents.zoomLevel = 0.5;
        const newZoomLevel = w.webContents.zoomLevel;
        expect(newZoomLevel).toEqual(0.5);
      } finally {
        w.webContents.zoomLevel = 0;
      }
    });

    it('can set the correct zoom factor (functions)', async () => {
      const w = new BrowserWindow({ show: false });
      try {
        await w.loadURL('about:blank');
        const zoomFactor = w.webContents.getZoomFactor();
        expect(zoomFactor).toEqual(1.0);

        w.webContents.setZoomFactor(0.5);
        const newZoomFactor = w.webContents.getZoomFactor();
        expect(newZoomFactor).toEqual(0.5);
      } finally {
        w.webContents.setZoomFactor(1.0);
      }
    });

    it('can set the correct zoom factor (properties)', async () => {
      const w = new BrowserWindow({ show: false });
      try {
        await w.loadURL('about:blank');
        const zoomFactor = w.webContents.zoomFactor;
        expect(zoomFactor).toEqual(1.0);

        w.webContents.zoomFactor = 0.5;
        const newZoomFactor = w.webContents.zoomFactor;
        expect(newZoomFactor).toEqual(0.5);
      } finally {
        w.webContents.zoomFactor = 1.0;
      }
    });

    it('can persist zoom level across navigation', () =>
      new Promise<void>((resolve, reject) => {
        const done = (error?: unknown) => (error ? reject(error) : resolve());
        const w = new BrowserWindow({
          show: false,
          webPreferences: { nodeIntegration: true, contextIsolation: false }
        });
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
            expect(zoomLevel).toBe(expectedZoomLevel);
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
            expect(zoomLevel).toBe(expectedZoomLevel);
            finalNavigation = true;
            w.webContents.goBack();
          } catch (e) {
            done(e);
          }
        });
        w.loadURL(`${standardScheme}://host1`);
      }));

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
      expect(zoomLevel1).toBe(hostZoomMap.host3);

      const zoomLevel2 = w2.webContents.zoomLevel;
      expect(zoomLevel1).toBe(zoomLevel2);
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
      expect(zoomLevel1).toBe(hostZoomMap.host3);

      const zoomLevel2 = w2.webContents.zoomLevel;
      expect(zoomLevel2).toBe(0);
      expect(zoomLevel1).not.toBe(zoomLevel2);
    });

    it('can persist when it contains iframe', () =>
      new Promise<void>((resolve, reject) => {
        const done = (error?: unknown) => (error ? reject(error) : resolve());
        const w = new BrowserWindow({ show: false });
        const server = http.createServer((req, res) => {
          setTimeout(200).then(() => {
            res.end();
          });
        });
        defer(() => {
          server.close();
        });
        listen(server).then(({ url }) => {
          const content = `<iframe src=${url}></iframe>`;
          w.webContents.on('did-frame-finish-load', (e, isMainFrame) => {
            if (!isMainFrame) {
              try {
                const zoomLevel = w.webContents.zoomLevel;
                expect(zoomLevel).toBe(2.0);

                w.webContents.zoomLevel = 0;
                done();
              } catch (e) {
                done(e);
              }
            }
          });
          w.webContents.on('dom-ready', () => {
            w.webContents.zoomLevel = 2.0;
          });
          w.loadURL(`data:text/html,${content}`);
        });
      }));

    it('cannot propagate when used with webframe', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      const w2 = new BrowserWindow({ show: false });

      const temporaryZoomSet = once(ipcMain, 'temporary-zoom-set');
      w.loadFile(path.join(fixturesPath, 'pages', 'webframe-zoom.html'));
      await temporaryZoomSet;

      const finalZoomLevel = w.webContents.getZoomLevel();
      await w2.loadFile(path.join(fixturesPath, 'pages', 'c.html'));
      const zoomLevel1 = w.webContents.zoomLevel;
      const zoomLevel2 = w2.webContents.zoomLevel;

      w2.setClosable(true);
      w2.close();

      expect(zoomLevel1).toBe(finalZoomLevel);
      expect(zoomLevel2).toBe(0);
      expect(zoomLevel1).not.toBe(zoomLevel2);
    });

    describe('with unique domains', () => {
      let server: http.Server;
      let serverUrl: string;
      let crossSiteUrl: string;

      beforeAll(async () => {
        server = http.createServer((req, res) => {
          setTimeout().then(() => res.end('hey'));
        });
        serverUrl = (await listen(server)).url;
        crossSiteUrl = serverUrl.replace('127.0.0.1', 'localhost');
      });

      afterAll(() => {
        server.close();
      });

      it('cannot persist zoom level after navigation with webFrame', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: { nodeIntegration: true, contextIsolation: false }
        });
        const source = `
          const {ipcRenderer, webFrame} = require('electron')
          webFrame.setZoomLevel(0.6)
          ipcRenderer.send('zoom-level-set', webFrame.getZoomLevel())
        `;
        const zoomLevelPromise = once(ipcMain, 'zoom-level-set');
        await w.loadURL(serverUrl);
        await w.webContents.executeJavaScript(source);
        let [, zoomLevel] = await zoomLevelPromise;
        expect(zoomLevel).toBe(0.6);
        const loadPromise = once(w.webContents, 'did-finish-load');
        await w.loadURL(crossSiteUrl);
        await loadPromise;
        zoomLevel = w.webContents.zoomLevel;
        expect(zoomLevel).toBe(0);
      });
    });
  });

  describe('webContents.setVisualZoomLevelLimits()', () => {
    afterEach(closeAllWindows);

    const pageScaleAfterPinchTo = async (w: BrowserWindow, scale: number) => {
      await w.webContents.debugger.sendCommand('Emulation.setPageScaleFactor', { pageScaleFactor: scale });
      return w.webContents.executeJavaScript('window.visualViewport.scale');
    };

    it('keeps the limits across navigations', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');
      w.webContents.debugger.attach();
      expect(await pageScaleAfterPinchTo(w, 2)).toBe(1);

      await w.webContents.setVisualZoomLevelLimits(1, 3);
      expect(await pageScaleAfterPinchTo(w, 2)).toBe(2);

      await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
      expect(await pageScaleAfterPinchTo(w, 2)).toBe(2);
      expect(await pageScaleAfterPinchTo(w, 5)).toBe(3);
    });

    it('rejects invalid limits', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');
      await expect(w.webContents.setVisualZoomLevelLimits(0, 3)).rejects.toThrow(/positive numbers/);
      await expect(w.webContents.setVisualZoomLevelLimits(3, 1)).rejects.toThrow(/positive numbers/);
      await expect(w.webContents.setVisualZoomLevelLimits(Number.NaN, 3)).rejects.toThrow(/positive numbers/);
    });
  });

  describe('zoom limits', () => {
    afterEach(closeAllWindows);

    it('clamps setZoomLevel() and setZoomFactor() to the displayable range', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');
      try {
        w.webContents.setZoomFactor(100);
        expect(Math.abs(w.webContents.getZoomFactor() - 5)).toBeLessThanOrEqual(0.001);
        w.webContents.setZoomLevel(-100);
        expect(Math.abs(w.webContents.getZoomFactor() - 0.25)).toBeLessThanOrEqual(0.001);
      } finally {
        w.webContents.zoomLevel = 0;
      }
    });

    it('lets the zoomIn role recover right after zooming out past the minimum', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');
      const menu = Menu.buildFromTemplate([{ role: 'zoomOut' }, { role: 'zoomIn' }]);
      try {
        for (let i = 0; i < 30; i++) menu.items[0].click(undefined, w, w.webContents);
        expect(Math.abs(w.webContents.getZoomFactor() - 0.25)).toBeLessThanOrEqual(0.001);
        menu.items[1].click(undefined, w, w.webContents);
        expect(w.webContents.getZoomFactor()).toBeGreaterThan(0.26);
      } finally {
        w.webContents.zoomLevel = 0;
      }
    });

    it('does not shrink the page when the visual zoom minimum is below 1', async () => {
      const w = new BrowserWindow({ show: false, width: 400, height: 400 });
      await w.loadURL('about:blank');
      await w.webContents.setVisualZoomLevelLimits(0.25, 3);
      w.setSize(500, 500);
      await setTimeout(200);
      const scale = await w.webContents.executeJavaScript('window.visualViewport.scale');
      expect(scale).toBe(1);
    });
  });

  describe('zoom mode', () => {
    afterEach(closeAllWindows);

    it('defaults to "default" zoom mode', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');
      expect(w.webContents.getZoomMode()).toBe('default');
    });

    it('can get and set zoom mode via functions', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');

      w.webContents.setZoomMode('isolated');
      expect(w.webContents.getZoomMode()).toBe('isolated');

      w.webContents.setZoomMode('manual');
      expect(w.webContents.getZoomMode()).toBe('manual');

      w.webContents.setZoomMode('disabled');
      expect(w.webContents.getZoomMode()).toBe('disabled');

      w.webContents.setZoomMode('default');
      expect(w.webContents.getZoomMode()).toBe('default');
    });

    it('can get and set zoom mode via property', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');

      w.webContents.zoomMode = 'isolated';
      expect(w.webContents.zoomMode).toBe('isolated');

      w.webContents.zoomMode = 'default';
      expect(w.webContents.zoomMode).toBe('default');
    });

    it('throws on invalid zoom mode', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');

      expect(() => {
        w.webContents.setZoomMode('invalid' as any);
      }).toThrow();
    });

    it('isolated mode prevents zoom propagation across same-origin tabs', async () => {
      const w = new BrowserWindow({ show: false });
      const w2 = new BrowserWindow({ show: false });

      defer(() => {
        w2.setClosable(true);
        w2.close();
      });

      await w.loadURL('about:blank');
      await w2.loadURL('about:blank');

      w.webContents.setZoomMode('isolated');
      w.webContents.setZoomLevel(2.0);

      expect(w.webContents.getZoomLevel()).toBe(2.0);
      expect(w2.webContents.getZoomLevel()).not.toBe(2.0);
    });

    it('disabled mode prevents zoom level changes', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');

      w.webContents.setZoomMode('disabled');
      const beforeLevel = w.webContents.getZoomLevel();
      w.webContents.setZoomLevel(2.0);
      expect(w.webContents.getZoomLevel()).toBe(beforeLevel);
    });

    it('persists isolated mode across cross-document navigation', async () => {
      const server = http.createServer((req, res) => {
        res.end('hello');
      });
      const { url: serverUrl } = await listen(server);
      defer(() => server.close());

      const w = new BrowserWindow({ show: false });
      await w.loadURL(serverUrl);

      w.webContents.setZoomMode('isolated');
      w.webContents.setZoomLevel(2.0);
      expect(w.webContents.getZoomMode()).toBe('isolated');

      const crossSiteUrl = serverUrl.replace('127.0.0.1', 'localhost');
      await w.loadURL(crossSiteUrl);
      expect(w.webContents.getZoomMode()).toBe('isolated');
    });

    it('resets isolated mode when switched back to default', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');

      w.webContents.setZoomMode('isolated');
      expect(w.webContents.getZoomMode()).toBe('isolated');

      w.webContents.setZoomMode('default');
      expect(w.webContents.getZoomMode()).toBe('default');
    });

    it('manual mode tracks zoom level without zooming the page', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');

      w.webContents.setZoomMode('manual');
      w.webContents.setZoomLevel(2.0);

      // Controller reports the manually tracked level
      expect(w.webContents.getZoomLevel()).toBe(2.0);
    });

    it('preserves zoom level after navigation in isolated mode', async () => {
      const server = http.createServer((req, res) => {
        res.end('hello');
      });
      const { url: serverUrl } = await listen(server);
      defer(() => server.close());

      const w = new BrowserWindow({ show: false });
      await w.loadURL(serverUrl);

      w.webContents.setZoomMode('isolated');
      w.webContents.setZoomLevel(2.0);

      const crossSiteUrl = serverUrl.replace('127.0.0.1', 'localhost');
      await w.loadURL(crossSiteUrl);

      expect(w.webContents.getZoomMode()).toBe('isolated');
      expect(w.webContents.getZoomLevel()).toBe(2.0);
    });

    it('transitions from disabled to isolated mode correctly', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');

      // Start in disabled mode — zoom changes should be ignored
      w.webContents.setZoomMode('disabled');
      w.webContents.setZoomLevel(3.0);
      expect(w.webContents.getZoomLevel()).toBe(0);

      // Transition to isolated mode (special code path: doesn't call
      // SetTemporaryZoomLevel, manually fires observer event instead)
      w.webContents.setZoomMode('isolated');
      expect(w.webContents.getZoomMode()).toBe('isolated');

      // Zoom should now work in isolated mode
      w.webContents.setZoomLevel(2.0);
      expect(w.webContents.getZoomLevel()).toBe(2.0);
    });

    it('isolated zoom does not leak to same-origin tabs after navigation', async () => {
      const server = http.createServer((req, res) => {
        res.end('hello');
      });
      const { url: serverUrl } = await listen(server);
      defer(() => server.close());

      const w = new BrowserWindow({ show: false });
      const w2 = new BrowserWindow({ show: false });
      defer(() => {
        w2.setClosable(true);
        w2.close();
      });

      // w2 loads the target origin first — reset any stale per-host zoom
      await w2.loadURL(serverUrl);
      w2.webContents.setZoomLevel(0);

      // w starts on a different origin, sets isolated zoom
      const crossSiteUrl = serverUrl.replace('127.0.0.1', 'localhost');
      await w.loadURL(crossSiteUrl);
      w.webContents.setZoomMode('isolated');
      w.webContents.setZoomLevel(2.0);

      // w navigates to the same origin as w2
      await w.loadURL(serverUrl);

      // w2 should remain unaffected
      expect(w.webContents.getZoomLevel()).toBe(2.0);
      expect(w2.webContents.getZoomLevel()).toBe(0);
    });

    it('can set zoom mode via webPreferences', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          zoomMode: 'isolated'
        }
      });
      await w.loadURL('about:blank');
      expect(w.webContents.getZoomMode()).toBe('isolated');
    });

    it('webPreferences zoomMode isolated prevents cross-tab zoom', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          zoomMode: 'isolated'
        }
      });
      const w2 = new BrowserWindow({ show: false });
      defer(() => {
        w2.setClosable(true);
        w2.close();
      });

      await w.loadURL('about:blank');
      await w2.loadURL('about:blank');

      w.webContents.setZoomLevel(2.0);
      expect(w.webContents.getZoomLevel()).toBe(2.0);
      expect(w2.webContents.getZoomLevel()).not.toBe(2.0);
    });

    it('webPreferences zoomMode disabled prevents zoom changes', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          zoomMode: 'disabled'
        }
      });
      await w.loadURL('about:blank');

      expect(w.webContents.getZoomMode()).toBe('disabled');
      w.webContents.setZoomLevel(2.0);
      expect(w.webContents.getZoomLevel()).toBe(0);
    });

    it('default mode shares zoom across same-origin tabs', async () => {
      const w = new BrowserWindow({ show: false });
      const w2 = new BrowserWindow({ show: false });
      defer(() => {
        w2.setClosable(true);
        w2.close();
      });

      await w.loadURL('about:blank');
      await w2.loadURL('about:blank');

      try {
        // Both in default mode — zoom should propagate per-domain
        w.webContents.setZoomLevel(2.0);
        expect(w2.webContents.getZoomLevel()).toBe(2.0);
      } finally {
        // Reset per-host zoom in HostZoomMap to avoid leaking to other tests
        w.webContents.setZoomLevel(0);
      }
    });

    it('switching from isolated to default re-joins per-domain zoom', async () => {
      const server = http.createServer((req, res) => {
        res.end('hello');
      });
      const { url: serverUrl } = await listen(server);
      defer(() => server.close());

      const w = new BrowserWindow({ show: false });
      const w2 = new BrowserWindow({ show: false });
      defer(() => {
        w2.setClosable(true);
        w2.close();
      });

      await w.loadURL(serverUrl);
      await w2.loadURL(serverUrl);
      // Reset any stale per-host zoom from previous tests
      w.webContents.setZoomLevel(0);

      try {
        // Isolated: w's zoom should NOT affect w2
        w.webContents.setZoomMode('isolated');
        w.webContents.setZoomLevel(2.0);
        expect(w2.webContents.getZoomLevel()).toBe(0);

        // Switch back to default: w's zoom should now affect w2
        w.webContents.setZoomMode('default');
        w.webContents.setZoomLevel(3.0);
        expect(w2.webContents.getZoomLevel()).toBe(3.0);
      } finally {
        // Reset per-host zoom in HostZoomMap to avoid leaking to other tests
        w.webContents.setZoomLevel(0);
      }
    });

    it('webPreferences zoomMode isolated persists across navigation', async () => {
      const server = http.createServer((req, res) => {
        res.end('hello');
      });
      const { url: serverUrl } = await listen(server);
      defer(() => server.close());

      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          zoomMode: 'isolated'
        }
      });
      await w.loadURL(serverUrl);
      w.webContents.setZoomLevel(2.0);

      const crossSiteUrl = serverUrl.replace('127.0.0.1', 'localhost');
      await w.loadURL(crossSiteUrl);

      expect(w.webContents.getZoomMode()).toBe('isolated');
      expect(w.webContents.getZoomLevel()).toBe(2.0);
    });

    it('preserves zoomMode and zoomLevel after reload', async () => {
      const server = http.createServer((req, res) => {
        res.end('hello');
      });
      const { url: serverUrl } = await listen(server);
      defer(() => server.close());

      const w = new BrowserWindow({ show: false });
      await w.loadURL(serverUrl);

      w.webContents.setZoomMode('isolated');
      w.webContents.setZoomLevel(2.0);

      // Reload the page
      const reloadPromise = once(w.webContents, 'did-finish-load');
      w.webContents.reload();
      await reloadPromise;

      expect(w.webContents.getZoomMode()).toBe('isolated');
      expect(w.webContents.getZoomLevel()).toBe(2.0);
    });

    it('isolated mode with webPreferences.zoomFactor preserves zoom after navigation', async () => {
      const server = http.createServer((req, res) => {
        res.end('hello');
      });
      const { url: serverUrl } = await listen(server);
      defer(() => server.close());

      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          zoomMode: 'isolated',
          zoomFactor: 1.5
        }
      });
      await w.loadURL(serverUrl);
      w.webContents.setZoomLevel(2.0);

      // Navigate cross-site — the zoomFactor must NOT override our isolated zoom
      const crossSiteUrl = serverUrl.replace('127.0.0.1', 'localhost');
      await w.loadURL(crossSiteUrl);

      expect(w.webContents.getZoomMode()).toBe('isolated');
      expect(w.webContents.getZoomLevel()).toBe(2.0);
    });

    it('preserves zoomMode and zoomLevel after same-origin navigation', async () => {
      const server = http.createServer((req, res) => {
        res.end('hello');
      });
      const { url: serverUrl } = await listen(server);
      defer(() => server.close());

      const w = new BrowserWindow({ show: false });
      await w.loadURL(serverUrl);

      w.webContents.setZoomMode('isolated');
      w.webContents.setZoomLevel(2.0);

      // Navigate to a different path on the same origin
      await w.loadURL(serverUrl + '/other-page');

      expect(w.webContents.getZoomMode()).toBe('isolated');
      expect(w.webContents.getZoomLevel()).toBe(2.0);
    });

    it('webPreferences zoomFactor is applied as initial zoom in isolated mode', async () => {
      const server = http.createServer((req, res) => {
        res.end('hello');
      });
      const { url: serverUrl } = await listen(server);
      defer(() => server.close());

      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          zoomMode: 'isolated',
          zoomFactor: 1.5
        }
      });
      await w.loadURL(serverUrl);

      // The zoomFactor should be applied as the initial zoom level
      expect(w.webContents.getZoomMode()).toBe('isolated');
      const zoomFactor = w.webContents.getZoomFactor();
      expect(Math.abs(zoomFactor - 1.5)).toBeLessThanOrEqual(0.05);
    });

    it('rapid mode switching does not crash', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');

      // Rapidly switch between all modes
      w.webContents.setZoomMode('isolated');
      w.webContents.setZoomMode('manual');
      w.webContents.setZoomMode('disabled');
      w.webContents.setZoomMode('default');
      w.webContents.setZoomMode('isolated');
      w.webContents.setZoomMode('default');

      expect(w.webContents.getZoomMode()).toBe('default');
    });

    it('mode transitions preserve expected zoom behavior', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');

      // disabled -> isolated -> manual -> default
      w.webContents.setZoomMode('disabled');
      w.webContents.setZoomLevel(3.0);
      expect(w.webContents.getZoomLevel()).toBe(0); // disabled ignores zoom

      w.webContents.setZoomMode('isolated');
      w.webContents.setZoomLevel(2.0);
      expect(w.webContents.getZoomLevel()).toBe(2.0); // isolated works

      w.webContents.setZoomMode('manual');
      w.webContents.setZoomLevel(1.5);
      expect(w.webContents.getZoomLevel()).toBe(1.5); // manual tracks level

      w.webContents.setZoomMode('default');
      // After switching to default, zoom should be at the per-domain level
      expect(w.webContents.getZoomMode()).toBe('default');
      // Reset per-host zoom in HostZoomMap to avoid leaking to other tests
      w.webContents.setZoomLevel(0);
    });

    it('isolated mode does not share zoom across different sessions', async () => {
      const w = new BrowserWindow({ show: false });
      const w2 = new BrowserWindow({
        show: false,
        webPreferences: { partition: 'zoom-test-partition' }
      });
      defer(() => {
        w2.setClosable(true);
        w2.close();
      });

      await w.loadURL('about:blank');
      await w2.loadURL('about:blank');

      w.webContents.setZoomMode('isolated');
      w.webContents.setZoomLevel(2.0);

      // Different session — should never share zoom regardless of mode
      expect(w2.webContents.getZoomLevel()).toBe(0);
    });

    it('setZoomLevel and setZoomFactor are consistent in isolated mode', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');

      w.webContents.setZoomMode('isolated');

      // Set via setZoomFactor, read via getZoomLevel
      w.webContents.setZoomFactor(1.5);
      const levelFromFactor = w.webContents.getZoomLevel();
      expect(levelFromFactor).not.toBe(0);

      // Set via setZoomLevel, read via getZoomFactor
      w.webContents.setZoomLevel(0);
      const factorFromLevel = w.webContents.getZoomFactor();
      expect(Math.abs(factorFromLevel - 1.0)).toBeLessThanOrEqual(0.05);
    });

    it('isolated mode works correctly after error page navigation', async () => {
      const server = http.createServer((req, res) => {
        res.end('hello');
      });
      const { url: serverUrl } = await listen(server);
      defer(() => server.close());

      const w = new BrowserWindow({ show: false });
      await w.loadURL(serverUrl);

      w.webContents.setZoomMode('isolated');
      w.webContents.setZoomLevel(2.0);

      // Navigate to an error page
      await w.loadURL('http://localhost:1/nonexistent').catch(() => {});

      // Mode should still be isolated
      expect(w.webContents.getZoomMode()).toBe('isolated');

      // Navigate back to a valid page
      await w.loadURL(serverUrl);
      expect(w.webContents.getZoomMode()).toBe('isolated');
      expect(w.webContents.getZoomLevel()).toBe(2.0);
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
      ] as const;
      for (const policy of policies) {
        w.webContents.setWebRTCIPHandlingPolicy(policy);
        expect(w.webContents.getWebRTCIPHandlingPolicy()).toBe(policy);
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
      expect(settings).toEqual({ min: 0, max: 0 });
    });

    it('can set and get webrtc udp port range policy with correct arguments', () => {
      w.webContents.setWebRTCUDPPortRange({ min: 1, max: 65535 });
      const settings = w.webContents.getWebRTCUDPPortRange();
      expect(settings).toEqual({ min: 1, max: 65535 });
    });

    it('can not set webrtc udp port range policy with invalid arguments', () => {
      expect(() => {
        w.webContents.setWebRTCUDPPortRange({ min: 0, max: 65535 });
      }).toThrow("'min' and 'max' must be in the (0, 65535] range or [0, 0]");
      expect(() => {
        w.webContents.setWebRTCUDPPortRange({ min: 1, max: 65536 });
      }).toThrow("'min' and 'max' must be in the (0, 65535] range or [0, 0]");
      expect(() => {
        w.webContents.setWebRTCUDPPortRange({ min: 60000, max: 56789 });
      }).toThrow("'max' must be greater than or equal to 'min'");
    });

    it('can reset webrtc udp port range policy to default with { min: 0, max: 0 }', () => {
      w.webContents.setWebRTCUDPPortRange({ min: 1, max: 65535 });
      const settings = w.webContents.getWebRTCUDPPortRange();
      expect(settings).toEqual({ min: 1, max: 65535 });
      w.webContents.setWebRTCUDPPortRange({ min: 0, max: 0 });
      const defaultSetting = w.webContents.getWebRTCUDPPortRange();
      expect(defaultSetting).toEqual({ min: 0, max: 0 });
    });
  });

  describe('opener api', () => {
    afterEach(closeAllWindows);
    it('can get opener with window.open()', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { sandbox: true } });
      await w.loadURL('about:blank');
      const childPromise = once(w.webContents, 'did-create-window') as Promise<
        [BrowserWindow, Electron.DidCreateWindowDetails]
      >;
      w.webContents.executeJavaScript('window.open("about:blank")', true);
      const [childWindow] = await childPromise;
      expect(childWindow.webContents.opener).toBe(w.webContents.mainFrame);
    });
    it('has no opener when using "noopener"', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { sandbox: true } });
      await w.loadURL('about:blank');
      const childPromise = once(w.webContents, 'did-create-window') as Promise<
        [BrowserWindow, Electron.DidCreateWindowDetails]
      >;
      w.webContents.executeJavaScript('window.open("about:blank", undefined, "noopener")', true);
      const [childWindow] = await childPromise;
      expect(childWindow.webContents.opener).toBeNull();
    });
    it('can get opener with a[target=_blank][rel=opener]', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { sandbox: true } });
      await w.loadURL('about:blank');
      const childPromise = once(w.webContents, 'did-create-window') as Promise<
        [BrowserWindow, Electron.DidCreateWindowDetails]
      >;
      w.webContents.executeJavaScript(
        `(function() {
        const a = document.createElement('a');
        a.target = '_blank';
        a.rel = 'opener';
        a.href = 'about:blank';
        a.click();
      }())`,
        true
      );
      const [childWindow] = await childPromise;
      expect(childWindow.webContents.opener).toBe(w.webContents.mainFrame);
    });
    it('has no opener with a[target=_blank][rel=noopener]', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { sandbox: true } });
      await w.loadURL('about:blank');
      const childPromise = once(w.webContents, 'did-create-window') as Promise<
        [BrowserWindow, Electron.DidCreateWindowDetails]
      >;
      w.webContents.executeJavaScript(
        `(function() {
        const a = document.createElement('a');
        a.target = '_blank';
        a.rel = 'noopener';
        a.href = 'about:blank';
        a.click();
      }())`,
        true
      );
      const [childWindow] = await childPromise;
      expect(childWindow.webContents.opener).toBeNull();
    });
  });

  describe('focusedFrame api', () => {
    const focusFrame = (frame: Electron.WebFrameMain) => {
      // There has to be a better way to do this...
      return frame.executeJavaScript(
        `(${() => {
          const input = document.createElement('input');
          document.body.appendChild(input);
          input.onfocus = () => input.remove();
          input.focus();
        }})()`,
        true
      );
    };

    it('is null before a url is committed', () => {
      const w = new BrowserWindow({ show: false });
      expect(w.webContents.focusedFrame).toBeNull();
    });

    it('is set when main frame is focused', async () => {
      const w = new BrowserWindow({ show: true });
      await w.loadURL('about:blank');
      w.webContents.focus();
      await waitUntil(() => w.webContents.focusedFrame === w.webContents.mainFrame);
    });

    it('is set to child frame when focused', async () => {
      const w = new BrowserWindow({ show: true });
      await w.loadFile(path.join(fixturesPath, 'sub-frames', 'frame-with-frame-container.html'));
      const childFrame = w.webContents.mainFrame.frames[0];
      await focusFrame(childFrame);
      await waitUntil(() => w.webContents.focusedFrame === childFrame);
    });
  });

  describe('unresponsive event', { tags: ['serial'] }, () => {
    afterEach(closeAllWindows);
    const testing = () => process._linkedBinding('electron_common_testing');
    // The hang monitor reports after kHungRendererDelay (15 s) plus a 1 s ping.
    const hangAndPoke = async (w: BrowserWindow, ms = 0) => {
      w.webContents
        .executeJavaScript(ms ? `{ const end = Date.now() + ${ms}; while (Date.now() < end) {} }` : 'while (true) {}')
        .catch(() => {});
      await setTimeout(200);
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'A' });
    };

    ifit(isTestingBindingAvailable())(
      'is not emitted within a hang delay of a system resume',
      { timeout: 70000 },
      async () => {
        const w = new BrowserWindow({ show: true });
        await w.loadURL('about:blank');
        let unresponsiveAt = 0;
        w.webContents.once('unresponsive', () => {
          unresponsiveAt = Date.now();
        });
        await hangAndPoke(w);
        // Sleep and wake while the timeout is pending; it would fire ~6 s after
        // this resume, which says nothing about the renderer.
        await setTimeout(10000);
        testing().simulatePowerEvent('suspend');
        testing().simulatePowerEvent('resume');
        const resumedAt = Date.now();
        await once(w.webContents, 'unresponsive');
        expect(unresponsiveAt - resumedAt).toBeGreaterThan(15000);
      }
    );

    it('is not followed by responsive just because the window is hidden', { timeout: 90000 }, async () => {
      const w = new BrowserWindow({ show: true });
      await w.loadURL('about:blank');
      await hangAndPoke(w, 40000);
      await once(w.webContents, 'unresponsive');
      const events: string[] = [];
      w.webContents.on('responsive', () => events.push('responsive'));
      w.webContents.on('unresponsive', () => events.push('unresponsive'));
      w.hide();
      await setTimeout(2000);
      expect(events, 'after hide').toEqual([]);
      w.show();
      // Still hung and visible again: reported again after one delay.
      await once(w.webContents, 'unresponsive');
      // The spin ends ~40 s in; the pending key event is then acked.
      await once(w.webContents, 'responsive');
      expect(events).toEqual(['unresponsive', 'responsive']);
    });

    it('is emitted for a hang with no suspend involved', { timeout: 40000 }, async () => {
      const w = new BrowserWindow({ show: true });
      await w.loadURL('about:blank');
      await hangAndPoke(w);
      await once(w.webContents, 'unresponsive');
    });
  });

  describe('render view deleted events', () => {
    let server: http.Server;
    let serverUrl: string;
    let crossSiteUrl: string;

    beforeAll(async () => {
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
            res.end("<html><script>window.open('wrong://url', 'second child');</script></html>");
          } else {
            res.end();
          }
        };
        setTimeout().then(respond);
      });
      serverUrl = (await listen(server)).url;
      crossSiteUrl = serverUrl.replace('127.0.0.1', 'localhost');
    });

    afterAll(() => {
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
      const destroyed = once(w.webContents, 'destroyed');
      w.loadURL(`${serverUrl}/redirect-cross-site`);
      await destroyed;
      expect(currentRenderViewDeletedEmitted, 'current-render-view-deleted was emitted').toBe(false);
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
      expect(currentRenderViewDeletedEmitted, 'child window was destroyed').toBe(false);
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
      expect(currentRenderViewDeletedEmitted, "current-render-view-deleted wasn't emitted").toBe(true);
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
      expect(rvhDeletedCount, "render-view-deleted wasn't emitted the expected nr. of times").toBe(
        expectedRenderViewDeletedEventCount
      );
    });
  });

  describe('setIgnoreMenuShortcuts(ignore)', { tags: ['serial'] }, () => {
    afterEach(closeAllWindows);

    const trackShortcutInvocations = (contents: WebContents) => {
      const previousMenu = Menu.getApplicationMenu();
      let invocations = 0;
      Menu.setApplicationMenu(
        Menu.buildFromTemplate([
          {
            label: 'Test',
            submenu: [{ label: 'Shortcut', accelerator: 'F13', click: () => invocations++ }]
          }
        ])
      );
      contents.debugger.attach();
      defer(() => {
        Menu.setApplicationMenu(previousMenu);
        if (!contents.isDestroyed() && contents.debugger.isAttached()) contents.debugger.detach();
      });

      return async (expectedInvocations: number) => {
        const base = { key: 'F13', code: 'F13', windowsVirtualKeyCode: 124 };
        if (process.platform === 'darwin') {
          // Native macOS menu input requires kVK_F13 and NSF13FunctionKey.
          await contents.debugger.sendCommand('Input.dispatchKeyEvent', {
            ...base,
            type: 'rawKeyDown',
            nativeVirtualKeyCode: 0x69,
            text: '\uF710'
          });
        } else {
          contents.sendInputEvent({ type: 'keyDown', keyCode: 'F13' });
        }
        await contents.debugger.sendCommand('Input.dispatchKeyEvent', { ...base, type: 'keyUp' });
        await waitUntil(() => invocations >= expectedInvocations);
        return invocations;
      };
    };

    it('does not throw', () => {
      const w = new BrowserWindow({ show: false });
      expect(() => {
        w.webContents.setIgnoreMenuShortcuts(true);
        w.webContents.setIgnoreMenuShortcuts(false);
      }).not.toThrow();
    });

    it('honors the initial ignoreMenuShortcuts preference', async () => {
      const window = new BrowserWindow({
        show: true,
        webPreferences: { ignoreMenuShortcuts: true } as Electron.WebPreferences
      });
      await window.loadURL('about:blank');
      window.webContents.focus();
      await waitUntil(() => window.webContents.isFocused());
      const sendShortcut = trackShortcutInvocations(window.webContents);
      expect(await sendShortcut(0)).toBe(0);
      window.webContents.setIgnoreMenuShortcuts(false);
      expect(await sendShortcut(1)).toBe(1);
    });

    it('does not crash for detached DevTools without preferences', async () => {
      const window = new BrowserWindow({ show: false });
      await window.loadURL('about:blank');
      const devToolsOpened = once(window.webContents, 'devtools-opened');
      window.webContents.openDevTools({ mode: 'detach', activate: false });
      await devToolsOpened;

      const devTools = window.webContents.devToolsWebContents!;
      expect(devTools.getLastWebPreferences()).toBe(null);
      devTools.setIgnoreMenuShortcuts(false);
      devTools.setIgnoreMenuShortcuts(true);
      devTools.setIgnoreMenuShortcuts(false);
    });

    for (const target of ['webview', 'docked DevTools'] as const) {
      it(`uses the source settings for ${target}`, async () => {
        const window = new BrowserWindow({ show: true, webPreferences: { webviewTag: true } });
        let source: WebContents;
        if (target === 'webview') {
          const attached = once(window.webContents, 'did-attach-webview') as Promise<[any, WebContents]>;
          await window.loadFile(path.join(fixturesPath, 'pages', 'webview-zoom-factor.html'));
          [, source] = await attached;
          // Navigating the guest while its src is still loading aborts that
          // load, and its did-fail-load can reject the loadURL() below.
          if (source.isLoading()) await once(source, 'did-finish-load');
          await source.loadURL('about:blank');
        } else {
          await window.loadURL('about:blank');
          const devToolsOpened = once(window.webContents, 'devtools-opened');
          const devToolsFocused = once(window.webContents, 'devtools-focused');
          window.webContents.openDevTools({ mode: 'right', activate: true });
          await Promise.all([devToolsOpened, devToolsFocused]);
          source = window.webContents.devToolsWebContents!;
          expect(source.getLastWebPreferences()).toBe(null);
          source.setIgnoreMenuShortcuts(false);
        }

        const sendShortcut = trackShortcutInvocations(source);
        let expectedInvocations = 0;
        for (const ignore of [true, false, true, false]) {
          if (target === 'webview') {
            window.focus();
            await window.webContents.executeJavaScript("document.querySelector('webview').focus()");
          }
          window.webContents.setIgnoreMenuShortcuts(!ignore);
          source.setIgnoreMenuShortcuts(ignore);
          if (!ignore) expectedInvocations++;
          expect(await sendShortcut(expectedInvocations)).toBe(expectedInvocations);
        }
      });
    }
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
        expect(w.webContents.isCrashed()).toBe(false);
      });

      it('forcefullyCrashRenderer() crashes the process with reason=killed||crashed', async () => {
        expect(w.webContents.isCrashed()).toBe(false);
        const crashEvent = once(w.webContents, 'render-process-gone') as Promise<
          [any, Electron.RenderProcessGoneDetails]
        >;
        w.webContents.forcefullyCrashRenderer();
        const [, details] = await crashEvent;
        expect(details.reason === 'killed' || details.reason === 'crashed', 'reason should be killed || crashed').toBe(
          true
        );
        expect(w.webContents.isCrashed()).toBe(true);
      });

      it('a crashed process is recoverable with reload()', async () => {
        expect(w.webContents.isCrashed()).toBe(false);
        w.webContents.forcefullyCrashRenderer();
        w.webContents.reload();
        expect(w.webContents.isCrashed()).toBe(false);
      });

      it('survives a synchronous reload() from the render-process-gone handler', async () => {
        // Regression test: a synchronous reload() from 'render-process-gone'
        // used to re-enter renderer process launch mid-teardown and
        // CHECK-crash the browser process. See
        // WebContents::PrimaryMainFrameRenderProcessGone.
        const crashEvent = once(w.webContents, 'render-process-gone');
        w.webContents.once('render-process-gone', () => {
          // Deliberately synchronous.
          w.webContents.reload();
        });
        w.webContents.forcefullyCrashRenderer();
        await crashEvent;
        await once(w.webContents, 'did-finish-load');
        expect(w.webContents.isCrashed()).toBe(false);
      });

      // The response is held until a renderer has been frozen, so the
      // navigation is waiting to commit when that renderer is killed; loadURL()
      // then rejects from inside content's teardown and the handler navigates
      // again straight away. With COOP the response starts a second renderer
      // for a speculative frame host and that is the one killed.
      for (const coop of [false, true]) {
        ifit(process.platform !== 'win32')(
          `survives a loadURL() from the rejection of a load whose ${coop ? 'speculative ' : ''}renderer died before commit`,
          async () => {
            let release: (() => void) | null = null;
            const server = http.createServer((req, res) => {
              release = () => {
                res.setHeader('content-type', 'text/html');
                if (coop) res.setHeader('cross-origin-opener-policy', 'same-origin-allow-popups');
                res.end('<h1>hi</h1>');
              };
            });
            defer(() => server.close());
            const { url } = await listen(server);
            const rendererPids = () =>
              app
                .getAppMetrics()
                .filter((m) => m.type === 'Tab')
                .map((m) => m.pid);

            const load = w.webContents.loadURL(url);
            await waitUntil(() => w.webContents.getOSProcessId() !== 0 && release !== null);
            const before = new Set(rendererPids());
            let victim = w.webContents.getOSProcessId();
            if (!coop) process.kill(victim, 'SIGSTOP');
            release!();
            if (coop) {
              await waitUntil(() => rendererPids().some((pid) => !before.has(pid)));
              victim = rendererPids().find((pid) => !before.has(pid))!;
              process.kill(victim, 'SIGSTOP');
            }
            await setTimeout(1000);
            process.kill(victim, 'SIGKILL');

            await expect(load).rejects.toThrow();
            await w.webContents.loadURL('about:blank');
            expect(w.webContents.isCrashed()).toBe(false);
          }
        );
      }
    });
  }

  // Destroying webContents in its event listener is going to crash when
  // Electron is built in Debug mode.
  describe('destroy()', { retry: 3 }, () => {
    // These tests are flaky on Windows CI and we don't know why, but their
    // purpose is to make sure Electron does not crash so it is fine to retry
    // them a few times.

    let server: http.Server;
    let serverUrl: string;

    beforeAll(
      () =>
        new Promise<void>((resolve, reject) => {
          const done = (error?: unknown) => (error ? reject(error) : resolve());
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
        })
    );

    afterAll(() => {
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
      it(`should not crash when invoked synchronously inside ${e.name} handler`, async () => {
        const contents = (webContents as typeof ElectronInternal.WebContents).create();
        const originalEmit = contents.emit.bind(contents);
        contents.emit = (...args) => {
          return originalEmit(...args);
        };
        contents.once(e.name as any, () => contents.destroy());
        const destroyed = once(contents, 'destroyed');
        contents.loadURL(serverUrl + e.url);
        await destroyed;
      });
    }
  });

  describe('did-change-theme-color event', () => {
    afterEach(closeAllWindows);
    it('is triggered with correct theme color', () =>
      new Promise<void>((resolve, reject) => {
        const done = (error?: unknown) => (error ? reject(error) : resolve());
        const w = new BrowserWindow({ show: true });
        let count = 0;
        w.webContents.on('did-change-theme-color', (e, color) => {
          try {
            if (count === 0) {
              count += 1;
              expect(color).toBe('#FFEEDD');
              w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));
            } else if (count === 1) {
              expect(color).toBeNull();
              done();
            }
          } catch (e) {
            done(e);
          }
        });
        w.loadFile(path.join(fixturesPath, 'pages', 'theme-color.html'));
      }));
  });

  describe('console-message event', () => {
    afterEach(closeAllWindows);
    it('is triggered with correct log message', () =>
      new Promise<void>((resolve) => {
        const w = new BrowserWindow({ show: true });
        w.webContents.on('console-message', (e) => {
          // Don't just assert as Chromium might emit other logs that we should ignore.
          if (e.message === 'a') {
            resolve();
          }
        });
        w.loadFile(path.join(fixturesPath, 'pages', 'a.html'));
      }));

    describe('on a destroyed WebContents', () => {
      const destroyedWebContents = async (handler?: (...args: any[]) => void) => {
        const w = new BrowserWindow({ show: false });
        const wc = w.webContents;
        if (handler) wc.on('console-message', handler);
        const destroyed = once(wc, 'destroyed');
        w.destroy();
        await destroyed;
        expect(wc.isDestroyed()).toBe(true);
        return wc;
      };

      it('does not throw when adding a listener', async () => {
        const wc = await destroyedWebContents();
        expect(() => wc.on('console-message', () => {})).not.toThrow();
        expect(wc.listenerCount('console-message')).toBe(1);
      });

      it('does not throw when removing a listener', async () => {
        const handler = () => {};
        const wc = await destroyedWebContents(handler);
        expect(() => wc.removeListener('console-message', handler)).not.toThrow();
        expect(wc.listenerCount('console-message')).toBe(0);
      });

      it('does not throw when removing all listeners', async () => {
        const wc = await destroyedWebContents(() => {});
        expect(() => wc.removeAllListeners('console-message')).not.toThrow();
        expect(wc.listenerCount('console-message')).toBe(0);
        wc.on('console-message', () => {});
        expect(() => wc.removeAllListeners()).not.toThrow();
      });
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
      expect(channel).toBe('message');
      expect(message).toBe('Hello World!');
    });
  });

  describe('ipc-message-sync event', () => {
    afterEach(closeAllWindows);
    it('emits when the renderer process sends a synchronous message', async () => {
      const w = new BrowserWindow({ show: true, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      await w.webContents.loadURL('about:blank');
      const promise: Promise<[string, string]> = new Promise((resolve) => {
        w.webContents.once('ipc-message-sync', (event, channel, arg) => {
          event.returnValue = 'foobar';
          resolve([channel, arg]);
        });
      });
      const result = await w.webContents.executeJavaScript(`
        require('electron').ipcRenderer.sendSync('message', 'Hello World!')
      `);

      const [channel, message] = await promise;
      expect(channel).toBe('message');
      expect(message).toBe('Hello World!');
      expect(result).toBe('foobar');
    });
  });

  describe('referrer', () => {
    afterEach(closeAllWindows);
    it('propagates referrer information to new target=_blank windows', () =>
      new Promise<void>((resolve, reject) => {
        const done = (error?: unknown) => (error ? reject(error) : resolve());
        const w = new BrowserWindow({ show: false });
        const server = http.createServer((req, res) => {
          if (req.url === '/should_have_referrer') {
            try {
              expect(req.headers.referer).toBe(`http://127.0.0.1:${(server.address() as AddressInfo).port}/`);
              return done();
            } catch (e) {
              return done(e);
            }
          }
          res.end('<a id="a" href="/should_have_referrer" target="_blank">link</a>');
        });
        defer(() => {
          server.close();
        });
        listen(server).then(({ url }) => {
          w.webContents.once('did-finish-load', () => {
            w.webContents.setWindowOpenHandler((details) => {
              expect(details.referrer.url).toBe(url + '/');
              expect(details.referrer.policy).toBe('strict-origin-when-cross-origin');
              return { action: 'allow' };
            });
            w.webContents.executeJavaScript('a.click()');
          });
          w.loadURL(url);
        });
      }));

    it('propagates referrer information to windows opened with window.open', () =>
      new Promise<void>((resolve, reject) => {
        const done = (error?: unknown) => (error ? reject(error) : resolve());
        const w = new BrowserWindow({ show: false });
        const server = http.createServer((req, res) => {
          if (req.url === '/should_have_referrer') {
            try {
              expect(req.headers.referer).toBe(`http://127.0.0.1:${(server.address() as AddressInfo).port}/`);
              return done();
            } catch (e) {
              return done(e);
            }
          }
          res.end('');
        });
        defer(() => {
          server.close();
        });
        listen(server).then(({ url }) => {
          w.webContents.once('did-finish-load', () => {
            w.webContents.setWindowOpenHandler((details) => {
              expect(details.referrer.url).toBe(url + '/');
              expect(details.referrer.policy).toBe('strict-origin-when-cross-origin');
              return { action: 'allow' };
            });
            w.webContents.executeJavaScript('window.open(location.href + "should_have_referrer")');
          });
          w.loadURL(url);
        });
      }));
  });

  describe('webframe messages in sandboxed contents', () => {
    afterEach(closeAllWindows);
    it('responds to executeJavaScript', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { sandbox: true } });
      await w.loadURL('about:blank');
      const result = await w.webContents.executeJavaScript('37 + 5');
      expect(result).toBe(42);
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
          expect(preloadPath).toBe(preload);
          expect(error.message).toBe('Hello World!');
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
          expect(preloadPath).toBe(preload);
          expect(error.message).toBe('foobar is not defined');
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
          expect(preloadPath).toBe(preload);
          expect(error.message).toContain('preload-invalid.js');
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
        expect(stats.size).not.toBe(0);
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
      return expect(promise).rejects.toThrow(`Failed to take heap snapshot with invalid file path ${badPath}`);
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
      return expect(promise).rejects.toThrow('Failed to take heap snapshot with nonexistent render frame');
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

    // Regression test: disabling throttling on a WebContentsView while its
    // window is hidden used to un-hide the RenderWidgetHost without telling the
    // view, so the surface the renderer produced was never embedded and the
    // window stayed blank (and un-capturable) once shown, until a resize.
    it('leaves a hidden WebContentsView paintable once its window is shown', async () => {
      const w = new BaseWindow({ show: false, width: 300, height: 200 });
      const view = new WebContentsView();
      w.contentView.addChildView(view);
      view.setBounds({ x: 0, y: 0, width: 300, height: 200 });
      await view.webContents.loadURL('data:text/html,<body style="background:%23ff0000">');
      view.webContents.setBackgroundThrottling(false);
      // Give the renderer time to submit frames while still hidden.
      await setTimeout(500);
      w.show();
      // In the broken state capturePage rejects forever ("Current display
      // surface not available for capture"); in the fixed state the surface is
      // available immediately or after a frame or two.
      let image: Electron.NativeImage | undefined;
      await waitUntil(
        async () => {
          try {
            image = await view.webContents.capturePage();
            return true;
          } catch {
            return false;
          }
        },
        { rate: 100, timeout: 5000 }
      );
      const size = image!.getSize();
      expect(size.width).toBeGreaterThan(0);
      expect(size.height).toBeGreaterThan(0);
      const px = image!.toBitmap();
      // BGRA; expect the red background, not the blank white surface.
      expect(px[2]).toBe(255);
      expect(px[1]).toBe(0);
      expect(px[0]).toBe(0);
    });

    describe('on a page nobody can see', () => {
      // The page is hidden on purpose with w.hide(), but the steps that expect
      // frames from a shown window (the video baseline, the iframe baseline,
      // resuming after w.show()) need it to be really visible. On the Windows
      // CI hosts a newly shown window can land behind another process's
      // console window, and the native occlusion tracker then keeps the page
      // hidden, so keep the window above everything (as #54334 does for the
      // visibility specs) and let only hide() decide what nobody can see.
      const createWindow = (backgroundThrottling: boolean) =>
        new BrowserWindow({
          width: 300,
          height: 200,
          alwaysOnTop: true,
          webPreferences: { backgroundThrottling }
        });

      // Counts requestAnimationFrame callbacks over half a second in |frame|
      // (the main frame when omitted).
      const framesInHalfSecond = (wc: Electron.WebContents, frame: Electron.WebFrameMain | null = wc.mainFrame) =>
        frame!.executeJavaScript(`new Promise(resolve => {
          let n = 0;
          let id = requestAnimationFrame(function f () { n++; id = requestAnimationFrame(f); });
          setTimeout(() => { cancelAnimationFrame(id); resolve(n); }, 500);
        })`) as Promise<number>;
      const visibilityState = (wc: Electron.WebContents, frame: Electron.WebFrameMain | null = wc.mainFrame) =>
        frame!.executeJavaScript('document.visibilityState') as Promise<string>;
      const stopsPainting = (wc: Electron.WebContents, frame?: Electron.WebFrameMain | null) =>
        waitUntil(async () => (await framesInHalfSecond(wc, frame)) === 0, { timeout: 10000 });
      const startsPainting = (wc: Electron.WebContents, frame?: Electron.WebFrameMain | null) =>
        waitUntil(async () => (await framesInHalfSecond(wc, frame)) > 0, { timeout: 10000 });

      // Regression test: re-enabling throttling on a hidden page used to leave
      // the RenderWidgetHost shown (from the setBackgroundThrottling(false)
      // call), so the page kept producing frames at full rate and reporting
      // itself visible until the window's visibility next changed.
      it('stops producing frames again once re-enabled', async () => {
        const w = createWindow(true);
        await w.loadURL('about:blank');
        w.hide();
        await stopsPainting(w.webContents);
        expect(await visibilityState(w.webContents)).toBe('hidden');
        w.webContents.setBackgroundThrottling(false);
        await startsPainting(w.webContents);
        expect(await visibilityState(w.webContents)).toBe('visible');
        w.webContents.setBackgroundThrottling(true);
        await stopsPainting(w.webContents);
        expect(await visibilityState(w.webContents)).toBe('hidden');
      });

      // Same, but with throttling disabled before the window is hidden (the
      // hide is swallowed while disabled), and checking the page comes back to
      // visible when the window is shown afterwards.
      it('stops producing frames when re-enabled after a hide, and resumes when shown', async () => {
        const w = createWindow(true);
        await w.loadURL('about:blank');
        w.webContents.setBackgroundThrottling(false);
        w.hide();
        await setTimeout(500);
        expect(await framesInHalfSecond(w.webContents)).toBeGreaterThan(0);
        expect(await visibilityState(w.webContents)).toBe('visible');
        w.webContents.setBackgroundThrottling(true);
        await stopsPainting(w.webContents);
        expect(await visibilityState(w.webContents)).toBe('hidden');
        w.show();
        await startsPainting(w.webContents);
        expect(await visibilityState(w.webContents)).toBe('visible');
      });

      // Cross-process iframes have their own RenderWidgetHost; re-enabling
      // throttling must hide those too, not just the main frame's.
      it('stops a cross-site iframe producing frames once re-enabled', async () => {
        let crossSiteUrl = '';
        const server = http.createServer((req, res) => {
          res.setHeader('Content-Type', 'text/html');
          res.end(
            req.url === '/child' ? '<body>child</body>' : `<iframe name="child" src="${crossSiteUrl}/child"></iframe>`
          );
        });
        defer(() => server.close());
        const serverUrl = (await listen(server)).url;
        crossSiteUrl = serverUrl.replace('127.0.0.1', 'localhost');
        // Disabled from the start so the iframe's widget is created with it.
        const w = createWindow(false);
        await w.loadURL(serverUrl);
        const child = w.webContents.mainFrame.frames.find((f) => f.name === 'child')!;
        expect(child.osProcessId).not.toBe(w.webContents.mainFrame.osProcessId);
        w.hide();
        await setTimeout(500);
        expect(await framesInHalfSecond(w.webContents, child)).toBeGreaterThan(0);
        w.webContents.setBackgroundThrottling(true);
        await stopsPainting(w.webContents, child);
        expect(await visibilityState(w.webContents, child)).toBe('hidden');
        w.webContents.setBackgroundThrottling(false);
        await startsPainting(w.webContents, child);
        w.show();
        await startsPainting(w.webContents, child);
      });

      // While throttling is disabled the page is reported visible; media must
      // be told too, or a muted video paused for being in the background stays
      // frozen on a page that otherwise renders.
      it('resumes background-paused video while disabled', async () => {
        const imageDataUrl = `data:image/webp;base64,${await fs.promises.readFile(path.join(fixturesPath, 'video-source-image.webp'), 'base64')}`;
        const encoder = new WebmGenerator(15);
        for (let i = 0; i < 30; i++) encoder.add(imageDataUrl);
        const webm: Uint8Array = await new Promise((resolve) => encoder.compile(resolve));
        const server = http.createServer((req, res) => {
          if (req.url === '/video.webm') {
            res.setHeader('Content-Type', 'video/webm');
            res.end(webm);
          } else {
            res.setHeader('Content-Type', 'text/html');
            res.end(`<video src="/video.webm" muted loop autoplay></video><script>
              window.videoFrames = 0;
              const v = document.querySelector('video');
              (function count () { v.requestVideoFrameCallback(() => { window.videoFrames++; count(); }); })();
            </script>`);
          }
        });
        defer(() => server.close());
        const w = createWindow(true);
        await w.loadURL((await listen(server)).url);
        const videoFramesInHalfSecond = async () => {
          const before = await w.webContents.executeJavaScript('window.videoFrames');
          await setTimeout(500);
          return (await w.webContents.executeJavaScript('window.videoFrames')) - before;
        };
        await waitUntil(async () => (await videoFramesInHalfSecond()) > 0, { timeout: 10000 });
        w.hide();
        // Background video pausing kicks in a moment after the page is hidden.
        await waitUntil(async () => (await videoFramesInHalfSecond()) === 0, { timeout: 15000 });
        w.webContents.setBackgroundThrottling(false);
        await waitUntil(async () => (await videoFramesInHalfSecond()) > 0, { timeout: 10000 });
        w.webContents.setBackgroundThrottling(true);
        await waitUntil(async () => (await videoFramesInHalfSecond()) === 0, { timeout: 15000 });
      });
    });
  });

  describe('getBackgroundThrottling()', () => {
    afterEach(closeAllWindows);
    it('works via getter', () => {
      const w = new BrowserWindow({ show: false });

      w.webContents.setBackgroundThrottling(false);
      expect(w.webContents.getBackgroundThrottling()).toBe(false);

      w.webContents.setBackgroundThrottling(true);
      expect(w.webContents.getBackgroundThrottling()).toBe(true);
    });

    it('works via property', () => {
      const w = new BrowserWindow({ show: false });

      w.webContents.backgroundThrottling = false;
      expect(w.webContents.backgroundThrottling).toBe(false);

      w.webContents.backgroundThrottling = true;
      expect(w.webContents.backgroundThrottling).toBe(true);
    });

    it('works via BrowserWindow', () => {
      const w = new BrowserWindow({ show: false });

      w.setBackgroundThrottling(false);
      expect(w.getBackgroundThrottling()).toBe(false);

      w.setBackgroundThrottling(true);
      expect(w.getBackgroundThrottling()).toBe(true);
    });
  });

  ifdescribe(features.isPrintingEnabled())('getPrintersAsync()', () => {
    afterEach(closeAllWindows);
    it('can get printer list', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { sandbox: true } });
      await w.loadURL('about:blank');
      const printers = await w.webContents.getPrintersAsync();
      expect(printers).toBeInstanceOf(Array);
    });
  });

  ifdescribe(features.isPrintingEnabled())('printToPDF()', () => {
    let server: http.Server | null;

    let w: BrowserWindow;

    beforeEach(() => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          sandbox: true
        }
      });
    });

    afterEach(async () => {
      await closeAllWindows();
      if (server) {
        server.close();
      }
    });

    it('rejects on incorrectly typed parameters', async () => {
      const badTypes = {
        landscape: [],
        displayHeaderFooter: '123',
        printBackground: 2,
        scale: 'not-a-number',
        pageSize: 'IAmAPageSize',
        margins: 'terrible',
        pageRanges: { oops: 'im-not-the-right-key' },
        headerTemplate: [1, 2, 3],
        footerTemplate: [4, 5, 6],
        preferCSSPageSize: 'no',
        generateTaggedPDF: 'wtf',
        generateDocumentOutline: [7, 8, 9]
      };

      await w.loadURL('data:text/html,<h1>Hello, World!</h1>');

      // These will hard crash in Chromium unless we type-check
      for (const [key, value] of Object.entries(badTypes)) {
        const param = { [key]: value };
        await expect(w.webContents.printToPDF(param)).rejects.toThrow();
      }
    });

    it('rejects when margins exceed physical page size', async () => {
      await w.loadURL('data:text/html,<h1>Hello, World!</h1>');

      await expect(
        w.webContents.printToPDF({
          pageSize: 'Letter',
          margins: {
            top: 100,
            bottom: 100,
            left: 5,
            right: 5
          }
        })
      ).rejects.toThrow('margins must be less than or equal to pageSize');
    });

    it('does not crash when called multiple times in parallel', async () => {
      await w.loadURL('data:text/html,<h1>Hello, World!</h1>');

      const promises = [];
      for (let i = 0; i < 3; i++) {
        promises.push(w.webContents.printToPDF({}));
      }

      const results = await Promise.all(promises);
      for (const data of results) {
        expect(data).toBeInstanceOf(Buffer);
        expect(data).not.toHaveLength(0);
      }
    });

    it('does not crash when called multiple times in sequence', async () => {
      await w.loadURL('data:text/html,<h1>Hello, World!</h1>');

      const results = [];
      for (let i = 0; i < 3; i++) {
        const result = await w.webContents.printToPDF({});
        results.push(result);
      }

      for (const data of results) {
        expect(data).toBeInstanceOf(Buffer);
        expect(data).not.toHaveLength(0);
      }
    });

    it('can print a PDF with default settings', async () => {
      await w.loadURL('data:text/html,<h1>Hello, World!</h1>');

      const data = await w.webContents.printToPDF({});
      expect(data).toBeInstanceOf(Buffer);
      expect(data).not.toHaveLength(0);
    });

    type PageSizeString = Exclude<Required<Electron.PrintToPDFOptions>['pageSize'], Electron.Size>;

    it('with custom page sizes', async () => {
      const paperFormats: Record<PageSizeString, ElectronInternal.PageSize> = {
        Letter: { width: 8.5, height: 11 },
        Legal: { width: 8.5, height: 14 },
        Tabloid: { width: 11, height: 17 },
        Ledger: { width: 17, height: 11 },
        A0: { width: 33.1, height: 46.8 },
        A1: { width: 23.4, height: 33.1 },
        A2: { width: 16.54, height: 23.4 },
        A3: { width: 11.7, height: 16.54 },
        A4: { width: 8.27, height: 11.7 },
        A5: { width: 5.83, height: 8.27 },
        A6: { width: 4.13, height: 5.83 }
      };

      await w.loadFile(path.join(import.meta.dirname, 'fixtures', 'api', 'print-to-pdf-small.html'));

      const formats = Object.keys(paperFormats) as PageSizeString[];
      const pdfs = [];
      for (const format of formats) {
        pdfs.push(await w.webContents.printToPDF({ pageSize: format }));
      }

      // Parse every PDF in one pdf.js subprocess: one Electron launch per
      // format is enough to push this test past its timeout on slow CI hosts.
      const pdfInfos = await readPDFs(pdfs);

      const approxEq = (a: number, b: number, epsilon = 0.01) => Math.abs(a - b) <= epsilon;

      for (const [i, format] of formats.entries()) {
        // page.view is [top, left, width, height].
        const width = pdfInfos[i].view[2] / 72;
        const height = pdfInfos[i].view[3] / 72;

        expect(approxEq(width, paperFormats[format].width)).toBe(true);
        expect(approxEq(height, paperFormats[format].height)).toBe(true);
      }
    });

    it('with custom header and footer', async () => {
      await w.loadFile(path.join(fixturesPath, 'api', 'print-to-pdf-small.html'));

      const data = await w.webContents.printToPDF({
        displayHeaderFooter: true,
        headerTemplate: "<div>I'm a PDF header</div>",
        footerTemplate: "<div>I'm a PDF footer</div>"
      });

      const pdfInfo = await readPDF(data);

      expect(containsText(pdfInfo.textContent, /I'm a PDF header/)).toBe(true);
      expect(containsText(pdfInfo.textContent, /I'm a PDF footer/)).toBe(true);
    });

    it('in landscape mode', async () => {
      await w.loadFile(path.join(import.meta.dirname, 'fixtures', 'api', 'print-to-pdf-small.html'));

      const data = await w.webContents.printToPDF({ landscape: true });
      const pdfInfo = await readPDF(data);

      // page.view is [top, left, width, height].
      const width = pdfInfo.view[2];
      const height = pdfInfo.view[3];

      expect(width).toBeGreaterThan(height);
    });

    it('with custom page ranges', async () => {
      await w.loadFile(path.join(import.meta.dirname, 'fixtures', 'api', 'print-to-pdf-large.html'));

      const data = await w.webContents.printToPDF({
        pageRanges: '1-3',
        landscape: true
      });

      const pdfInfo = await readPDF(data);

      // Check that correct # of pages are rendered.
      expect(pdfInfo.numPages).toBe(3);
    });

    it('recovers after a prior call fails with an invalid page range', async () => {
      await w.loadURL('data:text/html,<h1>Hello, World!</h1>');

      await expect(w.webContents.printToPDF({ pageRanges: '999' })).rejects.toThrow();

      const data = await w.webContents.printToPDF({});
      const pdfInfo = await readPDF(data);
      expect(pdfInfo.numPages).toBe(1);
    });

    it('does not tag PDFs by default', async () => {
      await w.loadFile(path.join(import.meta.dirname, 'fixtures', 'api', 'print-to-pdf-small.html'));

      const data = await w.webContents.printToPDF({});
      const pdfInfo = await readPDF(data);
      expect(pdfInfo.markInfo).toBeNull();
    });

    it('can print same-origin iframes', async () => {
      await w.loadFile(path.join(import.meta.dirname, 'fixtures', 'api', 'print-to-pdf-same-origin.html'));

      const data = await w.webContents.printToPDF({});
      const pdfInfo = await readPDF(data);
      expect(containsText(pdfInfo.textContent, /Virtual member functions/)).toBe(true);
    });

    // TODO(codebytere): OOPIF printing is disabled on Linux at the moment due to crashes.
    ifit(process.platform !== 'linux')('can print cross-origin iframes', async () => {
      server = http.createServer((_, res) => {
        res.writeHead(200);
        res.end(`
          <title>cross-origin iframe</title>
          <p>This page is displayed in an iframe.</p>
        `);
      });
      const { port } = await listen(server);

      await w.loadURL(`data:text/html,<iframe src="http://localhost:${port}"></iframe>`);

      const data = await w.webContents.printToPDF({});
      const pdfInfo = await readPDF(data);
      expect(containsText(pdfInfo.textContent, /This page is displayed in an iframe./)).toBe(true);
    });

    it('can generate tag data for PDFs', async () => {
      await w.loadFile(path.join(import.meta.dirname, 'fixtures', 'api', 'print-to-pdf-small.html'));

      const data = await w.webContents.printToPDF({ generateTaggedPDF: true });
      const pdfInfo = await readPDF(data);
      expect(pdfInfo.markInfo).toEqual({
        Marked: true,
        UserProperties: false,
        Suspects: false
      });
    });

    it('from an existing pdf document', async () => {
      const pdfPath = path.join(fixturesPath, 'cat.pdf');
      const readyToPrint = once(w.webContents, '-pdf-ready-to-print');
      await w.loadFile(pdfPath);
      await readyToPrint;
      const data = await w.webContents.printToPDF({});
      const pdfInfo = await readPDF(data);
      expect(pdfInfo.numPages).toBe(2);
      expect(containsText(pdfInfo.textContent, /Cat: The Ideal Pet/)).toBe(true);
    });

    it('from an existing pdf document in a WebView', async () => {
      const win = new BrowserWindow({
        show: false,
        webPreferences: {
          webviewTag: true
        }
      });

      await win.loadURL('about:blank');
      const webContentsCreated = once(app, 'web-contents-created') as Promise<[any, WebContents]>;

      const src = url.format({
        pathname: `${fixturesPath.replaceAll('\\', '/')}/cat.pdf`,
        protocol: 'file',
        slashes: true
      });
      await win.webContents.executeJavaScript(`
        new Promise((resolve, reject) => {
          const webview = new WebView()
          webview.setAttribute('src', '${src}')
          document.body.appendChild(webview)
          webview.addEventListener('did-finish-load', () => {
            resolve()
          })
        })
      `);

      const [, webContents] = await webContentsCreated;

      await once(webContents, '-pdf-ready-to-print');

      const data = await webContents.printToPDF({});
      const pdfInfo = await readPDF(data);
      expect(pdfInfo.numPages).toBe(2);
      expect(containsText(pdfInfo.textContent, /Cat: The Ideal Pet/)).toBe(true);
    });
  });

  describe('PictureInPicture video', () => {
    afterEach(closeAllWindows);
    it('works as expected', async () => {
      const w = new BrowserWindow({ webPreferences: { sandbox: true } });

      // TODO(codebytere): figure out why this workaround is needed and remove.
      // It is not germane to the actual test.
      await w.loadFile(path.join(fixturesPath, 'blank.html'));

      await w.loadFile(path.join(fixturesPath, 'api', 'picture-in-picture.html'));

      await w.webContents.executeJavaScript(
        "document.createElement('video').canPlayType('video/webm; codecs=\"vp8.0\"')",
        true
      );

      const result = await w.webContents.executeJavaScript('runTest(true)', true);
      expect(result).toBe(true);
    });
  });

  describe('Shared Workers', () => {
    afterEach(closeAllWindows);

    it('can get multiple shared workers', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });

      const ready = once(ipcMain, 'ready');
      w.loadFile(path.join(fixturesPath, 'api', 'shared-worker', 'shared-worker.html'));
      await ready;

      const sharedWorkers = w.webContents.getAllSharedWorkers();

      expect(sharedWorkers).toHaveLength(2);
      expect(sharedWorkers[0].url).toContain('shared-worker');
      expect(sharedWorkers[1].url).toContain('shared-worker');
    });

    it('can inspect a specific shared worker', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });

      const ready = once(ipcMain, 'ready');
      w.loadFile(path.join(fixturesPath, 'api', 'shared-worker', 'shared-worker.html'));
      await ready;

      const sharedWorkers = w.webContents.getAllSharedWorkers();

      const devtoolsOpened = once(w.webContents, 'devtools-opened');
      w.webContents.inspectSharedWorkerById(sharedWorkers[0].id);
      await devtoolsOpened;

      const devtoolsClosed = once(w.webContents, 'devtools-closed');
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

    beforeAll(async () => {
      server = http.createServer((request, response) => {
        if (request.url === '/no-auth') {
          return response.end('ok');
        }
        if (request.headers.authorization) {
          response.writeHead(200, { 'Content-type': 'text/plain' });
          return response.end(request.headers.authorization);
        }
        response.writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' }).end('401');
      });
      ({ port: serverPort, url: serverUrl } = await listen(server));
    });

    beforeAll(async () => {
      proxyServer = http.createServer((request, response) => {
        if (request.headers['proxy-authorization']) {
          response.writeHead(200, { 'Content-type': 'text/plain' });
          return response.end(request.headers['proxy-authorization']);
        }
        response.writeHead(407, { 'Proxy-Authenticate': 'Basic realm="Foo"' }).end();
      });
      proxyServerPort = (await listen(proxyServer)).port;
    });

    afterEach(async () => {
      await session.defaultSession.clearAuthCache();
    });

    afterAll(() => {
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
      expect(body).toBe(`Basic ${Buffer.from(`${user}:${pass}`).toString('base64')}`);
      expect(eventRequest.url).toBe(serverUrl + '/');
      expect(eventAuthInfo.isProxy).toBe(false);
      expect(eventAuthInfo.scheme).toBe('basic');
      expect(eventAuthInfo.host).toBe('127.0.0.1');
      expect(eventAuthInfo.port).toBe(serverPort);
      expect(eventAuthInfo.realm).toBe('Foo');
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
      expect(body).toBe(`Basic ${Buffer.from(`${user}:${pass}`).toString('base64')}`);
      expect(eventRequest.url).toBe(`${serverUrl}/no-auth`);
      expect(eventAuthInfo.isProxy).toBe(true);
      expect(eventAuthInfo.scheme).toBe('basic');
      expect(eventAuthInfo.host).toBe('127.0.0.1');
      expect(eventAuthInfo.port).toBe(proxyServerPort);
      expect(eventAuthInfo.realm).toBe('Foo');
    });

    it('cancels authentication when callback is called with no arguments', async () => {
      const w = new BrowserWindow({ show: false });
      w.webContents.on('login', (event, request, authInfo, cb) => {
        event.preventDefault();
        cb();
      });
      await w.loadURL(serverUrl);
      const body = await w.webContents.executeJavaScript('document.documentElement.textContent');
      expect(body).toBe('401');
    });

    it('does not crash when the webContents is destroyed while an auth request is in flight', async () => {
      const w = new BrowserWindow({ show: false });
      const wc = w.webContents;
      const dbg = wc.debugger;
      dbg.attach('1.3');
      const destroyed = once(wc, 'destroyed');
      dbg.on('message', (_e, method, params) => {
        if (method === 'Fetch.requestPaused') {
          dbg.sendCommand('Fetch.continueRequest', { requestId: params.requestId }).catch(() => {});
        } else if (method === 'Fetch.authRequired') {
          // Destroying here queues the WebContents deletion right before the
          // task that emits 'login' for the resumed auth challenge.
          wc.destroy();
          dbg
            .sendCommand('Fetch.continueWithAuth', {
              requestId: params.requestId,
              authChallengeResponse: { response: 'Default' }
            })
            .catch(() => {});
        }
      });
      await dbg.sendCommand('Fetch.enable', {
        patterns: [{ urlPattern: '*', requestStage: 'Request' }],
        handleAuthRequests: true
      });
      wc.loadURL(serverUrl).catch(() => {});
      await destroyed;
      await setTimeout(100);
    });
  });

  describe('page-title-updated event', () => {
    afterEach(closeAllWindows);
    it('is emitted with a full title for pages with no navigation', async () => {
      const bw = new BrowserWindow({ show: false });
      await bw.loadURL('about:blank');
      bw.webContents.executeJavaScript('child = window.open("", "", "show=no"); null');
      const [, child] = (await once(app, 'web-contents-created')) as [any, WebContents];
      bw.webContents.executeJavaScript('child.document.title = "new title"');
      const [, title] = (await once(child, 'page-title-updated')) as [any, string];
      expect(title).toBe('new title');
    });
  });

  describe('context-menu event', () => {
    afterEach(closeAllWindows);
    it('emits when right-clicked in page', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));

      const promise = once(w.webContents, 'context-menu') as Promise<[any, Electron.ContextMenuParams]>;

      // Simulate right-click to create context-menu event.
      const opts = { x: 0, y: 0, button: 'right' as const };
      w.webContents.sendInputEvent({ ...opts, type: 'mouseDown' });
      w.webContents.sendInputEvent({ ...opts, type: 'mouseUp' });

      const [, params] = await promise;

      expect(params.pageURL).toBe(w.webContents.getURL());
      expect(params.frame).toBeTypeOf('object');
      expect(params.x).toBeTypeOf('number');
      expect(params.y).toBeTypeOf('number');
    });

    // Skipping due to lack of native click support.
    it.skip('emits the correct number of times when right-clicked in page', async () => {
      const w = new BrowserWindow({ show: true });
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));

      let contextMenuEmitCount = 0;

      w.webContents.on('context-menu', () => {
        contextMenuEmitCount++;
      });

      // TODO(samuelmaddock): Perform native right-click. We've tried then
      // dropped robotjs and nutjs so for now this is a manual test.

      await once(w.webContents, 'context-menu');
      await setTimeout(100);
      expect(contextMenuEmitCount).toBe(1);
    });

    it('emits when right-clicked in page in a draggable region', async () => {
      const w = new BrowserWindow({ show: false });

      if (process.platform === 'win32') {
        w.on('system-context-menu', (event) => {
          event.preventDefault();
        });
      }

      await w.loadFile(path.join(fixturesPath, 'pages', 'draggable-page.html'));

      const promise = once(w.webContents, 'context-menu') as Promise<[any, Electron.ContextMenuParams]>;

      // Simulate right-click to create context-menu event.
      const midPoint = w.getBounds().width / 2;
      const opts = { x: midPoint, y: midPoint, button: 'right' as const };
      w.webContents.sendInputEvent({ ...opts, type: 'mouseDown' });
      w.webContents.sendInputEvent({ ...opts, type: 'mouseUp' });

      const [, params] = await promise;

      expect(params.pageURL).toBe(w.webContents.getURL());
      expect(params.frame).toBeTypeOf('object');
      expect(params.x).toBeTypeOf('number');
      expect(params.y).toBeTypeOf('number');
    });

    it('emits when right clicked in a WebContentsView', async () => {
      const w = new BaseWindow({ show: false });

      const mainView = new WebContentsView({
        webPreferences: {
          preload: path.join(import.meta.dirname, 'preload.js')
        }
      });

      const draggablePage = path.join(fixturesPath, 'pages', 'draggable-page.html');
      await mainView.webContents.loadFile(draggablePage);

      w.contentView.addChildView(mainView);

      const { width, height } = w.getContentBounds();
      mainView.setBounds({ x: 0, y: 0, width, height });

      const promise = once(mainView.webContents, 'context-menu') as Promise<[any, Electron.ContextMenuParams]>;

      // Simulate right-click to create context-menu event.
      const opts = { x: 0, y: 0, button: 'right' as const };
      mainView.webContents.sendInputEvent({ ...opts, type: 'mouseDown' });
      mainView.webContents.sendInputEvent({ ...opts, type: 'mouseUp' });

      const [, params] = await promise;

      expect(params.pageURL).toBe(mainView.webContents.getURL());
      expect(params.frame).toBeTypeOf('object');
      expect(params.x).toBeTypeOf('number');
      expect(params.y).toBeTypeOf('number');
    });

    it('emits when right clicked in a BrowserWindow with vibrancy', async () => {
      const w = new BrowserWindow({ show: false, vibrancy: 'titlebar' });
      await w.loadFile(path.join(fixturesPath, 'pages', 'draggable-page.html'));

      const promise = once(w.webContents, 'context-menu') as Promise<[any, Electron.ContextMenuParams]>;

      // Simulate right-click to create context-menu event.
      const opts = { x: 0, y: 0, button: 'right' as const };
      w.webContents.sendInputEvent({ ...opts, type: 'mouseDown' });
      w.webContents.sendInputEvent({ ...opts, type: 'mouseUp' });

      const [, params] = await promise;

      expect(params.pageURL).toBe(w.webContents.getURL());
      expect(params.frame).toBeTypeOf('object');
      expect(params.x).toBeTypeOf('number');
      expect(params.y).toBeTypeOf('number');
    });
  });

  describe('close() method', () => {
    afterEach(async () => {
      await closeAllWindows();
      await cleanupWebContents();
    });

    it('closes when close() is called', async () => {
      const w = (webContents as typeof ElectronInternal.WebContents).create();
      const destroyed = once(w, 'destroyed');
      w.close();
      await destroyed;
      expect(w.isDestroyed()).toBe(true);
    });

    it('closes when close() is called after loading a page', async () => {
      const w = (webContents as typeof ElectronInternal.WebContents).create();
      await w.loadURL('about:blank');
      const destroyed = once(w, 'destroyed');
      w.close();
      await destroyed;
      expect(w.isDestroyed()).toBe(true);
    });

    it('can be GCed before loading a page', async () => {
      const v8Util = process._linkedBinding('electron_common_v8_util');
      let registry: FinalizationRegistry<unknown> | null = null;
      const cleanedUp = new Promise<number>((resolve) => {
        registry = new FinalizationRegistry(resolve as any);
      });
      (() => {
        const w = (webContents as typeof ElectronInternal.WebContents).create();
        registry!.register(w, 42);
      })();
      const i = setInterval(() => v8Util.requestGarbageCollectionForTesting(), 100);
      defer(() => clearInterval(i));
      expect(await cleanedUp).toBe(42);
    });

    it('causes its parent browserwindow to be closed', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');
      const closed = once(w, 'closed');
      w.webContents.close();
      await closed;
      expect(w.isDestroyed()).toBe(true);
    });

    it('ignores beforeunload if waitForBeforeUnload not specified', async () => {
      const w = (webContents as typeof ElectronInternal.WebContents).create();
      await w.loadURL('about:blank');
      await w.executeJavaScript('window.onbeforeunload = () => "hello"; null');
      w.on('will-prevent-unload', () => {
        throw new Error('unexpected will-prevent-unload');
      });
      const destroyed = once(w, 'destroyed');
      w.close();
      await destroyed;
      expect(w.isDestroyed()).toBe(true);
    });

    it('runs beforeunload if waitForBeforeUnload is specified', async () => {
      const w = (webContents as typeof ElectronInternal.WebContents).create();
      await w.loadURL('about:blank');
      await w.executeJavaScript('window.onbeforeunload = () => "hello"; null');
      const willPreventUnload = once(w, 'will-prevent-unload');
      w.close({ waitForBeforeUnload: true });
      await willPreventUnload;
      expect(w.isDestroyed()).toBe(false);
    });

    it('overriding beforeunload prevention results in webcontents close', async () => {
      const w = (webContents as typeof ElectronInternal.WebContents).create();
      await w.loadURL('about:blank');
      await w.executeJavaScript('window.onbeforeunload = () => "hello"; null');
      w.once('will-prevent-unload', (e) => e.preventDefault());
      const destroyed = once(w, 'destroyed');
      w.close({ waitForBeforeUnload: true });
      await destroyed;
      expect(w.isDestroyed()).toBe(true);
    });
  });

  describe('content-bounds-updated event', () => {
    afterEach(closeAllWindows);
    it('emits when moveTo is called', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadURL('about:blank');
      w.webContents.executeJavaScript('window.moveTo(50, 50)', true);
      const [, rect] = (await once(w.webContents, 'content-bounds-updated')) as [any, Electron.Rectangle];
      const { width, height } = w.getBounds();
      expect(rect).toEqual({
        x: 50,
        y: 50,
        width,
        height
      });
      await new Promise(setImmediate);
      expect(w.getBounds().x).toBe(50);
      expect(w.getBounds().y).toBe(50);
    });

    it('emits when resizeTo is called', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadURL('about:blank');
      w.webContents.executeJavaScript('window.resizeTo(100, 100)', true);
      const [, rect] = (await once(w.webContents, 'content-bounds-updated')) as [any, Electron.Rectangle];
      const { x, y } = w.getBounds();
      expect(rect).toEqual({
        x,
        y,
        width: 100,
        height: 100
      });
      await new Promise(setImmediate);
      expect({
        width: w.getBounds().width,
        height: w.getBounds().height
      }).toEqual(
        process.platform === 'win32'
          ? {
              // The width is reported as being larger on Windows? I'm not sure why
              // this is.
              width: 136,
              height: 100
            }
          : {
              width: 100,
              height: 100
            }
      );
    });

    it('does not change window bounds if cancelled', async () => {
      const w = new BrowserWindow({ show: false });
      const { width, height } = w.getBounds();
      w.loadURL('about:blank');
      w.webContents.once('content-bounds-updated', (e) => e.preventDefault());
      await w.webContents.executeJavaScript('window.resizeTo(100, 100)', true);
      await new Promise(setImmediate);
      expect(w.getBounds().width).toBe(width);
      expect(w.getBounds().height).toBe(height);
    });
  });
});

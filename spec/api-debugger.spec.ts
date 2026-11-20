import { BrowserWindow, WebContentsView } from 'electron/main';

import { afterEach, beforeEach, describe, expect, it } from 'vitest';

import { once } from 'node:events';
import * as http from 'node:http';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';

import { emittedUntil } from './lib/events-helpers.ts';
import { defer, ifit, listen, startRemoteControlApp, waitUntil } from './lib/spec-helpers.ts';
import { closeAllWindows } from './lib/window-helpers.ts';

describe('debugger module', () => {
  const fixtures = path.resolve(import.meta.dirname, 'fixtures');
  let w: BrowserWindow;

  beforeEach(() => {
    w = new BrowserWindow({
      show: false,
      width: 400,
      height: 400
    });
  });

  afterEach(closeAllWindows);

  describe('debugger.attach', () => {
    it('succeeds when devtools is already open', async () => {
      await w.webContents.loadURL('about:blank');
      w.webContents.openDevTools();
      w.webContents.debugger.attach();
      expect(w.webContents.debugger.isAttached()).toBe(true);
    });

    it('fails when protocol version is not supported', () => {
      expect(() => w.webContents.debugger.attach('2.0')).toThrow();
      expect(w.webContents.debugger.isAttached()).toBe(false);
    });

    it('attaches when no protocol version is specified', () => {
      w.webContents.debugger.attach();
      expect(w.webContents.debugger.isAttached()).toBe(true);
    });

    it('throws when the webContents has been destroyed', async () => {
      const { debugger: dbg } = w.webContents;
      w.webContents.destroy();
      await once(w.webContents, 'destroyed');
      await setTimeout(50);
      expect(() => dbg.attach()).toThrow('No target available');
      expect(dbg.isAttached()).toBe(false);
    });
  });

  describe('debugger.detach', () => {
    it('fires detach event', async () => {
      const detach = once(w.webContents.debugger, 'detach');
      w.webContents.debugger.attach();
      w.webContents.debugger.detach();
      const [, reason] = await detach;
      expect(reason).toBe('target closed');
      expect(w.webContents.debugger.isAttached()).toBe(false);
    });

    it("doesn't disconnect an active devtools session", async () => {
      w.webContents.loadURL('about:blank');
      const detach = once(w.webContents.debugger, 'detach');
      w.webContents.debugger.attach();
      w.webContents.openDevTools();
      w.webContents.once('devtools-opened', () => {
        w.webContents.debugger.detach();
      });
      await detach;
      expect(w.webContents.debugger.isAttached()).toBe(false);
      expect(w.devToolsWebContents.isDestroyed()).toBe(false);
    });

    it('clears device metrics overrides left behind by the session', async () => {
      await w.webContents.loadURL('about:blank');
      const innerSize = () => w.webContents.executeJavaScript('window.innerWidth + "x" + window.innerHeight');

      w.webContents.debugger.attach();
      await w.webContents.debugger.sendCommand('Emulation.setDeviceMetricsOverride', {
        width: 200,
        height: 150,
        deviceScaleFactor: 0,
        mobile: false
      });
      expect(await innerSize()).toBe('200x150');

      // Detaching without Emulation.clearDeviceMetricsOverride used to leave
      // the view pinned at the emulated size.
      w.webContents.debugger.detach();
      await waitUntil(async () => (await innerSize()) !== '200x150');
    });

    it('fires detach event for a webContents that is still attached at quit', async () => {
      const rc = await startRemoteControlApp();
      let stdout = '';
      rc.process.stdout!.on('data', (chunk) => {
        stdout += chunk;
      });
      await rc.remotely(async () => {
        const { app, webContents } = require('electron');
        const contents = (webContents as any).create();
        await contents.loadURL('about:blank');
        contents.debugger.on('detach', (_event: unknown, reason: string) => {
          process.stdout.write(`debugger-detach: ${reason}\n`);
        });
        contents.debugger.attach();
        (globalThis as any).contents = contents;
        setImmediate(() => app.quit());
      });
      const [code] = await once(rc.process, 'close');
      expect(code).toBe(0);
      expect(stdout).toContain('debugger-detach: target closed');
    });
  });

  describe('debugger.sendCommand', () => {
    let server: http.Server;

    afterEach(() => {
      if (server != null) {
        server.close();
        server = null as any;
      }
    });

    it('returns response', async () => {
      w.webContents.loadURL('about:blank');
      w.webContents.debugger.attach();

      const params = { expression: '4+2' };
      const res = await w.webContents.debugger.sendCommand('Runtime.evaluate', params);

      expect(res.wasThrown).toBeUndefined();
      expect(res.result.value).toBe(6);

      w.webContents.debugger.detach();
    });

    it('returns response when devtools is opened', async () => {
      w.webContents.loadURL('about:blank');
      w.webContents.debugger.attach();

      const opened = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools();
      await opened;

      const params = { expression: '4+2' };
      const res = await w.webContents.debugger.sendCommand('Runtime.evaluate', params);

      expect(res.wasThrown).toBeUndefined();
      expect(res.result.value).toBe(6);

      w.webContents.debugger.detach();
    });

    it('fires message event', async () => {
      const url =
        process.platform !== 'win32'
          ? `file://${path.join(fixtures, 'pages', 'a.html')}`
          : `file:///${path.join(fixtures, 'pages', 'a.html').replaceAll('\\', '/')}`;
      w.webContents.loadURL(url);
      w.webContents.debugger.attach();
      const message = emittedUntil(
        w.webContents.debugger,
        'message',
        (event: Electron.Event, method: string) => method === 'Console.messageAdded'
      );
      w.webContents.debugger.sendCommand('Console.enable');
      const [, , params] = await message;
      w.webContents.debugger.detach();
      expect(params.message.level).toBe('log');
      expect(params.message.url).toBe(url);
      expect(params.message.text).toBe('a');
    });

    it('returns error message when command fails', async () => {
      w.webContents.loadURL('about:blank');
      w.webContents.debugger.attach();

      const promise = w.webContents.debugger.sendCommand('Test');
      await expect(promise).rejects.toThrow("'Test' wasn't found");

      w.webContents.debugger.detach();
    });

    it('rejects a non-string value for a string parameter', async () => {
      // Sends a number for Input.dispatchKeyEvent's optional `text` string param,
      // which must reject rather than abort the browser process. See
      // spec/fixtures/crash-cases/debugger-send-command-wrong-type.
      w.webContents.loadURL('about:blank');
      w.webContents.debugger.attach();

      const promise = w.webContents.debugger.sendCommand('Input.dispatchKeyEvent', {
        type: 'keyDown',
        text: 1 as any
      });
      await expect(promise).rejects.toThrow();

      w.webContents.debugger.detach();
    });

    it('handles valid unicode characters in message', async () => {
      server = http.createServer((req, res) => {
        res.setHeader('Content-Type', 'text/plain; charset=utf-8');
        res.end('\u0024');
      });

      const { url } = await listen(server);
      // Network.enable only reaches the renderer once it exists. Sent during
      // the first navigation of a fresh window, it can arrive after the
      // response, so Network.responseReceived never fires. Load a page first
      // and wait for Network.enable before the real navigation.
      await w.loadURL('about:blank');
      w.webContents.debugger.attach();
      await w.webContents.debugger.sendCommand('Network.enable');

      let requestId: string | undefined;
      const loadingFinished = emittedUntil(
        w.webContents.debugger,
        'message',
        (_event: any, method: string, params: any) => {
          if (method === 'Network.responseReceived' && params.response.url.startsWith('http://127.0.0.1')) {
            requestId = params.requestId;
          }
          return method === 'Network.loadingFinished' && requestId !== undefined && params.requestId === requestId;
        }
      );
      await w.loadURL(url);
      await loadingFinished;
      const { body } = await w.webContents.debugger.sendCommand('Network.getResponseBody', { requestId });
      expect(body).toBe('\u0024');
    });

    it('does not crash for invalid unicode characters in message', async () => {
      server = http.createServer((req, res) => {
        res.setHeader('Content-Type', 'text/plain; charset=utf-8');
        res.end('\uFFFF');
      });

      const { url } = await listen(server);
      // Network.enable only reaches the renderer once it exists. On a fresh
      // window that is during the first navigation, which can then finish
      // before the renderer reports Network.loadingFinished. Load a page
      // first and wait for Network.enable before the real navigation.
      await w.loadURL('about:blank');
      w.webContents.debugger.attach();
      await w.webContents.debugger.sendCommand('Network.enable');

      // loadingFinished indicates that page has been loaded and it did not
      // crash because of invalid UTF-8 data
      const loadingFinished = emittedUntil(
        w.webContents.debugger,
        'message',
        (_event: any, method: string) => method === 'Network.loadingFinished'
      );
      await w.loadURL(url);
      await loadingFinished;
    });

    it('can continue a Fetch-paused document navigation without webRequest listeners', async () => {
      server = http.createServer((_req, res) => {
        res.setHeader('Content-Type', 'text/html; charset=utf-8');
        res.end('<!DOCTYPE html><html><body><h1>Hello World</h1></body></html>');
      });

      const { port } = await listen(server);
      const url = `http://localhost:${port}`;
      const continueRequests: Array<Promise<any>> = [];

      const onMessage = (_event: Electron.Event, method: string, params: any) => {
        if (method === 'Fetch.requestPaused') {
          continueRequests.push(
            w.webContents.debugger.sendCommand('Fetch.continueRequest', {
              requestId: params.requestId
            })
          );
        }
      };

      w.webContents.debugger.attach();
      w.webContents.debugger.on('message', onMessage);

      try {
        await w.webContents.debugger.sendCommand('Fetch.enable', {
          patterns: [{ resourceType: 'Document' }]
        });

        await w.loadURL(url);
        await Promise.all(continueRequests);
      } finally {
        w.webContents.debugger.off('message', onMessage);
        if (w.webContents.debugger.isAttached()) {
          w.webContents.debugger.detach();
        }
      }
    });

    it('reports network requests for a new document after a main-frame navigation', async () => {
      server = http.createServer((req, res) => {
        if (req.url === '/page') {
          res.setHeader('Content-Type', 'text/html; charset=utf-8');
          res.end(
            '<!DOCTYPE html><html><head>' +
              '<link rel="stylesheet" href="/style.css">' +
              '<script src="/script.js"></script>' +
              '</head><body>page</body></html>'
          );
        } else if (req.url === '/style.css') {
          res.setHeader('Content-Type', 'text/css; charset=utf-8');
          res.end('body { color: red; }');
        } else if (req.url === '/script.js') {
          res.setHeader('Content-Type', 'text/javascript; charset=utf-8');
          res.end('void 0;');
        } else {
          res.setHeader('Content-Type', 'text/html; charset=utf-8');
          res.end('<!DOCTYPE html><html><body>start</body></html>');
        }
      });

      const { url } = await listen(server);
      await w.loadURL(`${url}/start`);
      w.webContents.debugger.attach();
      try {
        await w.webContents.debugger.sendCommand('Network.enable');
        await w.webContents.debugger.sendCommand('Page.enable');
        await w.webContents.debugger.sendCommand('Target.setAutoAttach', {
          autoAttach: true,
          flatten: true,
          waitForDebuggerOnStart: false
        });

        const requestedUrls = new Set<string>();
        const loadEvent = new Promise<void>((resolve) => {
          w.webContents.debugger.on('message', (_event, method, params) => {
            if (method === 'Network.requestWillBeSent') {
              requestedUrls.add(params.request.url);
            } else if (method === 'Page.loadEventFired') {
              resolve();
            }
          });
        });

        // This navigation swaps the main-frame RenderFrameHost under
        // RenderDocument. Before the fix, the debugger tore down and rebound its
        // session here, dropping these subresource requests.
        await w.loadURL(`${url}/page`);

        await loadEvent;
        expect(requestedUrls).toContain(`${url}/style.css`);
        expect(requestedUrls).toContain(`${url}/script.js`);
      } finally {
        if (w.webContents.debugger.isAttached()) {
          w.webContents.debugger.detach();
        }
      }
    });

    it('can get and set cookies using the Storage API', async () => {
      await w.webContents.loadURL('about:blank');
      w.webContents.debugger.attach('1.1');

      await w.webContents.debugger.sendCommand('Storage.clearCookies', {});
      await w.webContents.debugger.sendCommand('Storage.setCookies', {
        cookies: [
          {
            name: 'cookieOne',
            value: 'cookieValueOne',
            url: 'https://cookieone.com'
          },
          {
            name: 'cookieTwo',
            value: 'cookieValueTwo',
            url: 'https://cookietwo.com'
          }
        ]
      });

      const { cookies } = await w.webContents.debugger.sendCommand('Storage.getCookies', {});
      expect(cookies).toHaveLength(2);

      const cookieOne = cookies.find((cookie: any) => cookie.name === 'cookieOne');
      expect(cookieOne.domain).toBe('cookieone.com');
      expect(cookieOne.value).toBe('cookieValueOne');

      const cookieTwo = cookies.find((cookie: any) => cookie.name === 'cookieTwo');
      expect(cookieTwo.domain).toBe('cookietwo.com');
      expect(cookieTwo.value).toBe('cookieValueTwo');
    });

    it('uses empty sessionId by default', async () => {
      w.webContents.loadURL('about:blank');
      w.webContents.debugger.attach();
      const onMessage = once(w.webContents.debugger, 'message');
      await w.webContents.debugger.sendCommand('Target.setDiscoverTargets', { discover: true });
      const [, method, params, sessionId] = await onMessage;
      expect(method).toBe('Target.targetCreated');
      expect(params.targetInfo.targetId).not.toHaveLength(0);
      expect(sessionId).toHaveLength(0);
      w.webContents.debugger.detach();
    });

    it('creates unique session id for each target', () =>
      new Promise<void>((resolve) => {
        w.webContents.loadFile(path.join(import.meta.dirname, 'fixtures', 'sub-frames', 'debug-frames.html'));
        w.webContents.debugger.attach();
        let debuggerSessionId: string;

        w.webContents.debugger.on('message', (_event, ...args) => {
          const [method, params, sessionId] = args;
          if (method === 'Target.targetCreated') {
            w.webContents.debugger
              .sendCommand('Target.attachToTarget', { targetId: params.targetInfo.targetId, flatten: true })
              .then((result) => {
                debuggerSessionId = result.sessionId;
                w.webContents.debugger.sendCommand('Debugger.enable', {}, result.sessionId);

                // Ensure debugger finds a script to pause to possibly reduce flaky
                // tests.
                w.webContents.mainFrame.executeJavaScript('void 0;');
              });
          }
          if (method === 'Debugger.scriptParsed') {
            if (sessionId === debuggerSessionId) {
              w.webContents.debugger.detach();
              resolve();
            }
          }
        });
        w.webContents.debugger.sendCommand('Target.setDiscoverTargets', { discover: true });
      }));
  });

  describe('Browser window commands', () => {
    const toCdpBounds = ({ x, y, width, height }: Electron.Rectangle) => ({ left: x, top: y, width, height });

    const expectBoundsClose = (actual: Electron.Rectangle, expected: Electron.Rectangle) => {
      // Allow for rounding when the display's scale factor is fractional.
      for (const key of ['x', 'y', 'width', 'height'] as const) {
        expect(Math.abs(actual[key] - expected[key]), key).toBeLessThanOrEqual(1);
      }
    };

    const getWindowState = async () => {
      const { bounds } = await w.webContents.debugger.sendCommand('Browser.getWindowBounds', { windowId: w.id });
      return bounds.windowState;
    };

    const setWindowBounds = (bounds: Record<string, unknown>) =>
      w.webContents.debugger.sendCommand('Browser.setWindowBounds', { windowId: w.id, bounds });

    beforeEach(async () => {
      await w.loadURL('about:blank');
      w.webContents.debugger.attach();
    });

    it('getWindowForTarget returns the window of the attached target', async () => {
      const { windowId, bounds } = await w.webContents.debugger.sendCommand('Browser.getWindowForTarget');
      expect(windowId).toBe(w.id);
      expect(bounds).toEqual({ ...toCdpBounds(w.getBounds()), windowState: 'normal' });
    });

    it('getWindowForTarget accepts a targetId', async () => {
      const { targetInfo } = await w.webContents.debugger.sendCommand('Target.getTargetInfo');
      const { windowId } = await w.webContents.debugger.sendCommand('Browser.getWindowForTarget', {
        targetId: targetInfo.targetId
      });
      expect(windowId).toBe(w.id);
    });

    it('getWindowForTarget rejects an unknown targetId', async () => {
      const promise = w.webContents.debugger.sendCommand('Browser.getWindowForTarget', { targetId: 'unknown' });
      await expect(promise).rejects.toThrow('No target with given id');
    });

    it('getWindowBounds returns the bounds of a window', async () => {
      const { bounds } = await w.webContents.debugger.sendCommand('Browser.getWindowBounds', { windowId: w.id });
      expect(bounds).toEqual({ ...toCdpBounds(w.getBounds()), windowState: 'normal' });
    });

    it('getWindowBounds rejects an unknown windowId', async () => {
      const promise = w.webContents.debugger.sendCommand('Browser.getWindowBounds', { windowId: -1 });
      await expect(promise).rejects.toThrow('Browser window not found');
    });

    it('setWindowBounds sets the window bounds', async () => {
      await setWindowBounds({ left: 100, top: 120, width: 300, height: 350 });
      expectBoundsClose(w.getBounds(), { x: 100, y: 120, width: 300, height: 350 });
    });

    it('setWindowBounds keeps the bounds that are not given', async () => {
      const original = w.getBounds();
      await setWindowBounds({ width: original.width + 20 });
      expectBoundsClose(w.getBounds(), { ...original, width: original.width + 20 });
    });

    it('setWindowBounds rejects a window state combined with bounds', async () => {
      await expect(setWindowBounds({ width: 300, windowState: 'maximized' })).rejects.toThrow(
        "The 'minimized', 'maximized' and 'fullscreen' states cannot be combined"
      );
    });

    it('setWindowBounds rejects an invalid window state', async () => {
      await expect(setWindowBounds({ windowState: 'tiled' })).rejects.toThrow('Invalid window state: tiled');
    });

    it('setWindowBounds rejects an unknown windowId', async () => {
      const promise = w.webContents.debugger.sendCommand('Browser.setWindowBounds', {
        windowId: -1,
        bounds: { width: 300 }
      });
      await expect(promise).rejects.toThrow('Browser window not found');
    });

    it('getWindowForTarget returns the window that hosts a WebContentsView', async () => {
      const view = new WebContentsView();
      defer(() => view.webContents.close());
      w.contentView.addChildView(view);
      await view.webContents.loadURL('about:blank');
      view.webContents.debugger.attach();
      const { windowId } = await view.webContents.debugger.sendCommand('Browser.getWindowForTarget');
      expect(windowId).toBe(w.id);
    });

    it('getWindowForTarget follows a WebContentsView that moves to another window', async () => {
      const other = new BrowserWindow({ show: false, width: 400, height: 400 });
      const view = new WebContentsView();
      defer(() => view.webContents.close());
      w.contentView.addChildView(view);
      await view.webContents.loadURL('about:blank');
      view.webContents.debugger.attach();

      w.contentView.removeChildView(view);
      other.contentView.addChildView(view);
      const { windowId } = await view.webContents.debugger.sendCommand('Browser.getWindowForTarget');
      expect(windowId).toBe(other.id);

      // Closing the window the view came from does not affect it.
      const closed = once(w, 'closed');
      w.close();
      await closed;
      const { windowId: windowIdAfterClose } =
        await view.webContents.debugger.sendCommand('Browser.getWindowForTarget');
      expect(windowIdAfterClose).toBe(other.id);
    });

    const openDevTools = async (mode: Electron.OpenDevToolsOptions['mode']) => {
      const opened = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools({ mode, activate: false });
      await opened;
      const frontend = w.webContents.devToolsWebContents!;
      frontend.debugger.attach();
      return frontend;
    };

    // Like Chrome, where a DevTools frontend is never a tab.
    it('getWindowForTarget does not find a window for docked DevTools', async () => {
      const frontend = await openDevTools('right');
      await expect(frontend.debugger.sendCommand('Browser.getWindowForTarget')).rejects.toThrow(
        'Browser window not found'
      );
    });

    it('getWindowForTarget does not find a window for undocked DevTools', async () => {
      const frontend = await openDevTools('undocked');
      await expect(frontend.debugger.sendCommand('Browser.getWindowForTarget')).rejects.toThrow(
        'Browser window not found'
      );
    });

    it('getWindowForTarget returns the own window of a custom DevTools frontend', async () => {
      const devtools = new BrowserWindow({ show: false });
      const domReady = once(devtools.webContents, 'dom-ready');
      w.webContents.setDevToolsWebContents(devtools.webContents);
      w.webContents.openDevTools();
      await domReady;
      devtools.webContents.debugger.attach();
      const { windowId } = await devtools.webContents.debugger.sendCommand('Browser.getWindowForTarget');
      expect(windowId).toBe(devtools.id);
    });

    const showWindow = async () => {
      const shown = once(w, 'show');
      w.show();
      await shown;
    };

    // Like the BrowserWindow specs, the maximize, minimize and fullscreen
    // transitions are skipped on Linux, where those events are unreliable in
    // CI.
    ifit(process.platform !== 'linux')('setWindowBounds maximizes and restores the window', async () => {
      await showWindow();
      const maximize = once(w, 'maximize');
      await setWindowBounds({ windowState: 'maximized' });
      await maximize;
      expect(await getWindowState()).toBe('maximized');

      // Going back to normal from maximized has to unmaximize the window.
      const unmaximize = once(w, 'unmaximize');
      await setWindowBounds({ windowState: 'normal' });
      await unmaximize;
      expect(w.isMaximized()).toBe(false);
      expect(await getWindowState()).toBe('normal');
    });

    ifit(process.platform !== 'linux')('setWindowBounds minimizes and restores the window', async () => {
      await showWindow();
      const normalBounds = w.getBounds();
      const minimize = once(w, 'minimize');
      await setWindowBounds({ windowState: 'minimized' });
      await minimize;

      // Like Chrome, a minimized window reports its restored bounds.
      const { bounds } = await w.webContents.debugger.sendCommand('Browser.getWindowBounds', { windowId: w.id });
      expect(bounds.windowState).toBe('minimized');
      expectBoundsClose({ x: bounds.left, y: bounds.top, width: bounds.width, height: bounds.height }, normalBounds);

      await expect(setWindowBounds({ windowState: 'fullscreen' })).rejects.toThrow(
        'To make minimized window fullscreen, restore it to normal state first.'
      );
      await expect(setWindowBounds({ windowState: 'maximized' })).rejects.toThrow(
        'To maximize a minimized or fullscreen window, restore it to normal state first.'
      );

      const restore = once(w, 'restore');
      await setWindowBounds({ windowState: 'normal' });
      await restore;
      expect(w.isMinimized()).toBe(false);
      expect(await getWindowState()).toBe('normal');
    });

    ifit(process.platform !== 'linux')(
      'setWindowBounds enters and leaves fullscreen',
      { tags: ['serial'] },
      async () => {
        await showWindow();
        const enterFullScreen = once(w, 'enter-full-screen');
        await setWindowBounds({ windowState: 'fullscreen' });
        await enterFullScreen;
        expect(await getWindowState()).toBe('fullscreen');

        await expect(setWindowBounds({ windowState: 'maximized' })).rejects.toThrow(
          'To maximize a minimized or fullscreen window, restore it to normal state first.'
        );
        await expect(setWindowBounds({ windowState: 'minimized' })).rejects.toThrow(
          'To minimize a fullscreen window, restore it to normal state first.'
        );

        const leaveFullScreen = once(w, 'leave-full-screen');
        await setWindowBounds({ windowState: 'normal' });
        await leaveFullScreen;
        expect(w.isFullScreen()).toBe(false);
        expect(await getWindowState()).toBe('normal');
      }
    );

    it('setWindowBounds rejects fullscreen for a window that is not fullscreenable', async () => {
      w.fullScreenable = false;
      await expect(setWindowBounds({ windowState: 'fullscreen' })).rejects.toThrow('Window is not fullscreenable');
      expect(w.isFullScreen()).toBe(false);
      expect(await getWindowState()).toBe('normal');
    });

    // A window event can be emitted while the command is still being handled,
    // and its listener can detach the client that sent the command.
    const expectDetachDuringCommand = async (event: string, bounds: Record<string, unknown>) => {
      const detached = new Promise<void>((resolve) => {
        w.once(event as any, () => {
          w.webContents.debugger.detach();
          resolve();
        });
      });
      const outcome = setWindowBounds(bounds).then(
        () => 'replied',
        (error: Error) => error.message
      );
      await detached;
      // The reply is dropped when the listener ran first; either way, the
      // command settles without crashing.
      expect(['replied', 'target closed while handling command']).toContain(await outcome);
      expect(w.webContents.debugger.isAttached()).toBe(false);

      w.webContents.debugger.attach();
      const { bounds: current } = await w.webContents.debugger.sendCommand('Browser.getWindowBounds', {
        windowId: w.id
      });
      expect(current.windowState).toBeDefined();
    };

    it('setWindowBounds does not crash when a resize listener detaches the debugger', async () => {
      await expectDetachDuringCommand('resize', { width: w.getBounds().width + 20 });
    });

    // The maximize event is emitted synchronously on Windows and macOS.
    ifit(process.platform === 'win32' || process.platform === 'darwin')(
      'setWindowBounds does not crash when a maximize listener detaches the debugger',
      async () => {
        await showWindow();
        await expectDetachDuringCommand('maximize', { windowState: 'maximized' });
      }
    );
  });
});

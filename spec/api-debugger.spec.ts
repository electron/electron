import { BrowserWindow } from 'electron/main';

import { afterEach, beforeEach, describe, expect, it } from 'vitest';

import { once } from 'node:events';
import * as http from 'node:http';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';

import { emittedUntil } from './lib/events-helpers.ts';
import { listen, waitUntil } from './lib/spec-helpers.ts';
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
      w.loadURL(url);
      // If we do this synchronously, it's fast enough to attach and enable
      // network capture before the load. If we do it before the loadURL, for
      // some reason network capture doesn't get enabled soon enough and we get
      // an error when calling `Network.getResponseBody`.
      w.webContents.debugger.attach();
      w.webContents.debugger.sendCommand('Network.enable');
      const [, , { requestId }] = await emittedUntil(
        w.webContents.debugger,
        'message',
        (_event: any, method: string, params: any) =>
          method === 'Network.responseReceived' && params.response.url.startsWith('http://127.0.0.1')
      );
      await emittedUntil(
        w.webContents.debugger,
        'message',
        (_event: any, method: string, params: any) =>
          method === 'Network.loadingFinished' && params.requestId === requestId
      );
      const { body } = await w.webContents.debugger.sendCommand('Network.getResponseBody', { requestId });
      expect(body).toBe('\u0024');
    });

    it('does not crash for invalid unicode characters in message', async () => {
      w.webContents.debugger.attach();

      const loadingFinished = new Promise<void>((resolve) => {
        w.webContents.debugger.on('message', (event, method) => {
          // loadingFinished indicates that page has been loaded and it did not
          // crash because of invalid UTF-8 data
          if (method === 'Network.loadingFinished') {
            resolve();
          }
        });
      });

      server = http.createServer((req, res) => {
        res.setHeader('Content-Type', 'text/plain; charset=utf-8');
        res.end('\uFFFF');
      });

      const { url } = await listen(server);
      w.webContents.debugger.sendCommand('Network.enable');
      w.loadURL(url);

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
});

import type { MediaAccessPermissionRequest } from 'electron';
import {
  BrowserWindow,
  type WebContents,
  webFrameMain,
  session,
  ipcMain,
  app,
  clipboard,
  ClipboardItem,
  protocol,
  webContents,
  dialog,
  type MessageBoxOptions
} from 'electron/main';

import { afterAll, afterEach, beforeAll, beforeEach, describe, expect, it } from 'vitest';
import { WebSocket, WebSocketServer } from 'ws';

import * as ChildProcess from 'node:child_process';
import { EventEmitter, once } from 'node:events';
import * as fs from 'node:fs';
import * as http from 'node:http';
import * as https from 'node:https';
import * as os from 'node:os';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';
import * as url from 'node:url';

import { emittedUntil } from './lib/events-helpers.ts';
import {
  ifit,
  ifdescribe,
  defer,
  focusWebContents,
  itremote,
  listen,
  startRemoteControlApp,
  waitUntil
} from './lib/spec-helpers.ts';
import { closeAllWindows } from './lib/window-helpers.ts';
import { PipeTransport } from './pipe-transport.ts';

import type { AddressInfo } from 'node:net';

const features = process._linkedBinding('electron_common_features');

const fixturesPath = path.resolve(import.meta.dirname, 'fixtures');
const certPath = path.join(fixturesPath, 'certificates');

describe('reporting api', () => {
  it('sends a report for an intervention', async () => {
    const reporting = new EventEmitter();
    const ses = session.fromPartition(`reporting-${Math.random()}`);

    // The Reporting API only works on https with valid certs. To dodge having
    // to set up a trusted certificate, hack the validator.
    ses.setCertificateVerifyProc((req, cb) => {
      cb(0);
    });

    const options = {
      key: fs.readFileSync(path.join(certPath, 'server.key')),
      cert: fs.readFileSync(path.join(certPath, 'server.pem')),
      ca: [
        fs.readFileSync(path.join(certPath, 'rootCA.pem')),
        fs.readFileSync(path.join(certPath, 'intermediateCA.pem'))
      ],
      requestCert: true,
      rejectUnauthorized: false
    };

    const server = https.createServer(options, (req, res) => {
      if (req.url?.endsWith('report')) {
        let data = '';
        req.on('data', (d) => {
          data += d.toString('utf-8');
        });
        req.on('end', () => {
          reporting.emit('report', JSON.parse(data));
        });
      }

      const { port } = server.address() as any;
      res.setHeader('Reporting-Endpoints', `default="https://localhost:${port}/report"`);
      res.setHeader('Content-Type', 'text/html');

      res.end('<script>window.navigator.vibrate(1)</script>');
    });

    await listen(server);
    const bw = new BrowserWindow({ show: false, webPreferences: { session: ses } });
    const pageUrl = `https://localhost:${(server.address() as AddressInfo).port}/a`;

    try {
      const reportGenerated = once(reporting, 'report');
      await bw.loadURL(pageUrl);

      const [reports] = await reportGenerated;
      expect(reports).toBeInstanceOf(Array);
      expect(reports).toHaveLength(1);
      const { type, url, body } = reports[0];
      expect(type).toBe('intervention');
      expect(url).toBe(pageUrl);
      expect(body.id).toBe('NavigatorVibrate');
      expect(body.message).toMatch(
        /Blocked call to navigator.vibrate because user hasn't tapped on the frame or any embedded frame yet/
      );
    } finally {
      bw.destroy();
      server.close();
      ses.setCertificateVerifyProc(null);
    }
  });
});

describe('window.postMessage', () => {
  afterEach(async () => {
    await closeAllWindows();
  });

  it('sets the source and origin correctly', async () => {
    const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
    w.loadURL(`file://${fixturesPath}/pages/window-open-postMessage-driver.html`);
    const [, message] = await once(ipcMain, 'complete');
    expect(message.data).toBe('testing');
    expect(message.origin).toBe('null');
    expect(message.sourceEqualsOpener).toBe(true);
    expect(message.eventOrigin).toBe('null');
  });
});

describe('focus handling', { tags: ['serial'] }, () => {
  let webviewContents: WebContents;
  let w: BrowserWindow;

  beforeEach(async () => {
    w = new BrowserWindow({
      alwaysOnTop: true,
      show: true,
      webPreferences: {
        nodeIntegration: true,
        webviewTag: true,
        contextIsolation: false
      }
    });

    const webviewReady = once(w.webContents, 'did-attach-webview') as Promise<[any, WebContents]>;
    await w.loadFile(path.join(fixturesPath, 'pages', 'tab-focus-loop-elements.html'));
    const [, wvContents] = await webviewReady;
    webviewContents = wvContents;
    await once(webviewContents, 'did-finish-load');
    w.focus();
  });

  afterEach(() => {
    webviewContents = null as unknown as WebContents;
    w.destroy();
    w = null as unknown as BrowserWindow;
  });

  const expectFocusChange = async () => {
    const [, focusedElementId] = await once(ipcMain, 'focus-changed');
    return focusedElementId;
  };

  describe('a TAB press', () => {
    const tabPressEvent: any = {
      type: 'keyDown',
      keyCode: 'Tab'
    };

    it('moves focus to the next focusable item', async () => {
      let focusChange = expectFocusChange();
      w.webContents.sendInputEvent(tabPressEvent);
      let focusedElementId = await focusChange;
      expect(focusedElementId, `should start focused in element-1, it's instead in ${focusedElementId}`).toBe(
        'BUTTON-element-1'
      );

      focusChange = expectFocusChange();
      w.webContents.sendInputEvent(tabPressEvent);
      focusedElementId = await focusChange;
      expect(focusedElementId, `focus should've moved to element-2, it's instead in ${focusedElementId}`).toBe(
        'BUTTON-element-2'
      );

      focusChange = expectFocusChange();
      w.webContents.sendInputEvent(tabPressEvent);
      focusedElementId = await focusChange;
      expect(
        focusedElementId,
        `focus should've moved to the webview's element-1, it's instead in ${focusedElementId}`
      ).toBe('BUTTON-wv-element-1');

      focusChange = expectFocusChange();
      webviewContents.sendInputEvent(tabPressEvent);
      focusedElementId = await focusChange;
      expect(
        focusedElementId,
        `focus should've moved to the webview's element-2, it's instead in ${focusedElementId}`
      ).toBe('BUTTON-wv-element-2');

      focusChange = expectFocusChange();
      webviewContents.sendInputEvent(tabPressEvent);
      focusedElementId = await focusChange;
      expect(focusedElementId, `focus should've moved to element-3, it's instead in ${focusedElementId}`).toBe(
        'BUTTON-element-3'
      );

      focusChange = expectFocusChange();
      w.webContents.sendInputEvent(tabPressEvent);
      focusedElementId = await focusChange;
      expect(focusedElementId, `focus should've looped back to element-1, it's instead in ${focusedElementId}`).toBe(
        'BUTTON-element-1'
      );
    });
  });

  describe('a SHIFT + TAB press', () => {
    const shiftTabPressEvent: any = {
      type: 'keyDown',
      modifiers: ['Shift'],
      keyCode: 'Tab'
    };

    it('moves focus to the previous focusable item', async () => {
      let focusChange = expectFocusChange();
      w.webContents.sendInputEvent(shiftTabPressEvent);
      let focusedElementId = await focusChange;
      expect(focusedElementId, `should start focused in element-3, it's instead in ${focusedElementId}`).toBe(
        'BUTTON-element-3'
      );

      focusChange = expectFocusChange();
      w.webContents.sendInputEvent(shiftTabPressEvent);
      focusedElementId = await focusChange;
      expect(
        focusedElementId,
        `focus should've moved to the webview's element-2, it's instead in ${focusedElementId}`
      ).toBe('BUTTON-wv-element-2');

      focusChange = expectFocusChange();
      webviewContents.sendInputEvent(shiftTabPressEvent);
      focusedElementId = await focusChange;
      expect(
        focusedElementId,
        `focus should've moved to the webview's element-1, it's instead in ${focusedElementId}`
      ).toBe('BUTTON-wv-element-1');

      focusChange = expectFocusChange();
      webviewContents.sendInputEvent(shiftTabPressEvent);
      focusedElementId = await focusChange;
      expect(focusedElementId, `focus should've moved to element-2, it's instead in ${focusedElementId}`).toBe(
        'BUTTON-element-2'
      );

      focusChange = expectFocusChange();
      w.webContents.sendInputEvent(shiftTabPressEvent);
      focusedElementId = await focusChange;
      expect(focusedElementId, `focus should've moved to element-1, it's instead in ${focusedElementId}`).toBe(
        'BUTTON-element-1'
      );

      focusChange = expectFocusChange();
      w.webContents.sendInputEvent(shiftTabPressEvent);
      focusedElementId = await focusChange;
      expect(focusedElementId, `focus should've looped back to element-3, it's instead in ${focusedElementId}`).toBe(
        'BUTTON-element-3'
      );
    });
  });
});

describe('cross origin isolation', () => {
  afterEach(closeAllWindows);

  let server: http.Server;
  let serverUrl: string;

  beforeAll(async () => {
    server = http.createServer((_req, res) => {
      res.setHeader('Content-Type', 'text/html');
      res.setHeader('Cross-Origin-Opener-Policy', 'same-origin');
      res.setHeader('Cross-Origin-Embedder-Policy', 'require-corp');
      res.end('<!doctype html>');
    });
    serverUrl = (await listen(server)).url;
  });

  afterAll(() => {
    server.close();
  });

  it('is enabled by COOP and COEP headers', async () => {
    const w = new BrowserWindow({ show: false });
    await w.loadURL(serverUrl);

    const crossOriginIsolated = await w.webContents.executeJavaScript('globalThis.crossOriginIsolated');
    expect(crossOriginIsolated).toBe(true);
  });
});

describe('web security', () => {
  afterEach(closeAllWindows);
  let server: http.Server;
  let serverUrl: string;
  beforeAll(async () => {
    server = http.createServer((req, res) => {
      res.setHeader('Content-Type', 'text/html');
      res.end('<body>');
    });
    serverUrl = (await listen(server)).url;
  });
  afterAll(() => {
    server.close();
  });

  it('engages CORB when web security is not disabled', async () => {
    const w = new BrowserWindow({
      show: false,
      webPreferences: { webSecurity: true, nodeIntegration: true, contextIsolation: false }
    });
    const p = once(ipcMain, 'success');
    await w.loadURL(`data:text/html,<script>
        const s = document.createElement('script')
        s.src = "${serverUrl}"
        // The script will not load under ORB, refs https://chromium-review.googlesource.com/c/chromium/src/+/3785025.
        // Before ORB an empty response is sent, which is now replaced by a network error.
        s.onerror = () => { require('electron').ipcRenderer.send('success') }
        document.documentElement.appendChild(s)
      </script>`);
    await p;
  });

  it('bypasses CORB when web security is disabled', async () => {
    const w = new BrowserWindow({
      show: false,
      webPreferences: { webSecurity: false, nodeIntegration: true, contextIsolation: false }
    });
    const p = once(ipcMain, 'success');
    await w.loadURL(`data:text/html,
      <script>
        window.onerror = (e) => { require('electron').ipcRenderer.send('success', e) }
      </script>
      <script src="${serverUrl}"></script>`);
    await p;
  });

  it('engages CORS when web security is not disabled', async () => {
    const w = new BrowserWindow({
      show: false,
      webPreferences: { webSecurity: true, nodeIntegration: true, contextIsolation: false }
    });
    const p = once(ipcMain, 'response');
    await w.loadURL(`data:text/html,<script>
        (async function() {
          try {
            await fetch('${serverUrl}');
            require('electron').ipcRenderer.send('response', 'passed');
          } catch {
            require('electron').ipcRenderer.send('response', 'failed');
          }
        })();
      </script>`);
    const [, response] = await p;
    expect(response).toBe('failed');
  });

  it('bypasses CORS when web security is disabled', async () => {
    const w = new BrowserWindow({
      show: false,
      webPreferences: { webSecurity: false, nodeIntegration: true, contextIsolation: false }
    });
    const p = once(ipcMain, 'response');
    await w.loadURL(`data:text/html,<script>
        (async function() {
          try {
            await fetch('${serverUrl}');
            require('electron').ipcRenderer.send('response', 'passed');
          } catch {
            require('electron').ipcRenderer.send('response', 'failed');
          }
        })();
      </script>`);
    const [, response] = await p;
    expect(response).toBe('passed');
  });

  describe('accessing file://', () => {
    async function loadFile(w: BrowserWindow) {
      const thisFile = url.format({
        pathname: import.meta.filename.replaceAll('\\', '/'),
        protocol: 'file',
        slashes: true
      });
      await w.loadURL(`data:text/html,<script>
          function loadFile() {
            return new Promise((resolve) => {
              fetch('${thisFile}').then(
                () => resolve('loaded'),
                () => resolve('failed')
              )
            });
          }
        </script>`);
      return await w.webContents.executeJavaScript('loadFile()');
    }

    it('is forbidden when web security is enabled', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { webSecurity: true } });
      const result = await loadFile(w);
      expect(result).toBe('failed');
    });

    it('is allowed when web security is disabled', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { webSecurity: false } });
      const result = await loadFile(w);
      expect(result).toBe('loaded');
    });
  });

  describe('wasm-eval csp', () => {
    async function loadWasm(csp: string) {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          sandbox: true,
          enableBlinkFeatures: 'WebAssemblyCSP'
        }
      });
      await w.loadURL(`data:text/html,<head>
          <meta http-equiv="Content-Security-Policy" content="default-src 'self'; script-src 'self' 'unsafe-inline' ${csp}">
        </head>
        <script>
          function loadWasm() {
            const wasmBin = new Uint8Array([0, 97, 115, 109, 1, 0, 0, 0])
            return new Promise((resolve) => {
              WebAssembly.instantiate(wasmBin).then(() => {
                resolve('loaded')
              }).catch((error) => {
                resolve(error.message)
              })
            });
          }
        </script>`);
      return (await w.webContents.executeJavaScript('loadWasm()')).trim();
    }

    it('wasm codegen is disallowed by default', async () => {
      const r = await loadWasm('');
      expect(r).toBe(
        "WebAssembly.instantiate(): Compiling or instantiating WebAssembly module violates the following Content Security policy directive because 'unsafe-eval' is not an allowed source of script in the following Content Security Policy directive: \"script-src 'self' 'unsafe-inline'\"."
      );
    });

    it('wasm codegen is allowed with "wasm-unsafe-eval" csp', async () => {
      const r = await loadWasm("'wasm-unsafe-eval'");
      expect(r).toBe('loaded');
    });
  });

  describe('WebAssembly streaming compilation', () => {
    it('works in a renderer with nodeIntegration', async () => {
      const server = http.createServer((req, res) => {
        res.setHeader('Content-Type', 'application/wasm');
        res.setHeader('Access-Control-Allow-Origin', '*');
        // Minimal valid WebAssembly module (magic number + version).
        res.end(Buffer.from([0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00]));
      });
      const { url: serverUrl } = await listen(server);
      defer(() => server.close());

      const w = new BrowserWindow({
        show: false,
        webPreferences: { nodeIntegration: true, contextIsolation: false }
      });
      await w.loadURL('about:blank');
      const result = await w.webContents.executeJavaScript(
        `WebAssembly.instantiateStreaming(fetch('${serverUrl}')).then(() => 'loaded')`
      );
      expect(result).toBe('loaded');
    });
  });

  describe('csp', () => {
    for (const sandbox of [true, false]) {
      describe(`when sandbox: ${sandbox}`, () => {
        for (const contextIsolation of [true, false]) {
          describe(`when contextIsolation: ${contextIsolation}`, () => {
            it('prevents eval from running in an inline script', async () => {
              const w = new BrowserWindow({
                show: false,
                webPreferences: { sandbox, contextIsolation }
              });
              w.loadURL(`data:text/html,<head>
              <meta http-equiv="Content-Security-Policy" content="default-src 'self'; script-src 'self' 'unsafe-inline'">
            </head>
            <script>
              try {
                // We use console.log here because it is easier than making a
                // preload script, and the behavior under test changes when
                // contextIsolation: false
                console.log(eval('true'))
              } catch (e) {
                console.log(e.message)
              }
            </script>`);
              const [{ message }] = await once(w.webContents, 'console-message');
              expect(message).toMatch(/Evaluating a string as JavaScript violates/);
            });

            it('does not prevent eval from running in an inline script when there is no csp', async () => {
              const w = new BrowserWindow({
                show: false,
                webPreferences: { sandbox, contextIsolation }
              });
              w.loadURL(`data:text/html,
            <script>
              try {
                // We use console.log here because it is easier than making a
                // preload script, and the behavior under test changes when
                // contextIsolation: false
                console.log(eval('true'))
              } catch (e) {
                console.log(e.message)
              }
            </script>`);
              const [{ message }] = await once(w.webContents, 'console-message');
              expect(message).toBe('true');
            });

            it('prevents eval from running in executeJavaScript', async () => {
              const w = new BrowserWindow({
                show: false,
                webPreferences: { sandbox, contextIsolation }
              });
              w.loadURL(
                "data:text/html,<head><meta http-equiv=\"Content-Security-Policy\" content=\"default-src 'self'; script-src 'self' 'unsafe-inline'\"></meta></head>"
              );
              await expect(w.webContents.executeJavaScript('eval("true")')).rejects.toThrow();
            });

            it('does not prevent eval from running in executeJavaScript when there is no csp', async () => {
              const w = new BrowserWindow({
                show: false,
                webPreferences: { sandbox, contextIsolation }
              });
              w.loadURL('data:text/html,');
              expect(await w.webContents.executeJavaScript('eval("true")')).toBe(true);
            });
          });
        }
      });
    }
  });

  it('does not crash when multiple WebContent are created with web security disabled', () => {
    const options = { show: false, webPreferences: { webSecurity: false } };
    const w1 = new BrowserWindow(options);
    w1.loadURL(serverUrl);
    const w2 = new BrowserWindow(options);
    w2.loadURL(serverUrl);
  });
});

describe('command line switches', () => {
  let appProcess: ChildProcess.ChildProcessWithoutNullStreams | undefined;
  afterEach(() => {
    if (appProcess && !appProcess.killed) {
      appProcess.kill();
      appProcess = undefined;
    }
  });
  describe('--lang switch', () => {
    const currentLocale = app.getLocale();
    const currentSystemLocale = app.getSystemLocale();
    const currentPreferredLanguages = JSON.stringify(app.getPreferredSystemLanguages());
    const testLocale = async (locale: string, result: string, printEnv: boolean = false) => {
      const appPath = path.join(fixturesPath, 'api', 'locale-check');
      const args = [appPath, `--set-lang=${locale}`];
      if (printEnv) {
        args.push('--print-env');
      }
      if (process.platform === 'darwin') args.push('--use-mock-keychain');
      appProcess = ChildProcess.spawn(process.execPath, args);

      let output = '';
      appProcess.stdout.on('data', (data) => {
        output += data;
      });
      let stderr = '';
      appProcess.stderr.on('data', (data) => {
        stderr += data;
      });

      const [code, signal] = await once(appProcess, 'close');
      if (code !== 0) {
        throw new Error(`Process exited with code "${code}" signal "${signal}" output "${output}" stderr "${stderr}"`);
      }

      output = output.replaceAll(/(\r\n|\n|\r)/gm, '');
      expect(output).toBe(result);
    };

    it('should set the locale', async () => testLocale('fr', `fr|${currentSystemLocale}|${currentPreferredLanguages}`));
    it('should set the locale with country code', async () =>
      testLocale('zh-CN', `zh-CN|${currentSystemLocale}|${currentPreferredLanguages}`));
    it('should resolve a locale whose country code is not shipped', async () =>
      testLocale('de-DE', `de|${currentSystemLocale}|${currentPreferredLanguages}`));
    it('should not set an invalid locale', async () =>
      testLocale('asdfkl', `${currentLocale}|${currentSystemLocale}|${currentPreferredLanguages}`));

    const lcAll = String(process.env.LC_ALL);
    ifit(process.platform === 'linux')('current process has a valid LC_ALL env', async () => {
      // The LC_ALL env should not be set to DOM locale string.
      expect(lcAll).not.toBe(app.getLocale());
    });
    ifit(process.platform === 'linux')('should not change LC_ALL', async () => testLocale('fr', lcAll, true));
    ifit(process.platform === 'linux')('should not change LC_ALL when setting invalid locale', async () =>
      testLocale('asdfkl', lcAll, true)
    );
    ifit(process.platform === 'linux')('should not change LC_ALL when --lang is not set', async () =>
      testLocale('', lcAll, true)
    );
  });

  describe('--remote-debugging-pipe switch', () => {
    it('should expose CDP via pipe', async () => {
      const electronPath = process.execPath;
      const args = ['--remote-debugging-pipe'];
      if (process.platform === 'darwin') args.push('--use-mock-keychain');
      appProcess = ChildProcess.spawn(electronPath, args, {
        stdio: ['inherit', 'inherit', 'inherit', 'pipe', 'pipe']
      }) as ChildProcess.ChildProcessWithoutNullStreams;
      const stdio = appProcess.stdio as unknown as [
        NodeJS.ReadableStream,
        NodeJS.WritableStream,
        NodeJS.WritableStream,
        NodeJS.WritableStream,
        NodeJS.ReadableStream
      ];
      const pipe = new PipeTransport(stdio[3], stdio[4]);
      const versionPromise = new Promise((resolve) => {
        pipe.onmessage = resolve;
      });
      pipe.send({ id: 1, method: 'Browser.getVersion', params: {} });
      const message = (await versionPromise) as any;
      expect(message.id).toBe(1);
      expect(message.result.product).toContain('Chrome');
      expect(message.result.userAgent).toContain('Electron');
    });
    it('should override --remote-debugging-port switch', async () => {
      const electronPath = process.execPath;
      const args = ['--remote-debugging-pipe', '--remote-debugging-port=0'];
      if (process.platform === 'darwin') args.push('--use-mock-keychain');
      appProcess = ChildProcess.spawn(electronPath, args, {
        stdio: ['inherit', 'inherit', 'pipe', 'pipe', 'pipe']
      }) as ChildProcess.ChildProcessWithoutNullStreams;
      let stderr = '';
      appProcess.stderr.on('data', (data: string) => {
        stderr += data;
      });
      const stdio = appProcess.stdio as unknown as [
        NodeJS.ReadableStream,
        NodeJS.WritableStream,
        NodeJS.WritableStream,
        NodeJS.WritableStream,
        NodeJS.ReadableStream
      ];
      const pipe = new PipeTransport(stdio[3], stdio[4]);
      const versionPromise = new Promise((resolve) => {
        pipe.onmessage = resolve;
      });
      pipe.send({ id: 1, method: 'Browser.getVersion', params: {} });
      const message = (await versionPromise) as any;
      expect(message.id).toBe(1);
      expect(stderr).not.toContain('DevTools listening on');
    });
    it('should shut down Electron upon Browser.close CDP command', async () => {
      const electronPath = process.execPath;
      const args = ['--remote-debugging-pipe'];
      if (process.platform === 'darwin') args.push('--use-mock-keychain');
      appProcess = ChildProcess.spawn(electronPath, args, {
        stdio: ['inherit', 'inherit', 'inherit', 'pipe', 'pipe']
      }) as ChildProcess.ChildProcessWithoutNullStreams;
      const stdio = appProcess.stdio as unknown as [
        NodeJS.ReadableStream,
        NodeJS.WritableStream,
        NodeJS.WritableStream,
        NodeJS.WritableStream,
        NodeJS.ReadableStream
      ];
      const pipe = new PipeTransport(stdio[3], stdio[4]);
      pipe.send({ id: 1, method: 'Browser.close', params: {} });
      await once(appProcess, 'exit');
    });
  });

  describe('--remote-debugging-port switch', () => {
    it('should display the discovery page', () =>
      new Promise<void>((resolve, reject) => {
        const done = (error?: unknown) => (error ? reject(error) : resolve());
        const electronPath = process.execPath;
        let output = '';
        const args = ['--remote-debugging-port='];
        if (process.platform === 'darwin') args.push('--use-mock-keychain');
        appProcess = ChildProcess.spawn(electronPath, args);
        appProcess.stdout.on('data', (data) => {
          console.log(data);
        });

        appProcess.stderr.on('data', (data) => {
          console.log(data);
          output += data;
          const m = /DevTools listening on ws:\/\/127.0.0.1:(\d+)\//.exec(output);
          if (m) {
            appProcess!.stderr.removeAllListeners('data');
            const port = m[1];
            http
              .get(`http://127.0.0.1:${port}`, (res) => {
                try {
                  expect(res.statusCode).toEqual(200);
                  expect(parseInt(res.headers['content-length']!)).toBeGreaterThan(0);
                  done();
                } catch (e) {
                  done(e);
                } finally {
                  res.destroy();
                }
              })
              .on('error', done);
          }
        });
      }));

    it('should use bundled devtools frontend URL in /json response', async () => {
      // Regression test for https://github.com/electron/electron/issues/51035
      // Verifies that devtoolsFrontendUrl points to the local bundled path
      // (/devtools/inspector.html) rather than the remote CDN URL
      // (https://chrome-devtools-frontend.appspot.com), which would 404 for
      // Electron's custom Chromium builds.
      const electronPath = process.execPath;
      let stderr = '';
      appProcess = ChildProcess.spawn(electronPath, ['--remote-debugging-port=0']);

      const port = await new Promise<string>((resolve, reject) => {
        appProcess!.stderr.on('data', (data: Buffer) => {
          stderr += data.toString();
          const m = /DevTools listening on ws:\/\/127\.0\.0\.1:(\d+)\//.exec(stderr);
          if (m) {
            appProcess!.stderr.removeAllListeners('data');
            resolve(m[1]);
          }
        });
        appProcess!.on('exit', () =>
          reject(new Error(`Process exited before DevTools port was found. stderr: ${stderr}`))
        );
      });

      const jsonTargets = await new Promise<any[]>((resolve, reject) => {
        http
          .get(`http://127.0.0.1:${port}/json`, (res) => {
            let body = '';
            res.on('data', (chunk: Buffer) => {
              body += chunk.toString();
            });
            res.on('end', () => {
              try {
                resolve(JSON.parse(body));
              } catch {
                reject(new Error(`Failed to parse /json response: ${body}`));
              }
            });
            res.on('error', reject);
          })
          .on('error', reject);
      });

      expect(jsonTargets).toBeInstanceOf(Array);
      expect(jsonTargets.length).toBeGreaterThan(0);

      // Every target's devtoolsFrontendUrl must use the bundled path,
      // not the remote CDN (chrome-devtools-frontend.appspot.com).
      for (const target of jsonTargets) {
        expect(target).toHaveProperty('devtoolsFrontendUrl');
        expect(
          target.devtoolsFrontendUrl,
          `devtoolsFrontendUrl should start with /devtools/ (bundled), got: ${target.devtoolsFrontendUrl}`
        ).toMatch(/^\/devtools\//);
        expect(target.devtoolsFrontendUrl, 'devtoolsFrontendUrl should not point to the remote CDN').not.toContain(
          'chrome-devtools-frontend.appspot.com'
        );
      }
    });

    it('clears device metrics overrides when a client disconnects without detaching', async () => {
      // A client dying without clearing its overrides used to leave the page
      // pinned at the emulated size forever.
      const appPath = path.join(fixturesPath, 'apps', 'remote-debugging-emulation');
      appProcess = ChildProcess.spawn(process.execPath, [appPath, '--remote-debugging-port=0']);

      let stderr = '';
      const browserWsUrl = await new Promise<string>((resolve, reject) => {
        appProcess!.stderr.on('data', (data: Buffer) => {
          stderr += data.toString();
          const m = /DevTools listening on (ws:\/\/\S+)/.exec(stderr);
          if (m) {
            appProcess!.stderr.removeAllListeners('data');
            resolve(m[1]);
          }
        });
        appProcess!.on('exit', () =>
          reject(new Error(`Process exited before DevTools URL was found. stderr: ${stderr}`))
        );
      });

      type Client = {
        socket: WebSocket;
        send(method: string, params?: unknown, sessionId?: string): Promise<any>;
        attachToPage(): Promise<string>;
      };
      const connectClient = async (): Promise<Client> => {
        const socket = new WebSocket(browserWsUrl);
        await once(socket, 'open');
        let nextId = 1;
        const pending = new Map<number, { resolve: (result: any) => void; reject: (error: Error) => void }>();
        socket.on('message', (data) => {
          const message = JSON.parse(data.toString());
          const handler = message.id && pending.get(message.id);
          if (handler) {
            pending.delete(message.id);
            if (message.error) handler.reject(new Error(message.error.message));
            else handler.resolve(message.result);
          }
        });
        const failPending = (why: string) => {
          for (const handler of pending.values()) handler.reject(new Error(why));
          pending.clear();
        };
        socket.on('error', (error) => failPending(`websocket error: ${error.message}`));
        socket.on('close', () => failPending('websocket closed'));
        const send = (method: string, params: unknown = {}, sessionId?: string) =>
          new Promise<any>((resolve, reject) => {
            const id = nextId++;
            pending.set(id, { resolve, reject });
            socket.send(JSON.stringify({ id, method, params, sessionId }));
          });
        const attachToPage = async () => {
          // The window may not exist yet when the DevTools server comes up.
          let page: any;
          await waitUntil(async () => {
            const { targetInfos } = await send('Target.getTargets');
            page = targetInfos.find((target: any) => target.type === 'page');
            return page !== undefined;
          });
          const { sessionId } = await send('Target.attachToTarget', { targetId: page.targetId, flatten: true });
          return sessionId;
        };
        return { socket, send, attachToPage };
      };
      const innerSize = async (client: Client, sessionId: string) => {
        const { result } = await client.send(
          'Runtime.evaluate',
          {
            expression: 'window.innerWidth + "x" + window.innerHeight',
            returnByValue: true
          },
          sessionId
        );
        return result.value;
      };

      const clientA = await connectClient();
      const sessionA = await clientA.attachToPage();
      const originalSize = await innerSize(clientA, sessionA);
      await clientA.send(
        'Emulation.setDeviceMetricsOverride',
        {
          width: 800,
          height: 450,
          deviceScaleFactor: 0,
          mobile: false
        },
        sessionA
      );
      await waitUntil(async () => (await innerSize(clientA, sessionA)) === '800x450');
      expect(await innerSize(clientA, sessionA)).toBe('800x450');

      // Drop the TCP connection like a killed client process would.
      (clientA.socket as any)._socket.destroy();

      const clientB = await connectClient();
      const sessionB = await clientB.attachToPage();
      await waitUntil(async () => (await innerSize(clientB, sessionB)) === originalSize);
      clientB.socket.close();
    });
  });

  describe('--trace-startup switch', () => {
    const outputFilePath = path.join(app.getPath('temp'), 'trace.json');
    afterEach(() => {
      if (fs.existsSync(outputFilePath)) {
        fs.unlinkSync(outputFilePath);
      }
    });

    it('creates startup trace', async () => {
      // node.async_hooks relies on %trace builtin to log trace points from JS
      // https://github.com/nodejs/node/blob/8b199eef3dd4de910a6521adc42ae611a62a19e1/lib/internal/trace_events_async_hooks.js#L48-L53
      // The phase event arg TRACE_EVENT_PHASE_NESTABLE_ASYNC_(BEGIN | END) is not supported in v8_use_perfetto mode
      // https://source.chromium.org/chromium/chromium/src/+/main:v8/src/builtins/builtins-trace.cc;l=201-216
      // and leads to the following error: TypeError: Trace event phase must be a number.
      // TODO: Identify why the error started appearing with roll https://github.com/electron/electron/pull/47561
      // given both v8_use_perfetto has been enabled before the roll and builtins-trace macro hasn't changed.
      const rc = await startRemoteControlApp([
        '--trace-startup="*,-node.async_hooks"',
        `--trace-startup-file=${outputFilePath}`,
        '--trace-startup-duration=1',
        '--enable-logging'
      ]);
      const stderrComplete = new Promise<string>((resolve) => {
        let stderr = '';
        rc.process.stderr!.on('data', (chunk) => {
          stderr += chunk.toString('utf8');
        });
        rc.process.on('close', () => {
          resolve(stderr);
        });
      });
      rc.remotely(() => {
        global.setTimeout(() => {
          require('electron').app.quit();
        }, 5000);
      });
      const stderr = await stderrComplete;
      expect(stderr).toMatch(/Completed startup tracing to/);
      expect(fs.existsSync(outputFilePath), 'output exists').toBe(true);
      expect(
        fs.statSync(outputFilePath).size,
        `the trace output file is empty, check "${outputFilePath}"`
      ).toBeGreaterThan(0);
    });
  });
});

describe('chromium features', () => {
  afterEach(closeAllWindows);

  describe('accessing key names also used as Node.js module names', () => {
    it('does not crash', () =>
      new Promise<void>((resolve, reject) => {
        const done = (error?: unknown) => (error ? reject(error) : resolve());
        const w = new BrowserWindow({ show: false });
        w.webContents.once('did-finish-load', () => {
          done();
        });
        w.webContents.once('render-process-gone', () => done(new Error('WebContents crashed.')));
        w.loadFile(path.join(fixturesPath, 'pages', 'external-string.html'));
      }));
  });

  describe('first party sets', () => {
    const fps = ['https://fps-member1.glitch.me', 'https://fps-member2.glitch.me', 'https://fps-member3.glitch.me'];

    it('loads first party sets', async () => {
      const appPath = path.join(fixturesPath, 'api', 'first-party-sets', 'base');
      const args = [appPath];
      if (process.platform === 'darwin') args.push('--use-mock-keychain');
      const fpsProcess = ChildProcess.spawn(process.execPath, args);

      let output = '';
      fpsProcess.stdout.on('data', (data) => {
        output += data;
      });
      await once(fpsProcess, 'close');

      expect(output).toContain(fps.join(','));
    });

    it('loads sets from the command line', async () => {
      const appPath = path.join(fixturesPath, 'api', 'first-party-sets', 'command-line');
      const args = [appPath, `--use-first-party-set=${fps}`];
      if (process.platform === 'darwin') args.push('--use-mock-keychain');
      const fpsProcess = ChildProcess.spawn(process.execPath, args);

      let output = '';
      fpsProcess.stdout.on('data', (data) => {
        output += data;
      });
      await once(fpsProcess, 'close');

      expect(output).toContain(fps.join(','));
    });
  });

  describe('loading jquery', () => {
    it('does not crash', () =>
      new Promise<void>((resolve, reject) => {
        const done = (error?: unknown) => (error ? reject(error) : resolve());
        const w = new BrowserWindow({ show: false });
        w.webContents.once('did-finish-load', () => {
          done();
        });
        w.webContents.once('render-process-gone', () => done(new Error('WebContents crashed.')));
        w.loadFile(path.join(import.meta.dirname, 'fixtures', 'pages', 'jquery.html'));
      }));
  });

  describe('navigator.keyboard', () => {
    afterEach(closeAllWindows);

    it('getLayoutMap() should return a KeyboardLayoutMap object', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
      const size = await w.webContents.executeJavaScript(`
        navigator.keyboard.getLayoutMap().then(map => map.size)
      `);

      expect(size).toBeTypeOf('number');
    });

    ifit(process.platform !== 'darwin')('should lock the keyboard', { tags: ['serial'] }, async () => {
      const w = new BrowserWindow({ show: true });
      await w.loadFile(path.join(fixturesPath, 'pages', 'modal.html'));

      // Test that without lock, with ESC:
      // - the window leaves fullscreen
      // - the dialog is not closed
      const enterFS1 = once(w, 'enter-full-screen');
      await w.webContents.executeJavaScript('document.body.requestFullscreen()', true);
      await enterFS1;

      await w.webContents.executeJavaScript("document.getElementById('favDialog').showModal()", true);
      const open1 = await w.webContents.executeJavaScript("document.getElementById('favDialog').open");
      expect(open1).toBe(true);

      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Escape' });
      await setTimeout(1000);
      await waitUntil(async () => {
        return await w.webContents.executeJavaScript("document.getElementById('favDialog').open");
      });
      expect(w.isFullScreen()).toBe(false);

      // Test that with lock, with ESC:
      // - the window does not leave fullscreen
      // - the dialog is closed
      const enterFS2 = once(w, 'enter-full-screen');
      await w.webContents.executeJavaScript(
        `
        document.body.requestFullscreen();
      `,
        true
      );
      await enterFS2;

      // Request keyboard lock after window has gone fullscreen
      // otherwise it will result in blink::kKeyboardLockRequestFailedErrorMsg.
      await w.webContents.executeJavaScript(
        `
        navigator.keyboard.lock(['Escape']);
      `,
        true
      );

      await w.webContents.executeJavaScript("document.getElementById('favDialog').showModal()", true);
      const open2 = await w.webContents.executeJavaScript("document.getElementById('favDialog').open");
      expect(open2).toBe(true);

      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Escape' });
      await setTimeout(1000);
      await waitUntil(async () => {
        const openAfter2 = await w.webContents.executeJavaScript("document.getElementById('favDialog').open");
        return openAfter2 === false;
      });
      expect(w.isFullScreen()).toBe(true);
    });
  });

  describe('navigator.languages', () => {
    it('should return the system locale only', async () => {
      const appLocale = app.getLocale();
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');
      const languages = await w.webContents.executeJavaScript('navigator.languages');
      expect(languages.length).toBeGreaterThan(0);
      expect(languages).toContain(appLocale);
    });
  });

  describe('navigator.serviceWorker', () => {
    it('should register for file scheme', () =>
      new Promise<void>((resolve, reject) => {
        const done = (error?: unknown) => (error ? reject(error) : resolve());
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            partition: 'sw-file-scheme-spec',
            contextIsolation: false
          }
        });
        w.webContents.on('ipc-message', (event, channel, message) => {
          if (channel === 'reload') {
            w.webContents.reload();
          } else if (channel === 'error') {
            done(message);
          } else if (channel === 'response') {
            expect(message).toBe('Hello from serviceWorker!');
            session
              .fromPartition('sw-file-scheme-spec')
              .clearStorageData({
                storages: ['serviceworkers']
              })
              .then(() => done());
          }
        });
        w.webContents.on('render-process-gone', () => done(new Error('WebContents crashed.')));
        w.loadFile(path.join(fixturesPath, 'pages', 'service-worker', 'index.html'));
      }));

    it('should register for intercepted file scheme', () =>
      new Promise<void>((resolve, reject) => {
        const done = (error?: unknown) => (error ? reject(error) : resolve());
        const customSession = session.fromPartition('intercept-file');
        customSession.protocol.interceptBufferProtocol('file', (request, callback) => {
          let file = new URL(request.url).pathname!;
          if (file[0] === '/' && process.platform === 'win32') file = file.slice(1);

          file = file.replace('service-worker.js', 'service-worker-intercepted.js');

          const content = fs.readFileSync(path.normalize(file));
          const ext = path.extname(file);
          let type = 'text/html';

          if (ext === '.js') type = 'application/javascript';
          callback({ data: content, mimeType: type } as any);
        });

        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            session: customSession,
            contextIsolation: false
          }
        });
        w.webContents.on('ipc-message', (event, channel, message) => {
          if (channel === 'reload') {
            w.webContents.reload();
          } else if (channel === 'error') {
            done(`unexpected error : ${message}`);
          } else if (channel === 'response') {
            expect(message).toBe('Hello from serviceWorker intercepted!');
            customSession
              .clearStorageData({
                storages: ['serviceworkers']
              })
              .then(() => {
                customSession.protocol.uninterceptProtocol('file');
                done();
              });
          }
        });
        w.webContents.on('render-process-gone', () => done(new Error('WebContents crashed.')));
        w.loadFile(path.join(fixturesPath, 'pages', 'service-worker', 'index.html'));
      }));

    it('should trigger webRequest handlers when loaded as a file', () =>
      new Promise<void>((resolve, reject) => {
        const done = (error?: unknown) => (error ? reject(error) : resolve());
        const customSession = session.fromPartition('sw-file-scheme-webRequest');
        customSession.webRequest.onBeforeRequest((details, cb) => {
          if (details.url.endsWith('service-worker.js')) {
            done(); // Service worker triggered webRequest handler.
          }
          cb({});
        });

        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            session: customSession,
            contextIsolation: false
          }
        });
        w.webContents.on('render-process-gone', () => done(new Error('WebContents crashed.')));
        w.loadFile(path.join(fixturesPath, 'pages', 'service-worker', 'index.html'));
      }));

    it('should register for custom scheme', () =>
      new Promise<void>((resolve, reject) => {
        const done = (error?: unknown) => (error ? reject(error) : resolve());
        const customSession = session.fromPartition('custom-scheme');
        customSession.protocol.registerFileProtocol(serviceWorkerScheme, (request, callback) => {
          let file = new URL(request.url).pathname!;
          if (file[0] === '/' && process.platform === 'win32') file = file.slice(1);

          callback({ path: path.normalize(file) } as any);
        });

        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            session: customSession,
            contextIsolation: false
          }
        });
        w.webContents.on('ipc-message', (event, channel, message) => {
          if (channel === 'reload') {
            w.webContents.reload();
          } else if (channel === 'error') {
            done(`unexpected error : ${message}`);
          } else if (channel === 'response') {
            expect(message).toBe('Hello from serviceWorker!');
            customSession
              .clearStorageData({
                storages: ['serviceworkers']
              })
              .then(() => {
                customSession.protocol.uninterceptProtocol(serviceWorkerScheme);
                done();
              });
          }
        });
        w.webContents.on('render-process-gone', () => done(new Error('WebContents crashed.')));
        w.loadFile(path.join(fixturesPath, 'pages', 'service-worker', 'custom-scheme-index.html'));
      }));

    it('should not allow nodeIntegrationInWorker', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          nodeIntegrationInWorker: true,
          partition: 'sw-file-scheme-worker-spec',
          contextIsolation: false
        }
      });

      await w.loadURL(`file://${fixturesPath}/pages/service-worker/empty.html`);

      const data = await w.webContents.executeJavaScript(`
        navigator.serviceWorker.register('worker-no-node.js', {
          scope: './'
        }).then(() => navigator.serviceWorker.ready)

        new Promise((resolve) => {
          navigator.serviceWorker.onmessage = event => resolve(event.data);
        });
      `);

      expect(data).toBe('undefined undefined undefined undefined');
    });
  });

  describe('navigator.geolocation', () => {
    it('returns error when permission is denied', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          partition: 'geolocation-spec',
          contextIsolation: false
        }
      });
      const message = once(w.webContents, 'ipc-message');
      w.webContents.session.setPermissionRequestHandler((wc, permission, callback) => {
        if (permission === 'geolocation') {
          callback(false);
        } else {
          callback(true);
        }
      });
      w.loadFile(path.join(fixturesPath, 'pages', 'geolocation', 'index.html'));
      const [, channel] = await message;
      expect(channel, 'unexpected response from geolocation api').toBe('success');
    });

    it('returns position when permission is granted', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          partition: 'geolocation-spec'
        }
      });
      const requested: string[] = [];
      w.webContents.session.setPermissionRequestHandler((_wc, permission, callback) => {
        requested.push(permission);
        callback(true);
      });
      // Supply the position over CDP so the test doesn't depend on a real provider:
      // the network location service fails on CI.
      // The permission request still goes through the session's handler.
      const override = { latitude: 37.7749, longitude: -122.4194, accuracy: 10 };
      w.webContents.debugger.attach();
      await w.webContents.debugger.sendCommand('Emulation.setGeolocationOverride', override);
      await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      const position = await w.webContents.executeJavaScript(`new Promise((resolve, reject) =>
        navigator.geolocation.getCurrentPosition(
          ({ coords: { latitude, longitude, accuracy }, timestamp }) =>
            resolve({ coords: { latitude, longitude, accuracy }, timestamp }),
          err => reject(new Error(err.message))))`);
      expect(requested).toContain('geolocation');
      expect(position.coords).toEqual(override);
      expect(position.timestamp).toBeTypeOf('number');
    });

    ifdescribe(process.platform === 'darwin')('with --disable-geolocation', () => {
      const testSwitchBehavior = (handlerAction: 'allow' | 'deny' | 'none') => async () => {
        const rc = await startRemoteControlApp([
          '--disable-geolocation',
          `--boot-eval=fixturesPath=${JSON.stringify(fixturesPath)}`
        ]);

        const result = await rc.remotely(async (action: typeof handlerAction) => {
          const { session, BrowserWindow } = require('electron');
          const path = require('node:path');

          // Isolate each test's permissions to prevent permission state leaks between the test variations
          const testSession = session.fromPartition(`geolocation-disable-${action}`);

          if (action !== 'none') {
            // Make the PermissionRequestHandler behave according to action variable passed for this test
            testSession.setPermissionRequestHandler(
              (_wc: Electron.WebContents, permission: string, callback: (allow: boolean) => void) => {
                if (permission === 'geolocation') {
                  if (action === 'allow') callback(true);
                  else if (action === 'deny') callback(false);
                  else callback(false);
                }
              }
            );
          }

          const w = new BrowserWindow({
            show: false,
            webPreferences: {
              session: testSession,
              nodeIntegration: true,
              contextIsolation: false
            }
          });

          await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));

          const permissionState = await w.webContents.executeJavaScript(`
            navigator.permissions.query({ name: 'geolocation' })
              .then(status => status.state)
              .catch(() => 'error')
          `);

          const geoResult = await w.webContents.executeJavaScript(`
            new Promise(resolve => {
              navigator.geolocation.getCurrentPosition(
                () => resolve('allowed'),
                err => resolve(err.code)
              );
            })
          `);

          return { permissionState, geoResult };
        }, handlerAction);

        // Always expect status to be denied regardless of the decision made by a handler set via `session.setPermissionRequestHandler`
        expect(result.permissionState, `Unexpected permission state for ${handlerAction} handler`).toBe('denied');

        // 1 = PERMISSION_DENIED
        expect(result.geoResult, `Unexpected API result for ${handlerAction} handler`).toBe(1);
      };

      it('denies geolocation when permission request handler would allow', testSwitchBehavior('allow'));
      it('denies geolocation when permission request handler would deny', testSwitchBehavior('deny'));
      it('denies geolocation with no permission request handler', testSwitchBehavior('none'));
    });
  });

  describe('<geolocation> element', () => {
    afterEach(closeAllWindows);

    it('does not crash the renderer', () =>
      new Promise<void>((resolve, reject) => {
        const done = (error?: unknown) => (error ? reject(error) : resolve());
        const w = new BrowserWindow({ show: false });
        w.webContents.once('did-finish-load', () => done());
        w.webContents.once('render-process-gone', () => done(new Error('renderer crashed / was killed')));
        w.loadURL('data:text/html,<geolocation></geolocation>');
      }));
  });

  describe('File System API,', { tags: ['serial'] }, () => {
    let w: BrowserWindow | null = null;

    afterEach(async () => {
      session.defaultSession.setPermissionCheckHandler(null);
      session.defaultSession.setPermissionRequestHandler(null);
      await closeAllWindows();
    });

    // Pastes a file:// URI into the test-perms.html fixture
    // and resolves with the channel the fixture reported its new handle on.
    // Rejects if the fixture reports that getting the handle failed.
    const pasteFileUri = async (w: BrowserWindow, filePath: string) => {
      const handleCreated = emittedUntil(w.webContents, 'ipc-message', (_event: unknown, channel: string) => {
        return (
          channel === 'did-create-file-handle' ||
          channel === 'did-create-directory-handle' ||
          channel === 'file-system-error'
        );
      });
      await clipboard.write([new ClipboardItem({ 'text/uri-list': url.pathToFileURL(filePath).href })]);
      await focusWebContents(w.webContents);
      w.webContents.paste();
      const [, channel, message] = await handleCreated;
      if (channel === 'file-system-error') {
        throw new Error(`File handle acquisition failed: ${message}`);
      }
      return channel;
    };

    it('allows access by default to reading an OPFS file', async () => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          partition: 'file-system-spec',
          contextIsolation: false
        }
      });

      await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      const result = await w.webContents.executeJavaScript(
        `
        new Promise(async (resolve, reject) => {
          const root = await navigator.storage.getDirectory();
          const fileHandle = await root.getFileHandle('test', { create: true });
          const { name, size } = await fileHandle.getFile();
          resolve({ name, size });
        }
      )`,
        true
      );
      expect(result).toEqual({ name: 'test', size: 0 });
    });

    it('fileHandle.queryPermission by default has permission to read and write to OPFS files', async () => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          partition: 'file-system-spec',
          contextIsolation: false
        }
      });

      await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      const status = await w.webContents.executeJavaScript(
        `
        new Promise(async (resolve, reject) => {
          const root = await navigator.storage.getDirectory();
          const fileHandle = await root.getFileHandle('test', { create: true });
          const status = await fileHandle.queryPermission({ mode: 'readwrite' });
          resolve(status);
        }
      )`,
        true
      );
      expect(status).toBe('granted');
    });

    it('fileHandle.requestPermission automatically grants permission to read and write to OPFS files', async () => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          partition: 'file-system-spec',
          contextIsolation: false
        }
      });

      await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      const status = await w.webContents.executeJavaScript(
        `
        new Promise(async (resolve, reject) => {
          const root = await navigator.storage.getDirectory();
          const fileHandle = await root.getFileHandle('test', { create: true });
          const status = await fileHandle.requestPermission({ mode: 'readwrite' });
          resolve(status);
        }
      )`,
        true
      );
      expect(status).toBe('granted');
    });

    it('concurrent getFileHandle calls on the same file do not stall', async () => {
      const writablePath = path.join(fixturesPath, 'file-system', 'test-perms.html');
      const testDir = path.join(fixturesPath, 'file-system');
      const testFile = path.join(testDir, 'test.txt');
      const permissionRequests: unknown[] = [];

      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false,
          sandbox: false
        }
      });

      w.webContents.session.setPermissionRequestHandler((wc, permission, callback, details) => {
        if (permission === 'fileSystem') {
          permissionRequests.push(details);
          callback(true);
        } else {
          callback(false);
        }
      });

      await w.loadFile(writablePath);
      expect(await pasteFileUri(w, testDir)).toBe('did-create-directory-handle');
      const result = await w.webContents.executeJavaScript(
        `
        new Promise(async (resolve, reject) => {
          try {
            const handles = await Promise.all([
              handle.getFileHandle('test.txt'),
              handle.getFileHandle('test.txt')
            ]);
            resolve(handles.length === 2);
          } catch (err) {
            reject(err.message);
          }
        })
      `,
        true
      );
      expect(result).toBe(true);
      for (const details of permissionRequests) {
        expect(details).toEqual({
          fileAccessType: 'readable',
          isDirectory: false,
          isMainFrame: true,
          filePath: testFile,
          requestingUrl: url.pathToFileURL(writablePath).href
        });
      }
    });

    it('allows permission when trying to create a writable file handle', async () => {
      const writablePath = path.join(fixturesPath, 'file-system', 'test-perms.html');
      const testFile = path.join(fixturesPath, 'file-system', 'test.txt');
      const permissionRequests: unknown[] = [];

      const w = new BrowserWindow({
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false,
          sandbox: false
        }
      });

      w.webContents.session.setPermissionRequestHandler((wc, permission, callback, details) => {
        if (permission === 'fileSystem') {
          permissionRequests.push(details);
          callback(true);
          return;
        }
        callback(false);
      });

      await w.loadFile(writablePath);
      expect(await pasteFileUri(w, testFile)).toBe('did-create-file-handle');
      // A permission request such as createWritable() needs the page's user
      // activation in the browser process.
      // webFrameMain.executeJavaScript() updates that before it runs the script.
      // webContents.executeJavaScript() only tells the renderer,
      // whose activation update can lose the race with the request,
      // which is then rejected with "User activation is required to request permissions".
      const result = await w.webContents.mainFrame.executeJavaScript(
        `
        new Promise(async (resolve, reject) => {
          try {
            const writable = await handle.createWritable();
            resolve(true);
          } catch {
            resolve(false);
          }
        })
      `,
        true
      );
      expect(result).toBe(true);
      for (const details of permissionRequests) {
        expect(details).toEqual({
          fileAccessType: 'writable',
          isDirectory: false,
          isMainFrame: true,
          filePath: testFile,
          requestingUrl: url.pathToFileURL(writablePath).href
        });
      }
    });

    it('denies permission when trying to create a writable file handle', async () => {
      const writablePath = path.join(fixturesPath, 'file-system', 'test-perms.html');
      const testFile = path.join(fixturesPath, 'file-system', 'test.txt');
      const trace = (phase: string) => console.log(`File System denial: ${phase}`);
      const permissionRequests: unknown[] = [];

      const w = new BrowserWindow({
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false,
          sandbox: false
        }
      });

      w.webContents.on('ipc-message', (_event, channel, message) => {
        if (channel === 'file-system-progress' || channel === 'file-system-error') {
          trace(`${channel}: ${message}`);
        }
      });

      w.webContents.session.setPermissionRequestHandler((wc, permission, callback, details) => {
        if (permission === 'fileSystem') {
          trace(`permission requested: ${JSON.stringify(details)}`);
          permissionRequests.push(details);
        }
        callback(false);
      });

      trace('loading fixture');
      await w.loadFile(writablePath);
      trace('fixture loaded');
      await clipboard.write([new ClipboardItem({ 'text/uri-list': url.pathToFileURL(testFile).href })]);
      trace('clipboard written');

      const handleCreated = emittedUntil(w.webContents, 'ipc-message', (_event: unknown, channel: string) => {
        return (
          channel === 'did-create-file-handle' ||
          channel === 'did-create-directory-handle' ||
          channel === 'file-system-error'
        );
      });
      await focusWebContents(w.webContents);
      trace('paste requested');
      w.webContents.paste();
      const [, channel, message] = await handleCreated;
      if (channel === 'file-system-error') {
        throw new Error(`File handle acquisition failed: ${message}`);
      }
      expect(channel).toBe('did-create-file-handle');
      trace('file handle received');

      const permission = await w.webContents.executeJavaScript('handle.queryPermission({ mode: "readwrite" })');
      trace(`initial permission: ${permission}`);
      trace('createWritable requested');
      // webFrameMain.executeJavaScript(),
      // so that the browser process knows of the user activation before the request (see above).
      const writeError = (await w.webContents.mainFrame.executeJavaScript(
        `
        (async () => {
          try {
            await handle.createWritable();
            return null;
          } catch (error) {
            return { name: error.name, message: error.message };
          }
        })()
      `,
        true
      )) as { name: string; message: string } | null;
      trace(`createWritable ${writeError ? `rejected: ${writeError.name}: ${writeError.message}` : 'succeeded'}`);
      expect(writeError?.name).toBe('NotAllowedError');
      expect(permissionRequests).toEqual([
        {
          fileAccessType: 'writable',
          isDirectory: false,
          isMainFrame: true,
          filePath: testFile,
          requestingUrl: url.pathToFileURL(writablePath).href
        }
      ]);
    });

    it('calls twice when trying to query a read/write file handle permissions', async () => {
      const writablePath = path.join(fixturesPath, 'file-system', 'test-perms.html');
      const testFile = path.join(fixturesPath, 'file-system', 'test.txt');
      const permissionChecks: Electron.PermissionCheckHandlerHandlerDetails[] = [];

      const w = new BrowserWindow({
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false,
          sandbox: false
        }
      });

      w.webContents.session.setPermissionCheckHandler((wc, permission, origin, details) => {
        if (permission === 'fileSystem') {
          permissionChecks.push(details);
          return true;
        }

        return false;
      });

      await w.loadFile(writablePath);
      expect(await pasteFileUri(w, testFile)).toBe('did-create-file-handle');
      const permission = await w.webContents.executeJavaScript(
        `
        new Promise(async (resolve, reject) => {
          try {
            const permission = await handle.queryPermission({ mode: 'readwrite' });
            resolve(permission);
          } catch {
            resolve('denied');
          }
        })
      `,
        true
      );
      expect(permission).toBe('granted');
      expect(permissionChecks).toHaveLength(2);
      for (const { fileAccessType, isDirectory, filePath } of permissionChecks) {
        expect(['writable', 'readable']).toContain(fileAccessType);
        expect(isDirectory).toBe(false);
        expect(filePath).toBe(testFile);
      }
    });

    it('correctly denies permissions after creating a readable directory handle', async () => {
      const permPath = path.join(fixturesPath, 'file-system', 'test-perms.html');
      const testDir = path.join(fixturesPath, 'file-system');
      const permissionChecks: Electron.PermissionCheckHandlerHandlerDetails[] = [];

      const w = new BrowserWindow({
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false,
          sandbox: false
        }
      });

      w.webContents.session.setPermissionCheckHandler((wc, permission, origin, details) => {
        if (permission === 'fileSystem') {
          permissionChecks.push(details);
          return false;
        }
        return false;
      });

      await w.loadFile(permPath);
      expect(await pasteFileUri(w, testDir)).toBe('did-create-directory-handle');
      const permission = await w.webContents.executeJavaScript(
        `
        new Promise(async (resolve, reject) => {
          try {
            const permission = await handle.queryPermission({ mode: 'read' });
            resolve(permission);
          } catch {
            resolve('denied');
          }
        })
      `,
        true
      );
      expect(permission).toBe('denied');
      for (const details of permissionChecks) {
        expect(details).toMatchObject({ fileAccessType: 'readable', isDirectory: true, filePath: testDir });
      }
    });

    it('correctly allows permissions after creating a readable directory handle', async () => {
      const permPath = path.join(fixturesPath, 'file-system', 'test-perms.html');
      const testDir = path.join(fixturesPath, 'file-system');
      const permissionChecks: Electron.PermissionCheckHandlerHandlerDetails[] = [];

      const w = new BrowserWindow({
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false,
          sandbox: false
        }
      });

      w.webContents.session.setPermissionCheckHandler((wc, permission, origin, details) => {
        if (permission === 'fileSystem') {
          permissionChecks.push(details);
          return true;
        }
        return false;
      });

      await w.loadFile(permPath);
      expect(await pasteFileUri(w, testDir)).toBe('did-create-directory-handle');
      const permission = await w.webContents.executeJavaScript(
        `
        new Promise(async (resolve, reject) => {
          try {
            const permission = await handle.queryPermission({ mode: 'read' });
            resolve(permission);
          } catch {
            resolve('denied');
          }
        })
      `,
        true
      );
      expect(permission).toBe('granted');
      for (const details of permissionChecks) {
        expect(details).toMatchObject({ fileAccessType: 'readable', isDirectory: true, filePath: testDir });
      }
    });

    it('allows in-session persistence of granted file permissions', async () => {
      const writablePath = path.join(fixturesPath, 'file-system', 'test-perms.html');
      const testFile = path.join(fixturesPath, 'file-system', 'persist.txt');
      const permissionChecks: Electron.PermissionCheckHandlerHandlerDetails[] = [];

      const w = new BrowserWindow({
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false,
          sandbox: false
        }
      });

      w.webContents.session.setPermissionRequestHandler((_wc, _permission, callback) => {
        callback(true);
      });

      w.webContents.session.setPermissionCheckHandler((_wc, permission, _origin, details) => {
        if (permission === 'fileSystem') {
          permissionChecks.push(details);
          return true;
        }
        return false;
      });

      await w.loadFile(writablePath);
      expect(await pasteFileUri(w, testFile)).toBe('did-create-file-handle');

      const reloaded = once(w.webContents, 'did-finish-load');
      w.webContents.reload();
      await reloaded;
      expect(await pasteFileUri(w, testFile)).toBe('did-create-file-handle');
      const permission = await w.webContents.executeJavaScript(
        `
        new Promise(async (resolve, reject) => {
          try {
            const permission = await handle.queryPermission({ mode: 'read' });
            resolve(permission);
          } catch {
            resolve('denied');
          }
        })
      `,
        true
      );
      expect(permission).toBe('granted');
      for (const details of permissionChecks) {
        expect(details).toMatchObject({ fileAccessType: 'readable', isDirectory: false, filePath: testFile });
      }
    });
  });

  describe('File System Access permission scope', { tags: ['serial'] }, () => {
    // Pages obtain FileSystemHandles by pasting a file:// URI, the same way the
    // tests above do, so no picker is needed.
    const handlePage = `<!doctype html><body contenteditable tabindex="0">fsa<script>
      window.gotHandle = false; window.handle = null; window.handleError = null;
      document.onpaste = (event) => {
        event.preventDefault();
        event.clipboardData.items[0].getAsFileSystemHandle().then(
          (h) => { window.handle = h; window.gotHandle = true; },
          (e) => { window.handleError = String(e); window.gotHandle = true; });
      };
    </script></body>`;
    let serverA: http.Server;
    let serverB: http.Server;
    let urlA: string;
    let urlB: string;
    beforeAll(async () => {
      const handler = (_req: http.IncomingMessage, res: http.ServerResponse) => {
        res.setHeader('content-type', 'text/html');
        res.end(handlePage);
      };
      serverA = http.createServer(handler);
      serverB = http.createServer(handler);
      urlA = (await listen(serverA)).url;
      urlB = (await listen(serverB)).url;
    });
    afterAll(() => {
      serverA.close();
      serverB.close();
    });
    afterEach(closeAllWindows);

    const pasteHandle = async (w: BrowserWindow, frame: Electron.WebFrameMain, dirOrFile: string) => {
      await clipboard.write([new ClipboardItem({ 'text/uri-list': url.pathToFileURL(dirOrFile).href })]);
      await focusWebContents(w.webContents);
      await frame.executeJavaScript('window.focus(); document.body.focus(); window.gotHandle = false; true');
      w.webContents.paste();
    };
    const waitForHandle = (frame: Electron.WebFrameMain) =>
      waitUntil(async () => (await frame.executeJavaScript('window.gotHandle')) === true);

    it('does not let a cross-origin iframe request more access than it was granted', async () => {
      const ses = session.fromPartition(`fsa-scope-${Math.random()}`);
      const w = new BrowserWindow({ show: true, webPreferences: { session: ses } });
      await w.loadURL(urlA);
      await w.webContents.executeJavaScript(`new Promise((resolve) => {
        const f = document.createElement('iframe');
        f.src = ${JSON.stringify(urlB)};
        f.onload = resolve;
        document.body.appendChild(f);
      })`);
      const iframe = w.webContents.mainFrame.frames[0];
      const requests: any[] = [];
      ses.setPermissionRequestHandler((_wc, permission, callback, details) => {
        if (permission === 'fileSystem') requests.push(details);
        callback(true);
      });
      const testFile = path.join(fixturesPath, 'file-system', 'test.txt');
      await pasteHandle(w, iframe, testFile);
      await waitForHandle(iframe);
      expect(await iframe.executeJavaScript('window.handle && window.handle.kind')).toBe('file');
      // Matches Chrome: a third-party iframe cannot ask for more than it has.
      const status = await iframe.executeJavaScript(
        "window.handle.requestPermission({ mode: 'readwrite' }).catch((e) => e.name)",
        true
      );
      expect(status).toBe('SecurityError');
      expect(requests).toHaveLength(0);

      // The same request from the top-level document reaches the handler.
      await pasteHandle(w, w.webContents.mainFrame, testFile);
      await waitForHandle(w.webContents.mainFrame);
      const topStatus = await w.webContents.mainFrame.executeJavaScript(
        "window.handle.requestPermission({ mode: 'readwrite' })",
        true
      );
      expect(topStatus).toBe('granted');
      expect(requests).toHaveLength(1);
      expect(requests[0].isMainFrame).toBe(true);
    });

    it("asks about a restricted path once per requester, with that requester's identity", async () => {
      const ses = session.fromPartition(`fsa-scope-${Math.random()}`);
      const events: { details: any; callback: (action: 'allow' | 'deny' | 'tryAgain') => void }[] = [];
      ses.on('file-system-access-restricted', (_e, details, callback) => {
        events.push({ details, callback });
      });
      const wa = new BrowserWindow({ show: true, webPreferences: { session: ses } });
      await wa.loadURL(urlA);
      const wb = new BrowserWindow({ show: true, webPreferences: { session: ses } });
      await wb.loadURL(urlB);
      const restricted = os.homedir();

      await pasteHandle(wa, wa.webContents.mainFrame, restricted);
      await waitUntil(() => events.length === 1);
      await pasteHandle(wb, wb.webContents.mainFrame, restricted);
      await waitUntil(() => events.length === 2);

      expect(events[0].details.origin).toBe(`${urlA}/`);
      expect(events[0].details.webContents).toBe(wa.webContents);
      expect(events[0].details.frame).toBe(wa.webContents.mainFrame);
      expect(events[1].details.origin).toBe(`${urlB}/`);
      expect(events[1].details.webContents).toBe(wb.webContents);
      expect(events[1].details.frame).toBe(wb.webContents.mainFrame);

      // Answers are independent.
      events[1].callback('allow');
      events[0].callback('deny');
      await waitForHandle(wb.webContents.mainFrame);
      await waitForHandle(wa.webContents.mainFrame);
      expect(await wb.webContents.executeJavaScript('window.handle && window.handle.kind')).toBe('directory');
      expect(await wa.webContents.executeJavaScript('window.handle')).toBe(null);
    });

    it('revokes active grants once no top-level document of the origin remains', { timeout: 60000 }, async () => {
      const ses = session.fromPartition(`fsa-scope-${Math.random()}`);
      ses.setPermissionRequestHandler((_wc, _permission, callback) => callback(true));
      const testFile = path.join(fixturesPath, 'file-system', 'test.txt');

      // A top-level document of origin A obtains read/write access to the file.
      const top = new BrowserWindow({ show: true, webPreferences: { session: ses } });
      await top.loadURL(urlA);
      await pasteHandle(top, top.webContents.mainFrame, testFile);
      await waitForHandle(top.webContents.mainFrame);
      expect(
        await top.webContents.mainFrame.executeJavaScript(
          "window.handle.requestPermission({ mode: 'readwrite' })",
          true
        )
      ).toBe('granted');

      // An origin-A iframe inside an origin-B page holds a handle to the same
      // file and shares that grant.
      const host = new BrowserWindow({ show: true, webPreferences: { session: ses } });
      await host.loadURL(urlB);
      await host.webContents.executeJavaScript(`new Promise((resolve) => {
        const f = document.createElement('iframe');
        f.src = ${JSON.stringify(urlA)};
        f.onload = resolve;
        document.body.appendChild(f);
      })`);
      const iframe = host.webContents.mainFrame.frames[0];
      await pasteHandle(host, iframe, testFile);
      await waitForHandle(iframe);
      const query = () => iframe.executeJavaScript("window.handle.queryPermission({ mode: 'readwrite' })");
      expect(await query()).toBe('granted');

      // While another top-level origin-A document exists the grant survives the
      // first one closing...
      const otherTop = new BrowserWindow({ show: true, webPreferences: { session: ses } });
      await otherTop.loadURL(urlA);
      top.close();
      await setTimeout(6000);
      expect(await query()).toBe('granted');

      // ...and once the last one is gone it is revoked, so the embedded frame
      // can no longer write without the app being asked again.
      otherTop.close();
      await setTimeout(6000);
      expect(await query()).toBe('prompt');
    });
  });

  describe('web workers', () => {
    let appProcess: ChildProcess.ChildProcessWithoutNullStreams | undefined;

    afterEach(() => {
      if (appProcess && !appProcess.killed) {
        appProcess.kill();
        appProcess = undefined;
      }
    });

    it('Worker with nodeIntegrationInWorker has access to self.module.paths', async () => {
      const appPath = path.join(import.meta.dirname, 'fixtures', 'apps', 'self-module-paths');

      const args = [appPath];
      if (process.platform === 'darwin') args.push('--use-mock-keychain');
      appProcess = ChildProcess.spawn(process.execPath, args);

      const [code] = await once(appProcess, 'exit');
      expect(code).toBe(0);
    });

    itremote('Worker with nodeIntegrationInWorker has access to EventSource', () => {
      const es = new EventSource('https://example.com');
      expect(es).toHaveProperty('url', expect.any(String));
      expect(es).toHaveProperty('readyState', expect.any(Number));
      expect(es).toHaveProperty('withCredentials', expect.any(Boolean));
    });

    itremote(
      'Worker with nodeIntegrationInWorker has access to fetch-dependent interfaces',
      async (fixtures: string) => {
        const file = require('node:path').join(fixtures, 'hello.txt');
        expect(() => {
          fetch('file://' + file);
        }).not.toThrow();

        expect(() => {
          const formData = new FormData();
          formData.append('username', 'Groucho');
        }).not.toThrow();

        expect(() => {
          const request = new Request('https://example.com', {
            method: 'POST',
            body: JSON.stringify({ foo: 'bar' })
          });
          expect(request.method).toBe('POST');
        }).not.toThrow();

        expect(() => {
          const response = new Response('Hello, world!');
          expect(response.status).toBe(200);
        }).not.toThrow();

        expect(() => {
          const headers = new Headers();
          headers.append('Content-Type', 'text/xml');
        }).not.toThrow();
      },
      [path.join(import.meta.dirname, 'fixtures')]
    );

    it('Worker can work', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      const data = await w.webContents.executeJavaScript(`
        const worker = new Worker('../workers/worker.js');
        const message = 'ping';
        const eventPromise = new Promise((resolve) => { worker.onmessage = resolve; });
        worker.postMessage(message);
        eventPromise.then(t => t.data)
      `);
      expect(data).toBe('ping');
    });

    it('Worker has no node integration by default', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      const data = await w.webContents.executeJavaScript(`
        const worker = new Worker('../workers/worker_node.js');
        new Promise((resolve) => { worker.onmessage = e => resolve(e.data); })
      `);
      expect(data).toBe('undefined undefined undefined undefined');
    });

    it('Worker has node integration with nodeIntegrationInWorker', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          nodeIntegrationInWorker: true,
          contextIsolation: false
        }
      });

      w.loadURL(`file://${fixturesPath}/pages/worker.html`);
      const [, data] = await once(ipcMain, 'worker-result');
      expect(data).toBe('object function object function');
    });

    it('only gives node integration to workers created by frames that have it themselves', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegrationInWorker: true,
          contextIsolation: false
        }
      });
      await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
      const probe = `new Promise((resolve) => {
        const src = 'postMessage([typeof process, typeof require].join(" "))';
        const worker = new Worker(URL.createObjectURL(new Blob([src], { type: 'text/javascript' })));
        worker.onmessage = (e) => { worker.terminate(); resolve(e.data); };
      })`;
      // The main frame's workers get Node...
      expect(await w.webContents.mainFrame.executeJavaScript(probe)).toBe('object function');
      // ...a same-origin (same-process) iframe's workers do not, because the
      // iframe itself has no Node integration (nodeIntegrationInSubFrames is off).
      await w.webContents.executeJavaScript(`new Promise((resolve) => {
        const f = document.createElement('iframe');
        f.src = location.href;
        f.onload = resolve;
        document.body.appendChild(f);
      })`);
      const iframe = w.webContents.mainFrame.frames[0];
      expect(await iframe.executeJavaScript(probe)).toBe('undefined undefined');
    });

    it('gives node integration to subframe workers when nodeIntegrationInSubFrames is on', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegrationInWorker: true,
          nodeIntegrationInSubFrames: true,
          contextIsolation: false
        }
      });
      await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
      await w.webContents.executeJavaScript(`new Promise((resolve) => {
        const f = document.createElement('iframe');
        f.src = location.href;
        f.onload = resolve;
        document.body.appendChild(f);
      })`);
      const iframe = w.webContents.mainFrame.frames[0];
      const result = await iframe.executeJavaScript(`new Promise((resolve) => {
        const src = 'postMessage([typeof process, typeof require].join(" "))';
        const worker = new Worker(URL.createObjectURL(new Blob([src], { type: 'text/javascript' })));
        worker.onmessage = (e) => { worker.terminate(); resolve(e.data); };
      })`);
      expect(result).toBe('object function');
    });

    it('Worker does not have node integration when nodeIntegrationInWorker is disabled via setWindowOpenHandler', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          nodeIntegrationInWorker: true,
          contextIsolation: false
        }
      });

      w.webContents.setWindowOpenHandler(() => ({
        action: 'allow',
        overrideBrowserWindowOptions: {
          show: false,
          webPreferences: {
            nodeIntegration: false,
            nodeIntegrationInWorker: false,
            contextIsolation: true
          }
        }
      }));

      await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      const childCreated = once(app, 'browser-window-created') as Promise<[any, BrowserWindow]>;
      w.webContents.executeJavaScript(
        `window.open(${JSON.stringify(`file://${fixturesPath}/pages/blank.html`)}); void 0;`
      );
      const [, child] = await childCreated;
      await once(child.webContents, 'did-finish-load');

      const data = await child.webContents.executeJavaScript(`
        const worker = new Worker('../workers/worker_node.js');
        new Promise((resolve) => { worker.onmessage = e => resolve(e.data); })
      `);
      expect(data).toBe('undefined undefined undefined undefined');
    });

    it('Worker has node integration when nodeIntegrationInWorker is enabled via setWindowOpenHandler', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          nodeIntegrationInWorker: false,
          contextIsolation: false
        }
      });

      w.webContents.setWindowOpenHandler(() => ({
        action: 'allow',
        overrideBrowserWindowOptions: {
          show: false,
          webPreferences: {
            nodeIntegration: true,
            nodeIntegrationInWorker: true,
            contextIsolation: false
          }
        }
      }));

      await w.loadURL(`file://${fixturesPath}/pages/blank.html`);

      // Parent's workers should NOT have node integration.
      const parentData = await w.webContents.executeJavaScript(`
        new Promise((resolve) => {
          const worker = new Worker('../workers/worker_node.js');
          worker.onmessage = e => resolve(e.data);
        })
      `);
      expect(parentData).toBe('undefined undefined undefined undefined');

      const childCreated = once(app, 'browser-window-created') as Promise<[any, BrowserWindow]>;
      w.webContents.executeJavaScript(
        `window.open(${JSON.stringify(`file://${fixturesPath}/pages/blank.html`)}); void 0;`
      );
      const [, child] = await childCreated;
      await once(child.webContents, 'did-finish-load');

      // Child's workers should have node integration.
      const childData = await child.webContents.executeJavaScript(`
        new Promise((resolve) => {
          const worker = new Worker('../workers/worker_node.js');
          worker.onmessage = e => resolve(e.data);
        })
      `);
      expect(childData).toBe('object function object function');
    });

    it('Worker has access to fetch-dependent interfaces with nodeIntegrationInWorker', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          nodeIntegrationInWorker: true,
          contextIsolation: false
        }
      });

      w.loadURL(`file://${fixturesPath}/pages/worker-fetch.html`);
      const [, data] = await once(ipcMain, 'worker-fetch-result');
      expect(data).toBe('function function function function function');
    });

    it('AudioWorklet keeps node integration across pooled worker threads', async () => {
      // Regression test for https://github.com/electron/electron/issues/41263.
      // Blink pools the AudioWorklet backing thread (Chromium CL:5270028) so
      // the Nth+ AudioWorklet on a page reuses the same thread; the page
      // creates several AudioWorklet contexts in sequence and asserts node
      // integration is wired up in every one of them. Must match the value
      // hardcoded in spec/fixtures/pages/audio-worklet.html.
      const NUM_AUDIO_WORKLET_CONTEXTS = 6;
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          nodeIntegrationInWorker: true,
          contextIsolation: false
        }
      });

      w.loadURL(`file://${fixturesPath}/pages/audio-worklet.html`);
      const [, results] = await once(ipcMain, 'audio-worklet-result');
      expect(results).toBeInstanceOf(Array);
      expect(results).toHaveLength(NUM_AUDIO_WORKLET_CONTEXTS);
      for (const r of results) expect(r).toBe('ok');
    });

    describe('SharedWorker', () => {
      it('can work', async () => {
        const w = new BrowserWindow({ show: false });
        await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
        const data = await w.webContents.executeJavaScript(`
          const worker = new SharedWorker('../workers/shared_worker.js');
          const message = 'ping';
          const eventPromise = new Promise((resolve) => { worker.port.onmessage = e => resolve(e.data); });
          worker.port.postMessage(message);
          eventPromise
        `);
        expect(data).toBe('ping');
      });

      it('has no node integration by default', async () => {
        const w = new BrowserWindow({ show: false });
        await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
        const data = await w.webContents.executeJavaScript(`
          const worker = new SharedWorker('../workers/shared_worker_node.js');
          new Promise((resolve) => { worker.port.onmessage = e => resolve(e.data); })
        `);
        expect(data).toBe('undefined undefined undefined undefined');
      });

      it('does not have node integration with nodeIntegrationInWorker', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            nodeIntegrationInWorker: true,
            contextIsolation: false
          }
        });

        await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
        const data = await w.webContents.executeJavaScript(`
          const worker = new SharedWorker('../workers/shared_worker_node.js');
          new Promise((resolve) => { worker.port.onmessage = e => resolve(e.data); })
        `);
        expect(data).toBe('undefined undefined undefined undefined');
      });
    });
  });

  describe('form submit', () => {
    let server: http.Server;
    let serverUrl: string;

    beforeAll(async () => {
      server = http.createServer((req, res) => {
        let body = '';
        req.on('data', (chunk) => {
          body += chunk;
        });
        res.setHeader('Content-Type', 'application/json');
        req.on('end', () => {
          res.end(`body:${body}`);
        });
      });
      serverUrl = (await listen(server)).url;
    });
    afterAll(async () => {
      server.close();
      await closeAllWindows();
    });

    for (const isSandboxEnabled of [true, false]) {
      describe(`sandbox=${isSandboxEnabled}`, () => {
        it('posts data in the same window', async () => {
          const w = new BrowserWindow({
            show: false,
            webPreferences: {
              sandbox: isSandboxEnabled
            }
          });

          await w.loadFile(path.join(fixturesPath, 'pages', 'form-with-data.html'));

          const loadPromise = once(w.webContents, 'did-finish-load');

          w.webContents.executeJavaScript(`
            const form = document.querySelector('form')
            form.action = '${serverUrl}';
            form.submit();
          `);

          await loadPromise;

          const res = await w.webContents.executeJavaScript('document.body.innerText');
          expect(res).toBe('body:greeting=hello');
        });

        it('posts data to a new window with target=_blank', async () => {
          const w = new BrowserWindow({
            show: false,
            webPreferences: {
              sandbox: isSandboxEnabled
            }
          });

          await w.loadFile(path.join(fixturesPath, 'pages', 'form-with-data.html'));

          const windowCreatedPromise = once(app, 'browser-window-created') as Promise<[any, BrowserWindow]>;

          w.webContents.executeJavaScript(`
            const form = document.querySelector('form')
            form.action = '${serverUrl}';
            form.target = '_blank';
            form.submit();
          `);

          const [, newWin] = await windowCreatedPromise;

          const res = await newWin.webContents.executeJavaScript('document.body.innerText');
          expect(res).toBe('body:greeting=hello');
        });
      });
    }
  });

  describe('window.open', () => {
    for (const show of [true, false]) {
      it(`shows the child regardless of parent visibility when parent {show=${show}}`, async () => {
        const w = new BrowserWindow({ show });

        // toggle visibility
        if (show) {
          w.hide();
        } else {
          w.show();
        }

        defer(() => {
          w.close();
        });

        const promise = once(app, 'browser-window-created') as Promise<[any, BrowserWindow]>;
        w.loadFile(path.join(fixturesPath, 'pages', 'window-open.html'));
        const [, newWindow] = await promise;
        expect(newWindow.isVisible()).toBe(true);
      });
    }

    it('is always resizable', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadFile(path.resolve(import.meta.dirname, 'fixtures', 'blank.html'));
      w.webContents.executeJavaScript(`
        { b = window.open('about:blank', '', 'resizable=no,show=no'); null }
      `);
      const [, popup] = (await once(app, 'browser-window-created')) as [any, BrowserWindow];
      expect(popup.isResizable()).toBe(true);
    });

    // FIXME(zcbenz): This test is making the spec runner hang on exit on Windows.
    ifit(process.platform !== 'win32')(
      'disables node integration when it is disabled on the parent window',
      async () => {
        const windowUrl = url.pathToFileURL(path.join(fixturesPath, 'pages', 'window-opener-no-node-integration.html'));
        windowUrl.searchParams.set('p', `${fixturesPath}/pages/window-opener-node.html`);

        const w = new BrowserWindow({ show: false });
        w.loadFile(path.resolve(import.meta.dirname, 'fixtures', 'blank.html'));

        const { eventData } = await w.webContents.executeJavaScript(`(async () => {
        const message = new Promise(resolve => window.addEventListener('message', resolve, {once: true}));
        const b = window.open(${JSON.stringify(windowUrl)}, '', 'show=false')
        const e = await message
        b.close();
        return {
          eventData: e.data
        }
      })()`);

        expect(eventData.isProcessGlobalUndefined).toBe(true);
      }
    );

    it('disables node integration when it is disabled on the parent window for chrome devtools URLs', async () => {
      // NB. webSecurity is disabled because native window.open() is not
      // allowed to load devtools:// URLs.
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, webSecurity: false } });
      w.loadURL('about:blank');
      w.webContents.executeJavaScript(`
        { b = window.open('devtools://devtools/bundled/inspector.html', '', 'nodeIntegration=no,show=no'); null }
      `);
      const [, contents] = (await once(app, 'web-contents-created')) as [any, WebContents];
      const typeofProcessGlobal = await contents.executeJavaScript('typeof process');
      expect(typeofProcessGlobal).toBe('undefined');
    });

    it('can disable node integration when it is enabled on the parent window', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } });
      w.loadURL('about:blank');
      w.webContents.executeJavaScript(`
        { b = window.open('about:blank', '', 'nodeIntegration=no,show=no'); null }
      `);
      const [, contents] = (await once(app, 'web-contents-created')) as [any, WebContents];
      const typeofProcessGlobal = await contents.executeJavaScript('typeof process');
      expect(typeofProcessGlobal).toBe('undefined');
    });

    // TODO(jkleinsc) fix this flaky test on WOA
    ifit(process.platform !== 'win32' || process.arch !== 'arm64')(
      'disables JavaScript when it is disabled on the parent window',
      async () => {
        const w = new BrowserWindow({ show: true, webPreferences: { nodeIntegration: true } });
        w.webContents.loadFile(path.resolve(import.meta.dirname, 'fixtures', 'blank.html'));
        const windowUrl = url.format({
          pathname: `${fixturesPath}/pages/window-no-javascript.html`,
          protocol: 'file',
          slashes: true
        });
        w.webContents.executeJavaScript(`
        { b = window.open(${JSON.stringify(windowUrl)}, '', 'javascript=no,show=no'); null }
      `);
        const [, contents] = (await once(app, 'web-contents-created')) as [any, WebContents];
        await once(contents, 'did-finish-load');
        // Click link on page
        contents.sendInputEvent({ type: 'mouseDown', clickCount: 1, x: 1, y: 1 });
        contents.sendInputEvent({ type: 'mouseUp', clickCount: 1, x: 1, y: 1 });
        const [, window] = (await once(app, 'browser-window-created')) as [any, BrowserWindow];
        const preferences = window.webContents.getLastWebPreferences();
        expect(preferences!.javascript).toBe(false);
      }
    );

    it('defines a window.location getter', async () => {
      let targetURL: string;
      if (process.platform === 'win32') {
        targetURL = `file:///${fixturesPath.replaceAll('\\', '/')}/pages/base-page.html`;
      } else {
        targetURL = `file://${fixturesPath}/pages/base-page.html`;
      }
      const w = new BrowserWindow({ show: false });
      w.webContents.loadFile(path.resolve(import.meta.dirname, 'fixtures', 'blank.html'));
      w.webContents.executeJavaScript(`{ b = window.open(${JSON.stringify(targetURL)}); null }`);
      const [, window] = (await once(app, 'browser-window-created')) as [any, BrowserWindow];
      await once(window.webContents, 'did-finish-load');
      expect(await w.webContents.executeJavaScript('b.location.href')).toBe(targetURL);
    });

    it('defines a window.location setter', async () => {
      const w = new BrowserWindow({ show: false });
      w.webContents.loadFile(path.resolve(import.meta.dirname, 'fixtures', 'blank.html'));
      w.webContents.executeJavaScript('{ b = window.open("about:blank"); null }');
      const [, { webContents }] = (await once(app, 'browser-window-created')) as [any, BrowserWindow];
      await once(webContents, 'did-finish-load');
      // When it loads, redirect
      w.webContents.executeJavaScript(
        `{ b.location = ${JSON.stringify(`file://${fixturesPath}/pages/base-page.html`)}; null }`
      );
      await once(webContents, 'did-finish-load');
    });

    it('defines a window.location.href setter', async () => {
      const w = new BrowserWindow({ show: false });
      w.webContents.loadFile(path.resolve(import.meta.dirname, 'fixtures', 'blank.html'));
      w.webContents.executeJavaScript('{ b = window.open("about:blank"); null }');
      const [, { webContents }] = (await once(app, 'browser-window-created')) as [any, BrowserWindow];
      await once(webContents, 'did-finish-load');
      // When it loads, redirect
      w.webContents.executeJavaScript(
        `{ b.location.href = ${JSON.stringify(`file://${fixturesPath}/pages/base-page.html`)}; null }`
      );
      await once(webContents, 'did-finish-load');
    });

    it('open a blank page when no URL is specified', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadURL('about:blank');
      w.webContents.executeJavaScript('{ b = window.open(); null }');
      const [, { webContents }] = (await once(app, 'browser-window-created')) as [any, BrowserWindow];
      await once(webContents, 'did-finish-load');
      expect(await w.webContents.executeJavaScript('b.location.href')).toBe('about:blank');
    });

    it('open a blank page when an empty URL is specified', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadURL('about:blank');
      w.webContents.executeJavaScript("{ b = window.open(''); null }");
      const [, { webContents }] = (await once(app, 'browser-window-created')) as [any, BrowserWindow];
      await once(webContents, 'did-finish-load');
      expect(await w.webContents.executeJavaScript('b.location.href')).toBe('about:blank');
    });

    it('does not throw an exception when the frameName is a built-in object property', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadURL('about:blank');
      w.webContents.executeJavaScript("{ b = window.open('', '__proto__'); null }");
      const frameName = await new Promise((resolve) => {
        w.webContents.setWindowOpenHandler((details) => {
          setImmediate(() => resolve(details.frameName));
          return { action: 'allow' };
        });
      });
      expect(frameName).toBe('__proto__');
    });

    it('works when used in conjunction with the vm module', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false
        }
      });

      await w.loadFile(path.resolve(import.meta.dirname, 'fixtures', 'blank.html'));

      const { contextObject } = await w.webContents.executeJavaScript(`(async () => {
        const vm = require('node:vm');
        const contextObject = { count: 1, type: 'gecko' };
        window.open('');
        vm.runInNewContext('count += 1; type = "chameleon";', contextObject);
        return { contextObject };
      })()`);

      expect(contextObject).toEqual({ count: 2, type: 'chameleon' });
    });

    // FIXME(nornagon): I'm not sure this ... ever was correct?
    it.skip('inherit options of parent window', async () => {
      const w = new BrowserWindow({ show: false, width: 123, height: 456 });
      w.loadFile(path.resolve(import.meta.dirname, 'fixtures', 'blank.html'));
      const url = `file://${fixturesPath}/pages/window-open-size.html`;
      const { width, height, eventData } = await w.webContents.executeJavaScript(`(async () => {
        const message = new Promise(resolve => window.addEventListener('message', resolve, {once: true}));
        const b = window.open(${JSON.stringify(url)}, '', 'show=false')
        const e = await message
        b.close();
        const width = outerWidth;
        const height = outerHeight;
        return {
          width,
          height,
          eventData: e.data
        }
      })()`);
      expect(eventData).toBe(`size: ${width} ${height}`);
      expect(eventData).toBe('size: 123 456');
    });

    it('does not override child options', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadFile(path.resolve(import.meta.dirname, 'fixtures', 'blank.html'));
      const windowUrl = `file://${fixturesPath}/pages/window-open-size.html`;
      const { eventData } = await w.webContents.executeJavaScript(`(async () => {
        const message = new Promise(resolve => window.addEventListener('message', resolve, {once: true}));
        const b = window.open(${JSON.stringify(windowUrl)}, '', 'show=no,width=350,height=450')
        const e = await message
        b.close();
        return { eventData: e.data }
      })()`);
      expect(eventData).toBe('size: 350 450');
    });

    it('window opened with innerWidth option has the same innerWidth', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadFile(path.resolve(import.meta.dirname, 'fixtures', 'blank.html'));
      const windowUrl = `file://${fixturesPath}/pages/window-open-size-inner.html`;
      const windowCreatedPromise = once(app, 'browser-window-created') as Promise<[any, BrowserWindow]>;
      const eventDataPromise = w.webContents.executeJavaScript(`(async () => {
        const message = new Promise(resolve => window.addEventListener('message', resolve, { once: true }));
        b = window.open(${JSON.stringify(windowUrl)}, '', 'show=no,innerWidth=400,height=450');
        const e = await message;
        b.close();
        return e.data;
      })()`);
      const [[, newWindow], eventData] = await Promise.all([windowCreatedPromise, eventDataPromise]);

      expect(newWindow.getContentSize().toString()).toBe('400,450');
      expect(eventData).toBe('size: 400 450');
    });
    it('window opened with innerHeight option has the same innerHeight', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadFile(path.resolve(import.meta.dirname, 'fixtures', 'blank.html'));
      const windowUrl = `file://${fixturesPath}/pages/window-open-size-inner.html`;
      const windowCreatedPromise = once(app, 'browser-window-created') as Promise<[any, BrowserWindow]>;
      const eventDataPromise = w.webContents.executeJavaScript(`(async () => {
        const message = new Promise(resolve => window.addEventListener('message', resolve, {once: true}));
        const b = window.open(${JSON.stringify(windowUrl)}, '', 'show=no,width=350,innerHeight=400')
        const e = await message;
        b.close();
        return e.data;
      })()`);
      const [[, newWindow], eventData] = await Promise.all([windowCreatedPromise, eventDataPromise]);

      expect(newWindow.getContentSize().toString()).toBe('350,400');
      expect(eventData).toBe('size: 350 400');
    });

    it('loads preload script after setting opener to null', async () => {
      const w = new BrowserWindow({ show: false });
      w.webContents.setWindowOpenHandler(() => ({
        action: 'allow',
        overrideBrowserWindowOptions: {
          webPreferences: {
            preload: path.join(fixturesPath, 'module', 'preload.js')
          }
        }
      }));
      w.loadURL('about:blank');
      w.webContents.executeJavaScript('window.child = window.open(); child.opener = null');
      const [, { webContents }] = await once(app, 'browser-window-created');
      const [{ message }] = await once(webContents, 'console-message');
      expect(message).toBe(
        '{"require":"function","module":"object","exports":"object","process":"object","Buffer":"function"}'
      );
    });

    it('disables the <webview> tag when it is disabled on the parent window', async () => {
      const windowUrl = url.pathToFileURL(path.join(fixturesPath, 'pages', 'window-opener-no-webview-tag.html'));
      windowUrl.searchParams.set('p', `${fixturesPath}/pages/window-opener-webview.html`);

      const w = new BrowserWindow({ show: false });
      w.loadFile(path.resolve(import.meta.dirname, 'fixtures', 'blank.html'));

      const { eventData } = await w.webContents.executeJavaScript(`(async () => {
        const message = new Promise(resolve => window.addEventListener('message', resolve, {once: true}));
        const b = window.open(${JSON.stringify(windowUrl)}, '', 'webviewTag=no,contextIsolation=no,nodeIntegration=yes,show=no')
        const e = await message
        b.close();
        return { eventData: e.data }
      })()`);
      expect(eventData.isWebViewGlobalUndefined).toBe(true);
    });

    it('throws an exception when the arguments cannot be converted to strings', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadURL('about:blank');
      await expect(w.webContents.executeJavaScript("window.open('', { toString: null })")).rejects.toThrow();

      await expect(w.webContents.executeJavaScript("window.open('', '', { toString: 3 })")).rejects.toThrow();
    });

    it('does not throw an exception when the features include webPreferences', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadURL('about:blank');
      await w.webContents.executeJavaScript(
        "const b = window.open('', '', 'show=no,webPreferences='); b.close(); null"
      );
    });
  });

  describe('window.opener', () => {
    it('is null for main window', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false
        }
      });
      w.loadFile(path.join(fixturesPath, 'pages', 'window-opener.html'));
      const [, channel, opener] = await once(w.webContents, 'ipc-message');
      expect(channel).toBe('opener');
      expect(opener).toBe(null);
    });

    it('is not null for window opened by window.open', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false
        }
      });
      w.loadFile(path.resolve(import.meta.dirname, 'fixtures', 'blank.html'));

      const windowUrl = `file://${fixturesPath}/pages/window-opener.html`;
      const eventData = await w.webContents.executeJavaScript(`
        const b = window.open(${JSON.stringify(windowUrl)}, '', 'show=no');
        new Promise(resolve => window.addEventListener('message', resolve, {once: true})).then(e => e.data);
      `);
      expect(eventData).toBe('object');
    });
  });

  describe('window.opener.postMessage', () => {
    it('sets source and origin correctly', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadFile(path.resolve(import.meta.dirname, 'fixtures', 'blank.html'));

      const windowUrl = `file://${fixturesPath}/pages/window-opener-postMessage.html`;
      const { sourceIsChild, origin } = await w.webContents.executeJavaScript(`
        const b = window.open(${JSON.stringify(windowUrl)}, '', 'show=no');
        new Promise(resolve => window.addEventListener('message', resolve, {once: true})).then(e => ({
          sourceIsChild: e.source === b,
          origin: e.origin
        }));
      `);

      expect(sourceIsChild).toBe(true);
      expect(origin).toBe('null');
    });

    it('supports windows opened from a <webview>', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { webviewTag: true } });
      w.loadURL('about:blank');
      const childWindowUrl = url.pathToFileURL(path.join(fixturesPath, 'pages', 'webview-opener-postMessage.html'));
      childWindowUrl.searchParams.set('p', `${fixturesPath}/pages/window-opener-postMessage.html`);
      const message = await w.webContents.executeJavaScript(`
        const webview = new WebView();
        webview.allowpopups = true;
        webview.setAttribute('webpreferences', 'contextIsolation=no');
        webview.src = ${JSON.stringify(childWindowUrl)}
        const consoleMessage = new Promise(resolve => webview.addEventListener('console-message', resolve, {once: true}));
        document.body.appendChild(webview);
        consoleMessage.then(e => e.message)
      `);

      expect(message).toBe('message');
    });

    describe('targetOrigin argument', () => {
      let serverURL: string;
      let server: any;

      beforeEach(async () => {
        server = http.createServer((req, res) => {
          res.writeHead(200);
          const filePath = path.join(fixturesPath, 'pages', 'window-opener-targetOrigin.html');
          res.end(fs.readFileSync(filePath, 'utf8'));
        });
        serverURL = (await listen(server)).url;
      });

      afterEach(() => {
        server.close();
      });

      it('delivers messages that match the origin', async () => {
        const w = new BrowserWindow({ show: false });
        w.loadFile(path.resolve(import.meta.dirname, 'fixtures', 'blank.html'));
        const data = await w.webContents.executeJavaScript(`
          window.open(${JSON.stringify(serverURL)}, '', 'show=no');
          new Promise(resolve => window.addEventListener('message', resolve, {once: true})).then(e => e.data)
        `);
        expect(data).toBe('deliver');
      });
    });
  });

  describe('Storage Access API', () => {
    afterEach(closeAllWindows);
    afterEach(() => {
      session.defaultSession.setPermissionCheckHandler(null);
      session.defaultSession.setPermissionRequestHandler(null);
    });

    it('can determine if a permission is granted for "storage-access"', async () => {
      session.defaultSession.setPermissionCheckHandler((_wc, permission) => permission === 'storage-access');

      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixturesPath, 'pages', 'a.html'));

      const permission = await w.webContents.executeJavaScript(
        `
        navigator.permissions.query({ name: 'storage-access' })
          .then(permission => permission.state).catch(err => err.message);
      `,
        true
      );

      expect(permission).toBe('granted');
    });

    it('can determine if a permission is denied for "storage-access"', async () => {
      session.defaultSession.setPermissionCheckHandler((_wc, permission) => permission !== 'storage-access');

      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixturesPath, 'pages', 'a.html'));

      const permission = await w.webContents.executeJavaScript(
        `
        navigator.permissions.query({ name: 'storage-access' })
          .then(permission => permission.state).catch(err => err.message);
      `,
        true
      );

      expect(permission).toBe('denied');
    });

    it('can determine if a permission is granted for "top-level-storage-access"', async () => {
      session.defaultSession.setPermissionCheckHandler((_wc, permission) => permission === 'top-level-storage-access');

      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixturesPath, 'pages', 'a.html'));

      const permission = await w.webContents.executeJavaScript(
        `
        navigator.permissions.query({
          name: 'top-level-storage-access',
          requestedOrigin: "https://www.example.com",
        }).then(permission => permission.state).catch(err => err.message);
      `,
        true
      );

      expect(permission).toBe('granted');
    });

    it('can determine if a permission is denied for "top-level-storage-access"', async () => {
      session.defaultSession.setPermissionCheckHandler((_wc, permission) => permission !== 'top-level-storage-access');

      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixturesPath, 'pages', 'a.html'));

      const permission = await w.webContents.executeJavaScript(
        `
        navigator.permissions.query({
          name: 'top-level-storage-access',
          requestedOrigin: "https://www.example.com",
        }).then(permission => permission.state).catch(err => err.message);
      `,
        true
      );

      expect(permission).toBe('denied');
    });

    it('can grant a permission request for "top-level-storage-access"', async () => {
      session.defaultSession.setPermissionRequestHandler((_wc, permission, callback) => {
        callback(permission === 'top-level-storage-access');
      });

      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixturesPath, 'pages', 'button.html'));

      // requestStorageAccessFor returns a Promise that fulfills with undefined
      // if the access to third-party cookies was granted and rejects if access was denied.
      const permission = await w.webContents.executeJavaScript(
        `
        new Promise((resolve, reject) => {
          const button = document.getElementById('button');
          button.addEventListener("click", () => {
            document.requestStorageAccessFor('https://myfakesite').then(
              (res) => { resolve('granted') },
              (err) => { resolve('denied') },
            );
          });
          button.click();
        });
      `,
        true
      );

      expect(permission).toBe('granted');
    });

    it('can deny a permission request for "top-level-storage-access"', async () => {
      session.defaultSession.setPermissionRequestHandler((_wc, permission, callback) => {
        callback(permission !== 'top-level-storage-access');
      });

      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixturesPath, 'pages', 'button.html'));

      // requestStorageAccessFor returns a Promise that fulfills with undefined
      // if the access to third-party cookies was granted and rejects if access was denied.
      const permission = await w.webContents.executeJavaScript(
        `
        new Promise((resolve, reject) => {
          const button = document.getElementById('button');
          button.addEventListener("click", () => {
            document.requestStorageAccessFor('https://myfakesite').then(
              (res) => { resolve('granted') },
              (err) => { resolve('denied') },
            );
          });
          button.click();
        });
      `,
        true
      );

      expect(permission).toBe('denied');
    });
  });

  describe('IdleDetection', () => {
    afterEach(closeAllWindows);
    afterEach(() => {
      session.defaultSession.setPermissionCheckHandler(null);
      session.defaultSession.setPermissionRequestHandler(null);
    });

    it('can grant a permission request', async () => {
      session.defaultSession.setPermissionRequestHandler((_wc, permission, callback) => {
        callback(permission === 'idle-detection');
      });

      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixturesPath, 'pages', 'button.html'));
      const permission = await w.webContents.executeJavaScript(
        `
        new Promise((resolve, reject) => {
          const button = document.getElementById('button');
          button.addEventListener("click", async () => {
            const permission = await IdleDetector.requestPermission();
            resolve(permission);
          });
          button.click();
        });
      `,
        true
      );

      expect(permission).toBe('granted');
    });

    it('can deny a permission request', async () => {
      session.defaultSession.setPermissionRequestHandler((_wc, permission, callback) => {
        callback(permission !== 'idle-detection');
      });

      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixturesPath, 'pages', 'button.html'));
      const permission = await w.webContents.executeJavaScript(
        `
        new Promise((resolve, reject) => {
          const button = document.getElementById('button');
          button.addEventListener("click", async () => {
            const permission = await IdleDetector.requestPermission();
            resolve(permission);
          });
          button.click();
        });
      `,
        true
      );

      expect(permission).toBe('denied');
    });

    it('can allow the IdleDetector to start', async () => {
      session.defaultSession.setPermissionCheckHandler((wc, permission) => {
        return permission === 'idle-detection';
      });

      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
      const result = await w.webContents.executeJavaScript(
        `
        const detector = new IdleDetector({ threshold: 60000 });
        detector.start().then(() => {
          return 'success';
        }).catch(e => e.message);
      `,
        true
      );

      expect(result).toBe('success');
    });

    it('can prevent the IdleDetector from starting', async () => {
      session.defaultSession.setPermissionCheckHandler((wc, permission) => {
        return permission !== 'idle-detection';
      });

      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
      const result = await w.webContents.executeJavaScript(
        `
        const detector = new IdleDetector({ threshold: 60000 });
        detector.start().then(() => {
          console.log('success')
        }).catch(e => e.message);
      `,
        true
      );

      expect(result).toBe('Idle detection permission denied');
    });
  });

  describe('navigator.mediaDevices', () => {
    afterEach(closeAllWindows);
    afterEach(() => {
      session.defaultSession.setPermissionCheckHandler(null);
      session.defaultSession.setPermissionRequestHandler(null);
    });

    it('can return labels of enumerated devices', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
      const labels = await w.webContents.executeJavaScript(
        'navigator.mediaDevices.enumerateDevices().then(ds => ds.map(d => d.label))'
      );
      expect(labels.some((l: any) => l)).toBe(true);
    });

    it('does not return labels of enumerated devices when all permission denied', async () => {
      session.defaultSession.setPermissionCheckHandler(() => false);
      const w = new BrowserWindow({ show: false });
      w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
      const labels = await w.webContents.executeJavaScript(
        'navigator.mediaDevices.enumerateDevices().then(ds => ds.map(d => d.label))'
      );
      expect(labels.some((l: any) => l)).toBe(false);
    });

    it('does not return labels of enumerated audio devices when permission denied', async () => {
      session.defaultSession.setPermissionCheckHandler((wc, permission, origin, { mediaType }) => {
        return permission !== 'media' || mediaType !== 'audio';
      });
      const w = new BrowserWindow({ show: false });
      w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
      const devices = await w.webContents.executeJavaScript(
        `navigator.mediaDevices.enumerateDevices().then(ds => ds.map(d => {
          return ({ label: d.label, kind: d.kind })
        }));
      `
      );
      const audioDevices = devices.filter((d: any) => d.kind === 'audioinput');
      expect(audioDevices.some((d: any) => d.label)).toBe(false);
      const videoDevices = devices.filter((d: any) => d.kind === 'videoinput');
      expect(videoDevices.some((d: any) => d.label)).toBe(true);
    });

    it('does not return labels of enumerated video devices when permission denied', async () => {
      session.defaultSession.setPermissionCheckHandler((wc, permission, origin, { mediaType }) => {
        return permission !== 'media' || mediaType !== 'video';
      });
      const w = new BrowserWindow({ show: false });
      w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
      const devices = await w.webContents.executeJavaScript(
        `navigator.mediaDevices.enumerateDevices().then(ds => ds.map(d => {
          return ({ label: d.label, kind: d.kind })
        }));
      `
      );
      const audioDevices = devices.filter((d: any) => d.kind === 'audioinput');
      expect(audioDevices.some((d: any) => d.label)).toBe(true);
      const videoDevices = devices.filter((d: any) => d.kind === 'videoinput');
      expect(videoDevices.some((d: any) => d.label)).toBe(false);
    });

    it('returns the same device ids across reloads', async () => {
      const ses = session.fromPartition('persist:media-device-id');
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          session: ses,
          contextIsolation: false
        }
      });
      w.loadFile(path.join(fixturesPath, 'pages', 'media-id-reset.html'));
      const [, firstDeviceIds] = await once(ipcMain, 'deviceIds');
      w.webContents.reload();
      const [, secondDeviceIds] = await once(ipcMain, 'deviceIds');
      expect(firstDeviceIds).toEqual(secondDeviceIds);
    });

    it('can return new device id when cookie storage is cleared', async () => {
      const ses = session.fromPartition('persist:media-device-id');
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          session: ses,
          contextIsolation: false
        }
      });
      w.loadFile(path.join(fixturesPath, 'pages', 'media-id-reset.html'));
      const [, firstDeviceIds] = await once(ipcMain, 'deviceIds');
      await ses.clearStorageData({ storages: ['cookies'] });
      w.webContents.reload();
      const [, secondDeviceIds] = await once(ipcMain, 'deviceIds');
      expect(firstDeviceIds).not.toEqual(secondDeviceIds);
    });

    it('provides a securityOrigin to the request handler', async () => {
      session.defaultSession.setPermissionRequestHandler((wc, permission, callback, details) => {
        if ((details as MediaAccessPermissionRequest).securityOrigin !== undefined) {
          callback(true);
        } else {
          callback(false);
        }
      });
      const w = new BrowserWindow({ show: false });
      w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
      const labels = await w.webContents.executeJavaScript(`navigator.mediaDevices.getUserMedia({
          video: {
            mandatory: {
              chromeMediaSource: "desktop",
              minWidth: 1280,
              maxWidth: 1280,
              minHeight: 720,
              maxHeight: 720
            }
          }
        }).then((stream) => stream.getVideoTracks())`);
      expect(labels.some((l: any) => l)).toBe(true);
    });

    it('fails with "not supported" for getDisplayMedia', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
      const { ok, err } = await w.webContents.executeJavaScript(
        'navigator.mediaDevices.getDisplayMedia({video: true}).then(s => ({ok: true}), e => ({ok: false, err: e.message}))',
        true
      );
      expect(ok).toBe(false);
      expect(err).toBe('Not supported');
    });
  });

  describe('window.opener access', () => {
    const scheme = 'app';

    const fileUrl = `file://${fixturesPath}/pages/window-opener-location.html`;
    const httpUrl1 = `${scheme}://origin1`;
    const httpUrl2 = `${scheme}://origin2`;
    const fileBlank = `file://${fixturesPath}/pages/blank.html`;
    const httpBlank = `${scheme}://origin1/blank`;

    const table = [
      { parent: fileBlank, child: httpUrl1, nodeIntegration: false, openerAccessible: false },
      { parent: fileBlank, child: httpUrl1, nodeIntegration: true, openerAccessible: false },

      // {parent: httpBlank, child: fileUrl, nodeIntegration: false, openerAccessible: false}, // can't window.open()
      // {parent: httpBlank, child: fileUrl, nodeIntegration: true, openerAccessible: false}, // can't window.open()

      // NB. this is different from Chrome's behavior, which isolates file: urls from each other
      { parent: fileBlank, child: fileUrl, nodeIntegration: false, openerAccessible: true },
      { parent: fileBlank, child: fileUrl, nodeIntegration: true, openerAccessible: true },

      { parent: httpBlank, child: httpUrl1, nodeIntegration: false, openerAccessible: true },
      { parent: httpBlank, child: httpUrl1, nodeIntegration: true, openerAccessible: true },

      { parent: httpBlank, child: httpUrl2, nodeIntegration: false, openerAccessible: false },
      { parent: httpBlank, child: httpUrl2, nodeIntegration: true, openerAccessible: false }
    ];
    const s = (url: string) => (url.startsWith('file') ? 'file://...' : url);

    beforeAll(() => {
      protocol.registerFileProtocol(scheme, (request, callback) => {
        if (request.url.includes('blank')) {
          callback(`${fixturesPath}/pages/blank.html`);
        } else {
          callback(`${fixturesPath}/pages/window-opener-location.html`);
        }
      });
    });
    afterAll(() => {
      protocol.unregisterProtocol(scheme);
    });
    afterEach(closeAllWindows);

    describe('when opened from main window', () => {
      for (const { parent, child, nodeIntegration, openerAccessible } of table) {
        for (const sandboxPopup of [false, true]) {
          const description = `when parent=${s(parent)} opens child=${s(child)} with nodeIntegration=${nodeIntegration} sandboxPopup=${sandboxPopup}, child should ${openerAccessible ? '' : 'not '}be able to access opener`;
          it(description, async () => {
            const w = new BrowserWindow({
              show: true,
              webPreferences: { nodeIntegration: true, contextIsolation: false }
            });
            w.webContents.setWindowOpenHandler(() => ({
              action: 'allow',
              overrideBrowserWindowOptions: {
                webPreferences: {
                  sandbox: sandboxPopup
                }
              }
            }));
            await w.loadURL(parent);
            const childOpenerLocation = await w.webContents.executeJavaScript(`new Promise(resolve => {
              window.addEventListener('message', function f(e) {
                resolve(e.data)
              })
              window.open(${JSON.stringify(child)}, "", "show=no,nodeIntegration=${nodeIntegration ? 'yes' : 'no'}")
            })`);
            if (openerAccessible) {
              expect(childOpenerLocation).toBeTypeOf('string');
            } else {
              expect(childOpenerLocation).toBeNull();
            }
          });
        }
      }
    });

    describe('when opened from <webview>', () => {
      for (const { parent, child, nodeIntegration, openerAccessible } of table) {
        const description = `when parent=${s(parent)} opens child=${s(child)} with nodeIntegration=${nodeIntegration}, child should ${openerAccessible ? '' : 'not '}be able to access opener`;
        it(description, async () => {
          // This test involves three contexts:
          //  1. The root BrowserWindow in which the test is run,
          //  2. A <webview> belonging to the root window,
          //  3. A window opened by calling window.open() from within the <webview>.
          // We are testing whether context (3) can access context (2) under various conditions.

          // This is context (1), the base window for the test.
          const w = new BrowserWindow({
            show: false,
            webPreferences: { nodeIntegration: true, webviewTag: true, contextIsolation: false }
          });
          await w.loadURL('about:blank');

          const parentCode = `new Promise((resolve) => {
            // This is context (3), a child window of the WebView.
            const child = window.open(${JSON.stringify(child)}, "", "show=no,contextIsolation=no,nodeIntegration=yes")
            window.addEventListener("message", e => {
              resolve(e.data)
            })
          })`;
          const childOpenerLocation = await w.webContents.executeJavaScript(`new Promise((resolve, reject) => {
            // This is context (2), a WebView which will call window.open()
            const webview = new WebView()
            webview.setAttribute('nodeintegration', '${nodeIntegration ? 'on' : 'off'}')
            webview.setAttribute('webpreferences', 'contextIsolation=no')
            webview.setAttribute('allowpopups', 'on')
            webview.src = ${JSON.stringify(parent + '?p=' + encodeURIComponent(child))}
            webview.addEventListener('dom-ready', async () => {
              webview.executeJavaScript(${JSON.stringify(parentCode)}).then(resolve, reject)
            })
            document.body.appendChild(webview)
          })`);
          if (openerAccessible) {
            expect(childOpenerLocation).toBeTypeOf('string');
          } else {
            expect(childOpenerLocation).toBeNull();
          }
        });
      }
    });
  });

  describe('storage', () => {
    describe('custom non standard schemes', () => {
      const protocolName = 'storage';
      let contents: WebContents;
      beforeAll(() => {
        protocol.registerFileProtocol(protocolName, (request, callback) => {
          const parsedUrl = new URL(request.url);
          let filename;
          switch (parsedUrl.pathname) {
            case '/localStorage':
              filename = 'local_storage.html';
              break;
            case '/sessionStorage':
              filename = 'session_storage.html';
              break;
            case '/indexedDB':
              filename = 'indexed_db.html';
              break;
            case '/cookie':
              filename = 'cookie.html';
              break;
            default:
              filename = '';
          }
          callback({ path: `${fixturesPath}/pages/storage/${filename}` });
        });
      });

      afterAll(() => {
        protocol.unregisterProtocol(protocolName);
      });

      beforeEach(() => {
        contents = (webContents as typeof ElectronInternal.WebContents).create({
          nodeIntegration: true,
          contextIsolation: false
        });
      });

      afterEach(() => {
        contents.destroy();
        contents = null as any;
      });

      it('cannot access localStorage', async () => {
        const response = once(ipcMain, 'local-storage-response');
        contents.loadURL(protocolName + '://host/localStorage');
        const [, error] = await response;
        expect(error).toBe(
          "Failed to read the 'localStorage' property from 'Window': Access is denied for this document."
        );
      });

      it('cannot access sessionStorage', async () => {
        const response = once(ipcMain, 'session-storage-response');
        contents.loadURL(`${protocolName}://host/sessionStorage`);
        const [, error] = await response;
        expect(error).toBe(
          "Failed to read the 'sessionStorage' property from 'Window': Access is denied for this document."
        );
      });

      it('cannot access indexedDB', async () => {
        const response = once(ipcMain, 'indexed-db-response');
        contents.loadURL(`${protocolName}://host/indexedDB`);
        const [, error] = await response;
        expect(error).toBe(
          "Failed to execute 'open' on 'IDBFactory': access to the Indexed Database API is denied in this context."
        );
      });

      it('cannot access cookie', async () => {
        const response = once(ipcMain, 'cookie-response');
        contents.loadURL(`${protocolName}://host/cookie`);
        const [, error] = await response;
        expect(error).toBe("Failed to set the 'cookie' property on 'Document': Access is denied for this document.");
      });
    });

    describe('can be accessed', () => {
      let server: http.Server;
      let serverUrl: string;
      let serverCrossSiteUrl: string;
      beforeAll(async () => {
        server = http.createServer((req, res) => {
          const respond = () => {
            if (req.url === '/redirect-cross-site') {
              res.setHeader('Location', `${serverCrossSiteUrl}/redirected`);
              res.statusCode = 302;
              res.end();
            } else if (req.url === '/redirected') {
              res.end('<html><script>window.localStorage</script></html>');
            } else {
              res.end();
            }
          };
          setTimeout().then(respond);
        });
        serverUrl = (await listen(server)).url;
        serverCrossSiteUrl = serverUrl.replace('127.0.0.1', 'localhost');
      });

      afterAll(() => {
        server.close();
        server = null as any;
      });

      afterEach(closeAllWindows);

      const testLocalStorageAfterXSiteRedirect = (testTitle: string, extraPreferences = {}) => {
        it(testTitle, async () => {
          const w = new BrowserWindow({
            show: false,
            ...extraPreferences
          });
          let redirectedTo: string | undefined;
          w.webContents.on('did-redirect-navigation', (event, url) => {
            redirectedTo = url;
          });
          await w.loadURL(`${serverUrl}/redirect-cross-site`);
          expect(redirectedTo, 'didnt redirect').toBe(`${serverCrossSiteUrl}/redirected`);
        });
      };

      testLocalStorageAfterXSiteRedirect('after a cross-site redirect');
      testLocalStorageAfterXSiteRedirect('after a cross-site redirect in sandbox mode', { sandbox: true });
    });

    describe('DOM storage quota increase', () => {
      for (const storageName of ['localStorage', 'sessionStorage']) {
        it(`allows saving at least 40MiB in ${storageName}`, async () => {
          const w = new BrowserWindow({ show: false });
          w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
          // Although JavaScript strings use UTF-16, the underlying
          // storage provider may encode strings differently, muddling the
          // translation between character and byte counts. However,
          // a string of 40 * 2^20 characters will require at least 40MiB
          // and presumably no more than 80MiB, a size guaranteed to
          // to exceed the original 10MiB quota yet stay within the
          // new 100MiB quota.
          // Note that both the key name and value affect the total size.
          const testKeyName = '_electronDOMStorageQuotaIncreasedTest';
          const length = 40 * Math.pow(2, 20) - testKeyName.length;
          await w.webContents.executeJavaScript(`
            ${storageName}.setItem(${JSON.stringify(testKeyName)}, 'X'.repeat(${length}));
          `);
          // Wait at least one turn of the event loop to help avoid false positives
          // Although not entirely necessary, the previous version of this test case
          // failed to detect a real problem (perhaps related to DOM storage data caching)
          // wherein calling `getItem` immediately after `setItem` would appear to work
          // but then later (e.g. next tick) it would not.
          await setTimeout(1);
          try {
            const storedLength = await w.webContents.executeJavaScript(
              `${storageName}.getItem(${JSON.stringify(testKeyName)}).length`
            );
            expect(storedLength).toBe(length);
          } finally {
            await w.webContents.executeJavaScript(`${storageName}.removeItem(${JSON.stringify(testKeyName)});`);
          }
        });

        it(`throws when attempting to use more than 128MiB in ${storageName}`, async () => {
          const w = new BrowserWindow({ show: false });
          w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
          await expect(
            (async () => {
              const testKeyName = '_electronDOMStorageQuotaStillEnforcedTest';
              const length = 128 * Math.pow(2, 20) - testKeyName.length;
              try {
                await w.webContents.executeJavaScript(`
                ${storageName}.setItem(${JSON.stringify(testKeyName)}, 'X'.repeat(${length}));
              `);
              } finally {
                await w.webContents.executeJavaScript(`${storageName}.removeItem(${JSON.stringify(testKeyName)});`);
              }
            })()
          ).rejects.toThrow();
        });
      }
    });

    describe('persistent storage', () => {
      it('can be requested', async () => {
        const w = new BrowserWindow({ show: false });
        w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
        const grantedBytes = await w.webContents.executeJavaScript(`new Promise(resolve => {
          navigator.webkitPersistentStorage.requestQuota(1024 * 1024, resolve);
        })`);
        expect(grantedBytes).toBe(1048576);
      });
    });
  });

  ifdescribe(features.isPDFViewerEnabled())('PDF Viewer', () => {
    const pdfSource = url.format({
      pathname: path.join(import.meta.dirname, 'fixtures', 'cat.pdf').replaceAll('\\', '/'),
      protocol: 'file',
      slashes: true
    });

    it('successfully loads a PDF resource', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL(pdfSource);
      expect(w.getURL()).toBe(pdfSource);
    });

    it('loads a PDF resource in an in-memory session', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { partition: 'pdf-viewer-in-memory' } });
      await w.loadURL(pdfSource);
      // The viewer hosts the document in a frame of its own once its plugin is up.
      await waitUntil(() => w.webContents.mainFrame.framesInSubtree.filter((f) => f.url === pdfSource).length === 2);
    });

    it('successfully loads a PDF resource in a iframe', async () => {
      const w = new BrowserWindow({ show: false });

      const readyFrames = new Set<Electron.WebFrameMain['frameToken']>();
      w.webContents.on('frame-created', (_, { frame }) => {
        frame!.on('dom-ready', () => {
          readyFrames.add(frame!.frameToken);
        });
      });

      await w.loadFile(path.join(import.meta.dirname, 'fixtures', 'pages', 'pdf-in-iframe.html'));

      const frames = w.webContents.mainFrame.frames;
      expect(frames.length).toBe(1);
      expect(frames[0].url).toBe(pdfSource);
      expect(readyFrames).toContain(frames[0].frameToken);
    });
  });

  describe('window.history', () => {
    describe('window.history.pushState', () => {
      it('should push state after calling history.pushState() from the same url', async () => {
        const w = new BrowserWindow({ show: false });
        await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
        // History should have current page by now.
        expect(w.webContents.navigationHistory.length()).toBe(1);

        const waitCommit = once(w.webContents, 'navigation-entry-committed');
        w.webContents.executeJavaScript('window.history.pushState({}, "")');
        await waitCommit;
        // Initial page + pushed state.
        expect(w.webContents.navigationHistory.length()).toBe(2);
      });
    });

    describe('window.history.back', () => {
      it('should not allow sandboxed iframe to modify main frame state', async () => {
        const w = new BrowserWindow({ show: false });
        w.loadURL('data:text/html,<iframe sandbox="allow-scripts"></iframe>');
        await Promise.all([
          once(w.webContents, 'navigation-entry-committed'),
          once(w.webContents, 'did-frame-navigate'),
          once(w.webContents, 'did-navigate')
        ]);

        w.webContents.executeJavaScript('window.history.pushState(1, "")');
        await Promise.all([
          once(w.webContents, 'navigation-entry-committed'),
          once(w.webContents, 'did-navigate-in-page')
        ]);

        w.webContents.once('navigation-entry-committed' as any, () => {
          expect.fail('Unexpected navigation-entry-committed');
        });
        w.webContents.once('did-navigate-in-page', () => {
          expect.fail('Unexpected did-navigate-in-page');
        });
        await w.webContents.mainFrame.frames[0].executeJavaScript('window.history.back()');
        expect(await w.webContents.executeJavaScript('window.history.state')).toBe(1);
        expect(w.webContents.navigationHistory.getActiveIndex()).toBe(1);
      });
    });
  });

  describe('chrome:// pages', () => {
    const urls = [
      'chrome://accessibility',
      'chrome://gpu',
      'chrome://media-internals',
      'chrome://tracing',
      'chrome://webrtc-internals',
      'chrome://process-internals'
    ];

    for (const url of urls) {
      describe(url, () => {
        it('loads the page successfully', async () => {
          const w = new BrowserWindow({ show: false });
          await w.loadURL(url);
          const host = url.substring('chrome://'.length);
          const pageExists = await w.webContents.executeJavaScript(
            `window.hasOwnProperty('chrome') && window.location.host === '${host}'`
          );
          expect(pageExists).toBe(true);
        });
      });
    }
  });

  describe('document.hasFocus', { tags: ['serial'] }, () => {
    it('has correct value when multiple windows are opened', async () => {
      const w1 = new BrowserWindow({ show: true });
      const w2 = new BrowserWindow({ show: true });
      const w3 = new BrowserWindow({ show: false });
      await w1.loadFile(path.join(import.meta.dirname, 'fixtures', 'blank.html'));
      await w2.loadFile(path.join(import.meta.dirname, 'fixtures', 'blank.html'));
      await w3.loadFile(path.join(import.meta.dirname, 'fixtures', 'blank.html'));
      expect(webContents.getFocusedWebContents()?.id).toBe(w2.webContents.id);
      let focus = false;
      focus = await w1.webContents.executeJavaScript('document.hasFocus()');
      expect(focus).toBe(false);
      focus = await w2.webContents.executeJavaScript('document.hasFocus()');
      expect(focus).toBe(true);
      focus = await w3.webContents.executeJavaScript('document.hasFocus()');
      expect(focus).toBe(false);
    });
  });

  // https://developer.mozilla.org/en-US/docs/Web/API/NetworkInformation
  describe('navigator.connection', () => {
    it('returns the correct value', async () => {
      const w = new BrowserWindow({ show: false });

      w.webContents.session.enableNetworkEmulation({
        latency: 500,
        downloadThroughput: 6400,
        uploadThroughput: 6400
      });

      await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      const rtt = await w.webContents.executeJavaScript('navigator.connection.rtt');
      expect(rtt).toBeTypeOf('number');

      const downlink = await w.webContents.executeJavaScript('navigator.connection.downlink');
      expect(downlink).toBeTypeOf('number');

      const effectiveTypes = ['slow-2g', '2g', '3g', '4g'];
      const effectiveType = await w.webContents.executeJavaScript('navigator.connection.effectiveType');
      expect(effectiveTypes).toContain(effectiveType);
    });
  });

  describe('navigator.userAgentData', () => {
    // These tests are done on an http server because navigator.userAgentData
    // requires a secure context.
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

    describe('is not empty', () => {
      it('by default', async () => {
        const w = new BrowserWindow({ show: false });
        await w.loadURL(serverUrl);
        const platform = await w.webContents.executeJavaScript('navigator.userAgentData.platform');
        expect(platform).not.toHaveLength(0);
      });

      it('when there is a session-wide UA override', async () => {
        const ses = session.fromPartition(`${Math.random()}`);
        ses.setUserAgent('foobar');
        const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });
        await w.loadURL(serverUrl);
        const platform = await w.webContents.executeJavaScript('navigator.userAgentData.platform');
        expect(platform).not.toHaveLength(0);
      });

      it('when there is a WebContents-specific UA override', async () => {
        const w = new BrowserWindow({ show: false });
        w.webContents.setUserAgent('foo');
        await w.loadURL(serverUrl);
        const platform = await w.webContents.executeJavaScript('navigator.userAgentData.platform');
        expect(platform).not.toHaveLength(0);
      });

      it('when there is a WebContents-specific UA override at load time', async () => {
        const w = new BrowserWindow({ show: false });
        await w.loadURL(serverUrl, {
          userAgent: 'foo'
        });
        const platform = await w.webContents.executeJavaScript('navigator.userAgentData.platform');
        expect(platform).not.toHaveLength(0);
      });
    });

    describe('brand list', () => {
      it('contains chromium', async () => {
        const w = new BrowserWindow({ show: false });
        await w.loadURL(serverUrl);
        const brands = await w.webContents.executeJavaScript('navigator.userAgentData.brands');
        expect(brands.map((b: any) => b.brand)).toContain('Chromium');
      });
    });
  });

  describe('Badging API', () => {
    it('does not crash', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      await w.webContents.executeJavaScript('navigator.setAppBadge(42)');
      await w.webContents.executeJavaScript('navigator.setAppBadge()');
      await w.webContents.executeJavaScript('navigator.clearAppBadge()');
    });
  });

  describe('navigator.webkitGetUserMedia', () => {
    it('calls its callbacks', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      await w.webContents.executeJavaScript(`new Promise((resolve) => {
        navigator.webkitGetUserMedia({
          audio: true,
          video: false
        }, () => resolve(),
        () => resolve());
      })`);
    });
  });

  describe('navigator.language', () => {
    it('should not be empty', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');
      expect(await w.webContents.executeJavaScript('navigator.language')).not.toBe('');
    });
  });

  describe('heap snapshot', () => {
    it('does not crash', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      w.loadURL('about:blank');
      await w.webContents.executeJavaScript("process._linkedBinding('electron_common_v8_util').takeHeapSnapshot()");
    });
  });

  describe('webgl', () => {
    it('can be gotten as context in canvas', async (ctx) => {
      const w = new BrowserWindow({
        show: false
      });
      w.loadURL('about:blank');
      await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      const isFallbackAdapter = await w.webContents.executeJavaScript(`
        navigator.gpu?.requestAdapter().then(adapter => (adapter?.info?.isFallbackAdapter || !adapter?.info), true)`);

      if (isFallbackAdapter) {
        console.log('Skipping webgl test on fallback adapter');
        ctx.skip();
      } else {
        const canWebglContextBeCreated = await w.webContents.executeJavaScript(`
          document.createElement('canvas').getContext('webgl') != null;
        `);
        expect(canWebglContextBeCreated).toBe(true);
      }
    });
  });

  describe('iframe', () => {
    it('does not have node integration', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      const result = await w.webContents.executeJavaScript(`
        const iframe = document.createElement('iframe')
        iframe.src = './set-global.html';
        document.body.appendChild(iframe);
        new Promise(resolve => iframe.onload = e => resolve(iframe.contentWindow.test))
      `);
      expect(result).toBe('undefined undefined undefined');
    });
  });

  describe('websockets', () => {
    it('has user agent', async () => {
      const server = http.createServer();
      defer(() => server.close());
      const { port } = await listen(server);
      const wss = new WebSocketServer({ server });
      const finished = new Promise<string | undefined>((resolve, reject) => {
        wss.on('error', reject);
        wss.on('connection', (ws, upgradeReq) => {
          resolve(upgradeReq.headers['user-agent']);
        });
      });
      const w = new BrowserWindow({ show: false });
      w.loadURL('about:blank');
      w.webContents.executeJavaScript(`
        new WebSocket('ws://127.0.0.1:${port}');
      `);
      expect(await finished).toContain('Electron');
    });
  });

  describe('fetch', () => {
    it('does not crash', async () => {
      const server = http.createServer((req, res) => {
        res.end('test');
      });
      defer(() => server.close());
      const { port } = await listen(server);
      const w = new BrowserWindow({ show: false });
      w.loadURL(`file://${fixturesPath}/pages/blank.html`);
      const x = await w.webContents.executeJavaScript(`
        fetch('http://127.0.0.1:${port}').then((res) => res.body.getReader())
          .then((reader) => {
            return reader.read().then((r) => {
              reader.cancel();
              return r.value;
            });
          })
      `);
      expect(x).toEqual(new Uint8Array([116, 101, 115, 116]));
    });
  });

  describe('Promise', () => {
    beforeAll(() => {
      ipcMain.handle('ping', (e, arg) => arg);
    });
    afterAll(() => {
      ipcMain.removeHandler('ping');
    });
    itremote('resolves correctly in Node.js calls', async () => {
      await new Promise<void>((resolve, reject) => {
        class XElement extends HTMLElement {}
        customElements.define('x-element', XElement);
        setImmediate(() => {
          let called = false;
          Promise.resolve().then(() => {
            if (called) resolve();
            else reject(new Error('wrong sequence'));
          });
          document.createElement('x-element');
          called = true;
        });
      });
    });

    itremote('resolves correctly in Electron calls', async () => {
      await new Promise<void>((resolve, reject) => {
        class YElement extends HTMLElement {}
        customElements.define('y-element', YElement);
        require('electron')
          .ipcRenderer.invoke('ping')
          .then(() => {
            let called = false;
            Promise.resolve().then(() => {
              if (called) resolve();
              else reject(new Error('wrong sequence'));
            });
            document.createElement('y-element');
            called = true;
          });
      });
    });
  });

  describe('synchronous prompts', () => {
    describe('window.alert(message, title)', () => {
      itremote('throws an exception when the arguments cannot be converted to strings', () => {
        expect(() => {
          window.alert({ toString: null });
        }).toThrow('Cannot convert object to primitive value');
      });

      it('shows a message box', async () => {
        const w = new BrowserWindow({ show: false });
        w.loadURL('about:blank');
        const p = once(w.webContents, '-run-dialog');
        w.webContents.executeJavaScript('alert("hello")', true);
        const [info] = await p;
        expect(info.frame).toBe(w.webContents.mainFrame);
        expect(info.messageText).toBe('hello');
        expect(info.dialogType).toBe('alert');
      });

      it('does not crash if a webContents is destroyed while an alert is showing', async () => {
        const w = new BrowserWindow({ show: false });
        w.loadURL('about:blank');
        const p = once(w.webContents, '-run-dialog');
        w.webContents.executeJavaScript('alert("hello")', true);
        await p;
        w.webContents.close();
      });
    });

    describe('window.confirm(message, title)', () => {
      itremote('throws an exception when the arguments cannot be converted to strings', () => {
        expect(() => {
          (window.confirm as any)({ toString: null }, 'title');
        }).toThrow('Cannot convert object to primitive value');
      });

      it('shows a message box', async () => {
        const w = new BrowserWindow({ show: false });
        w.loadURL('about:blank');
        const p = once(w.webContents, '-run-dialog');
        const resultPromise = w.webContents.executeJavaScript('confirm("hello")', true);
        const [info, cb] = await p;
        expect(info.frame).toBe(w.webContents.mainFrame);
        expect(info.messageText).toBe('hello');
        expect(info.dialogType).toBe('confirm');
        cb(true, '');
        const result = await resultPromise;
        expect(result).toBe(true);
      });
    });

    describe('safeDialogs web preference', () => {
      const originalShowMessageBox = dialog.showMessageBox;
      afterEach(() => {
        dialog.showMessageBox = originalShowMessageBox;
        if (protocol.isProtocolHandled('https')) protocol.unhandle('https');
        if (protocol.isProtocolHandled('file')) protocol.unhandle('file');
      });
      it('does not show the checkbox if not enabled', async () => {
        const w = new BrowserWindow({ show: false, webPreferences: { safeDialogs: false } });
        w.loadURL('about:blank');
        // 1. The first alert() doesn't show the safeDialogs message.
        dialog.showMessageBox = () => Promise.resolve({ response: 0, checkboxChecked: false });
        await w.webContents.executeJavaScript('alert("hi")');

        let recordedOpts: MessageBoxOptions | undefined;
        dialog.showMessageBox = (bw, opts?: MessageBoxOptions) => {
          recordedOpts = opts;
          return Promise.resolve({ response: 0, checkboxChecked: false });
        };
        await w.webContents.executeJavaScript('alert("hi")');
        expect(recordedOpts?.checkboxLabel).toBe('');
      });

      it('is respected', async () => {
        const w = new BrowserWindow({ show: false, webPreferences: { safeDialogs: true } });
        w.loadURL('about:blank');
        // 1. The first alert() doesn't show the safeDialogs message.
        dialog.showMessageBox = () => Promise.resolve({ response: 0, checkboxChecked: false });
        await w.webContents.executeJavaScript('alert("hi")');

        // 2. The second alert() shows the message with a checkbox. Respond that we checked it.
        let recordedOpts: MessageBoxOptions | undefined;
        dialog.showMessageBox = (bw, opts?: MessageBoxOptions) => {
          recordedOpts = opts;
          return Promise.resolve({ response: 0, checkboxChecked: true });
        };
        await w.webContents.executeJavaScript('alert("hi")');
        expect(recordedOpts?.checkboxLabel).toBeTypeOf('string');
        expect(recordedOpts?.checkboxLabel).not.toHaveLength(0);

        // 3. The third alert() shouldn't show a dialog.
        dialog.showMessageBox = () => Promise.reject(new Error('unexpected showMessageBox'));
        await w.webContents.executeJavaScript('alert("hi")');
      });

      it('shows the safeDialogMessage', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: { safeDialogs: true, safeDialogsMessage: 'foo bar' }
        });
        w.loadURL('about:blank');
        dialog.showMessageBox = () => Promise.resolve({ response: 0, checkboxChecked: false });
        await w.webContents.executeJavaScript('alert("hi")');
        let recordedOpts: MessageBoxOptions | undefined;
        dialog.showMessageBox = (bw, opts?: MessageBoxOptions) => {
          recordedOpts = opts;
          return Promise.resolve({ response: 0, checkboxChecked: true });
        };
        await w.webContents.executeJavaScript('alert("hi")');
        expect(recordedOpts?.checkboxLabel).toBe('foo bar');
      });

      it('has persistent state across navigations', async () => {
        const w = new BrowserWindow({ show: false, webPreferences: { safeDialogs: true } });
        w.loadURL('about:blank');
        // 1. The first alert() doesn't show the safeDialogs message.
        dialog.showMessageBox = () => Promise.resolve({ response: 0, checkboxChecked: false });
        await w.webContents.executeJavaScript('alert("hi")');

        // 2. The second alert() shows the message with a checkbox. Respond that we checked it.
        dialog.showMessageBox = () => Promise.resolve({ response: 0, checkboxChecked: true });
        await w.webContents.executeJavaScript('alert("hi")');

        // 3. The third alert() shouldn't show a dialog.
        dialog.showMessageBox = () => Promise.reject(new Error('unexpected showMessageBox'));
        await w.webContents.executeJavaScript('alert("hi")');

        // 4. After navigating to the same origin, message boxes should still be hidden.
        w.loadURL('about:blank');
        await w.webContents.executeJavaScript('alert("hi")');
      });

      it('is separated by origin', async () => {
        protocol.handle('https', () => new Response(''));
        const w = new BrowserWindow({ show: false, webPreferences: { safeDialogs: true } });
        w.loadURL('https://example1');
        dialog.showMessageBox = () => Promise.resolve({ response: 0, checkboxChecked: false });
        await w.webContents.executeJavaScript('alert("hi")');
        dialog.showMessageBox = () => Promise.resolve({ response: 0, checkboxChecked: true });
        await w.webContents.executeJavaScript('alert("hi")');
        dialog.showMessageBox = () => Promise.reject(new Error('unexpected showMessageBox'));
        await w.webContents.executeJavaScript('alert("hi")');

        // A different origin is allowed to show message boxes after navigation.
        w.loadURL('https://example2');
        let dialogWasShown = false;
        dialog.showMessageBox = () => {
          dialogWasShown = true;
          return Promise.resolve({ response: 0, checkboxChecked: false });
        };
        await w.webContents.executeJavaScript('alert("hi")');
        expect(dialogWasShown).toBe(true);

        // Navigating back to the first origin means alerts are blocked again.
        w.loadURL('https://example1');
        dialog.showMessageBox = () => Promise.reject(new Error('unexpected showMessageBox'));
        await w.webContents.executeJavaScript('alert("hi")');
      });

      it('treats different file: paths as different origins', async () => {
        protocol.handle('file', () => new Response(''));
        const w = new BrowserWindow({ show: false, webPreferences: { safeDialogs: true } });
        w.loadURL('file:///path/1');
        dialog.showMessageBox = () => Promise.resolve({ response: 0, checkboxChecked: false });
        await w.webContents.executeJavaScript('alert("hi")');
        dialog.showMessageBox = () => Promise.resolve({ response: 0, checkboxChecked: true });
        await w.webContents.executeJavaScript('alert("hi")');
        dialog.showMessageBox = () => Promise.reject(new Error('unexpected showMessageBox'));
        await w.webContents.executeJavaScript('alert("hi")');

        w.loadURL('file:///path/2');
        let dialogWasShown = false;
        dialog.showMessageBox = () => {
          dialogWasShown = true;
          return Promise.resolve({ response: 0, checkboxChecked: false });
        };
        await w.webContents.executeJavaScript('alert("hi")');
        expect(dialogWasShown).toBe(true);
      });
    });
    describe('disableDialogs web preference', () => {
      const originalShowMessageBox = dialog.showMessageBox;
      afterEach(() => {
        dialog.showMessageBox = originalShowMessageBox;
        if (protocol.isProtocolHandled('https')) protocol.unhandle('https');
      });
      it('is respected', async () => {
        const w = new BrowserWindow({ show: false, webPreferences: { disableDialogs: true } });
        w.loadURL('about:blank');
        dialog.showMessageBox = () => Promise.reject(new Error('unexpected message box'));
        await w.webContents.executeJavaScript('alert("hi")');
      });
    });
  });

  describe('window.history', () => {
    describe('window.history.go(offset)', () => {
      itremote('throws an exception when the argument cannot be converted to a string', () => {
        expect(() => {
          (window.history.go as any)({ toString: null });
        }).toThrow('Cannot convert object to primitive value');
      });
    });
  });

  describe('console functions', () => {
    itremote('should exist', () => {
      expect(console.log, 'log').toBeTypeOf('function');
      expect(console.error, 'error').toBeTypeOf('function');
      expect(console.warn, 'warn').toBeTypeOf('function');
      expect(console.info, 'info').toBeTypeOf('function');
      expect(console.debug, 'debug').toBeTypeOf('function');
      expect(console.trace, 'trace').toBeTypeOf('function');
      expect(console.time, 'time').toBeTypeOf('function');
      expect(console.timeEnd, 'timeEnd').toBeTypeOf('function');
    });
  });

  describe('SpeechRecognition', () => {
    itremote('reports on-device recognition as unavailable without killing the renderer', async () => {
      const options = { langs: ['en-US'], processLocally: true };
      expect(await (window as any).SpeechRecognition.available(options)).toBe('unavailable');
      expect(await (window as any).SpeechRecognition.install(options)).toBe(false);
    });
  });

  // FIXME(nornagon): this is broken on CI, it triggers:
  // [FATAL:speech_synthesis.mojom-shared.h(237)] The outgoing message will
  // trigger VALIDATION_ERROR_UNEXPECTED_NULL_POINTER at the receiving side
  // (null text in SpeechSynthesisUtterance struct).
  describe('SpeechSynthesis', () => {
    itremote('should emit lifecycle events', async () => {
      const sentence = `long sentence which will take at least a few seconds to
          utter so that it's possible to pause and resume before the end`;
      const utter = new SpeechSynthesisUtterance(sentence);
      // Create a dummy utterance so that speech synthesis state
      // is initialized for later calls.
      speechSynthesis.speak(new SpeechSynthesisUtterance());
      speechSynthesis.cancel();
      speechSynthesis.speak(utter);
      // paused state after speak()
      expect(speechSynthesis.paused).toBe(false);
      await new Promise((resolve) => {
        utter.onstart = resolve;
      });
      // paused state after start event
      expect(speechSynthesis.paused).toBe(false);

      speechSynthesis.pause();
      // paused state changes async, right before the pause event
      expect(speechSynthesis.paused).toBe(false);
      await new Promise((resolve) => {
        utter.onpause = resolve;
      });
      expect(speechSynthesis.paused).toBe(true);

      speechSynthesis.resume();
      await new Promise((resolve) => {
        utter.onresume = resolve;
      });
      // paused state after resume event
      expect(speechSynthesis.paused).toBe(false);

      await new Promise((resolve) => {
        utter.onend = resolve;
      });
    });
  });

  describe('devtools', () => {
    it('fetch colors.css', async () => {
      // <link href="devtools://theme/colors.css?sets=ui,chrome" rel="stylesheet">
      const w = new BrowserWindow({ show: false });
      const devtools = new BrowserWindow({ show: false });
      const devToolsOpened = once(w.webContents, 'devtools-opened');

      w.webContents.setDevToolsWebContents(devtools.webContents);
      w.webContents.openDevTools();
      await devToolsOpened;
      expect(devtools.webContents.getURL().startsWith('devtools://devtools')).toBe(true);

      const result = await devtools.webContents.executeJavaScript(`
        document.body.querySelector('link[href*=\\'//theme/colors.css\\']')?.getAttribute('href');
      `);
      expect(result.startsWith('devtools://theme/colors.css?sets=ui,chrome')).toBe(true);
      const colorAccentResult = await devtools.webContents.executeJavaScript(`
        const style = getComputedStyle(document.body);
        style.getPropertyValue('--color-accent');
      `);
      expect(colorAccentResult).not.toBe('');
      const colorAppMenuHighlightSeverityLow = await devtools.webContents.executeJavaScript(`
        style.getPropertyValue('--color-app-menu-highlight-severity-low');
      `);
      expect(colorAppMenuHighlightSeverityLow).not.toBe('');
      const rgb = await devtools.webContents.executeJavaScript(`
        style.getPropertyValue('--color-accent-rgb');
      `);
      expect(rgb).toBe('');
    });
  });

  describe('long-animation-frame', () => {
    it('should include script attribution on custom protocols if AlwaysLogLOAFURL is enabled', async () => {
      const rc = await startRemoteControlApp(['--enable-features=AlwaysLogLOAFURL']);
      const hasAttribution = await rc.remotely(
        async (fixture: string) => {
          const { BrowserWindow, protocol, net } = require('electron/main');
          const { pathToFileURL } = require('node:url');

          protocol.handle('custom', () => net.fetch(pathToFileURL(fixture).toString()));

          // `show: true` is necessary on Windows and Linux due to https://github.com/electron/electron/issues/32001
          const w = new BrowserWindow({ show: true });
          await w.loadURL('custom://my-url');

          const hasAttribution = await w.webContents.executeJavaScript('hasAttributionPromise');

          global.setTimeout(() => require('electron').app.quit());

          return hasAttribution;
        },
        path.join(fixturesPath, 'chromium', 'long-animation-frame.html')
      );
      expect(hasAttribution).toBe(true);
    });
  });
});

ifdescribe(process.platform === 'darwin' && !process.mas)('kill ring', () => {
  afterEach(closeAllWindows);

  it('yanks back text killed with Ctrl+K', async () => {
    const w = new BrowserWindow({ show: true });
    await w.loadURL('data:text/html,<textarea id="t">kill me</textarea>');
    w.webContents.focus();
    await w.webContents.executeJavaScript(
      'const t = document.getElementById("t"); t.focus(); t.setSelectionRange(0, 0); null'
    );
    w.webContents.debugger.attach();
    const press = async (key: string, code: string, keyCode: number, commands: string[]) => {
      const base = { key, code, windowsVirtualKeyCode: keyCode, modifiers: 2 };
      await w.webContents.debugger.sendCommand('Input.dispatchKeyEvent', { ...base, type: 'rawKeyDown', commands });
      await w.webContents.debugger.sendCommand('Input.dispatchKeyEvent', { ...base, type: 'keyUp' });
    };
    await press('k', 'KeyK', 75, ['deleteToEndOfParagraph']);
    expect(await w.webContents.executeJavaScript('t.value')).toBe('');
    await press('y', 'KeyY', 89, ['yank']);
    expect(await w.webContents.executeJavaScript('t.value')).toBe('kill me');
  });
});

describe('font fallback', () => {
  async function getRenderedFonts(html: string) {
    const w = new BrowserWindow({ show: false });
    try {
      await w.loadURL(`data:text/html,${html}`);
      w.webContents.debugger.attach();
      const sendCommand = (method: string, commandParams?: any) =>
        w.webContents.debugger.sendCommand(method, commandParams);
      const { nodeId } = (await sendCommand('DOM.getDocument')).root.children[0];
      await sendCommand('CSS.enable');
      const { fonts } = await sendCommand('CSS.getPlatformFontsForNode', { nodeId });
      return fonts;
    } finally {
      w.close();
    }
  }

  it('should use Helvetica for sans-serif on Mac, and Arial on Windows and Linux', async () => {
    const html = '<body style="font-family: sans-serif">test</body>';
    const fonts = await getRenderedFonts(html);
    expect(fonts).toBeInstanceOf(Array);
    expect(fonts).toHaveLength(1);
    if (process.platform === 'win32') {
      expect(fonts[0].familyName).toBe('Arial');
    } else if (process.platform === 'darwin') {
      expect(fonts[0].familyName).toBe('Helvetica');
    } else if (process.platform === 'linux') {
      expect(fonts[0].familyName).toBe('DejaVu Sans');
    } // I think this depends on the distro? We don't specify a default.
  });

  ifit(process.platform !== 'linux')('should fall back to Japanese font for sans-serif Japanese script', async () => {
    const html = `
    <html lang="ja-JP">
      <head>
        <meta charset="utf-8" />
      </head>
      <body style="font-family: sans-serif">test 智史</body>
    </html>
    `;
    const fonts = await getRenderedFonts(html);
    expect(fonts).toBeInstanceOf(Array);
    expect(fonts).toHaveLength(1);
    if (process.platform === 'win32') {
      expect(['Meiryo', 'Yu Gothic']).toContain(fonts[0].familyName);
    } else if (process.platform === 'darwin') {
      expect(fonts[0].familyName).toBe('Hiragino Kaku Gothic ProN');
    }
  });
});

describe('iframe using HTML fullscreen API while window is OS-fullscreened', { tags: ['serial'] }, () => {
  const fullscreenChildHtml = fs.promises.readFile(path.join(fixturesPath, 'pages', 'fullscreen-oopif.html'));
  let w: BrowserWindow;
  let server: http.Server;
  let crossSiteUrl: string;

  beforeEach(async () => {
    server = http.createServer(async (_req, res) => {
      res.writeHead(200, { 'Content-Type': 'text/html' });
      res.write(await fullscreenChildHtml);
      res.end();
    });

    const serverUrl = (await listen(server)).url;
    crossSiteUrl = serverUrl.replace('127.0.0.1', 'localhost');

    w = new BrowserWindow({
      show: true,
      fullscreen: true,
      webPreferences: {
        nodeIntegration: true,
        nodeIntegrationInSubFrames: true,
        contextIsolation: false
      }
    });
  });

  afterEach(async () => {
    await closeAllWindows();
    (w as any) = null;
    server.close();
  });

  ifit(process.platform !== 'darwin')('can fullscreen from out-of-process iframes (non-macOS)', async () => {
    const fullscreenChange = once(ipcMain, 'fullscreenChange');
    const html = `<iframe style="width: 0" frameborder=0 src="${crossSiteUrl}" allowfullscreen></iframe>`;
    w.loadURL(`data:text/html,${html}`);
    await fullscreenChange;

    const fullscreenWidth = await w.webContents.executeJavaScript("document.querySelector('iframe').offsetWidth");
    expect(fullscreenWidth > 0).toBe(true);

    await w.webContents.executeJavaScript(
      "document.querySelector('iframe').contentWindow.postMessage('exitFullscreen', '*')"
    );

    await waitUntil(async () => {
      const width = await w.webContents.executeJavaScript("document.querySelector('iframe').offsetWidth");
      return width === 0;
    });
  });

  ifit(process.platform === 'darwin')('can fullscreen from out-of-process iframes (macOS)', async () => {
    await once(w, 'enter-full-screen');
    const fullscreenChange = once(ipcMain, 'fullscreenChange');
    const html = `<iframe style="width: 0" frameborder=0 src="${crossSiteUrl}" allowfullscreen></iframe>`;
    w.loadURL(`data:text/html,${html}`);
    await fullscreenChange;

    const fullscreenWidth = await w.webContents.executeJavaScript("document.querySelector('iframe').offsetWidth");
    expect(fullscreenWidth > 0).toBe(true);

    await w.webContents.executeJavaScript(
      "document.querySelector('iframe').contentWindow.postMessage('exitFullscreen', '*')"
    );
    await once(w.webContents, 'leave-html-full-screen');

    await waitUntil(async () => {
      const width = await w.webContents.executeJavaScript("document.querySelector('iframe').offsetWidth");
      return width === 0;
    });

    w.setFullScreen(false);
    await once(w, 'leave-full-screen');
  });

  it('can fullscreen from in-process iframes', async () => {
    if (process.platform === 'darwin') await once(w, 'enter-full-screen');

    await w.loadFile(path.join(fixturesPath, 'pages', 'fullscreen-ipif.html'));
    await w.webContents.executeJavaScript(
      "document.querySelector('iframe').contentDocument.querySelector('video').requestFullscreen()",
      true
    );

    const fullscreenWidth = await w.webContents.executeJavaScript("document.querySelector('iframe').offsetWidth");
    expect(fullscreenWidth > 0).toBe(true);

    await w.webContents.executeJavaScript('document.exitFullscreen()');
    const width = await w.webContents.executeJavaScript("document.querySelector('iframe').offsetWidth");
    expect(width).toBe(0);
  });

  it('emits fullscreenchange on the parent document for in-process iframes', async () => {
    if (process.platform === 'darwin') await once(w, 'enter-full-screen');

    w.webContents.setBackgroundThrottling(false);
    await w.loadFile(path.join(fixturesPath, 'pages', 'fullscreen-ipif.html'));
    const fullscreenElementIsIframe = await w.webContents.executeJavaScript(
      `(async () => {
        const iframe = document.querySelector('iframe');
        const fullscreenChange = new Promise(resolve => {
          document.addEventListener('fullscreenchange', () => {
            resolve(document.fullscreenElement === iframe);
          }, { once: true });
        });
        const [isFullscreen] = await Promise.all([
          fullscreenChange,
          iframe.contentDocument.querySelector('video').requestFullscreen()
        ]);
        return isFullscreen;
      })()`,
      true
    );
    expect(fullscreenElementIsIframe, 'parent document fullscreenElement is the iframe').toBe(true);

    await w.webContents.executeJavaScript('document.exitFullscreen()');
  });
});

describe('navigator.serial', () => {
  let w: BrowserWindow;
  beforeAll(async () => {
    w = new BrowserWindow({
      show: false
    });
    await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
  });

  const getPorts: any = () => {
    return w.webContents.executeJavaScript(
      `
      navigator.serial.requestPort().then(port => port.toString()).catch(err => err.toString());
    `,
      true
    );
  };

  const notFoundError = "NotFoundError: Failed to execute 'requestPort' on 'Serial': No port selected by the user.";

  afterAll(closeAllWindows);
  afterEach(() => {
    session.defaultSession.setPermissionCheckHandler(null);
    session.defaultSession.removeAllListeners('select-serial-port');
  });

  it('does not return a port if select-serial-port event is not defined', async () => {
    // Take screenshot to verify the test is running
    w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
    const port = await getPorts();
    expect(port).toBe(notFoundError);
  });

  it('does not return a port when permission denied', async () => {
    w.webContents.session.on('select-serial-port', (event, portList, webContents, callback) => {
      callback(portList[0].portId);
    });
    session.defaultSession.setPermissionCheckHandler(() => false);
    const port = await getPorts();
    expect(port).toBe(notFoundError);
  });

  it('does not crash when select-serial-port is called with an invalid port', async () => {
    w.webContents.session.on('select-serial-port', (event, portList, webContents, callback) => {
      callback('i-do-not-exist');
    });
    const port = await getPorts();
    expect(port).toBe(notFoundError);
  });

  it('returns a port when select-serial-port event is defined', async () => {
    let havePorts = false;
    w.webContents.session.on('select-serial-port', (event, portList, webContents, callback) => {
      if (portList.length > 0) {
        havePorts = true;
        callback(portList[0].portId);
      } else {
        callback('');
      }
    });
    const port = await getPorts();
    if (havePorts) {
      expect(port).toBe('[object SerialPort]');
    } else {
      expect(port).toBe(notFoundError);
    }
  });

  it('navigator.serial.getPorts() returns values', async () => {
    let havePorts = false;

    w.webContents.session.on('select-serial-port', (event, portList, webContents, callback) => {
      if (portList.length > 0) {
        havePorts = true;
        callback(portList[0].portId);
      } else {
        callback('');
      }
    });
    await getPorts();
    if (havePorts) {
      const grantedPorts = await w.webContents.executeJavaScript('navigator.serial.getPorts()');
      expect(grantedPorts).not.toHaveLength(0);
    }
  });

  it('supports port.forget()', async () => {
    let forgottenPortFromEvent = {};
    let havePorts = false;

    w.webContents.session.on('select-serial-port', (event, portList, webContents, callback) => {
      if (portList.length > 0) {
        havePorts = true;
        callback(portList[0].portId);
      } else {
        callback('');
      }
    });

    w.webContents.session.on('serial-port-revoked', (event, details) => {
      forgottenPortFromEvent = details.port;
    });

    await getPorts();
    if (havePorts) {
      const grantedPorts = await w.webContents.executeJavaScript('navigator.serial.getPorts()');
      if (grantedPorts.length > 0) {
        const forgottenPort = await w.webContents.executeJavaScript(`
          navigator.serial.getPorts().then(async(ports) => {
            const portInfo = await ports[0].getInfo();
            await ports[0].forget();
            if (portInfo.usbVendorId && portInfo.usbProductId) {
              return {
                vendorId: '' + portInfo.usbVendorId,
                productId: '' + portInfo.usbProductId
              }
            } else {
              return {};
            }
          })
        `);
        const grantedPorts2 = await w.webContents.executeJavaScript('navigator.serial.getPorts()');
        expect(grantedPorts2.length).toBeLessThan(grantedPorts.length);
        if (forgottenPort.vendorId && forgottenPort.productId) {
          expect(forgottenPortFromEvent).toMatchObject(forgottenPort);
        }
      }
    }
  });
});

describe('window.getScreenDetails', () => {
  let w: BrowserWindow;
  beforeAll(async () => {
    w = new BrowserWindow({
      show: false
    });
    await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
  });

  afterAll(closeAllWindows);
  afterEach(() => {
    session.defaultSession.setPermissionRequestHandler(null);
  });

  const getScreenDetails: any = () => {
    return w.webContents.executeJavaScript(
      'window.getScreenDetails().then(data => data.screens).catch(err => err.message)',
      true
    );
  };

  it('returns screens when a PermissionRequestHandler is not defined', async () => {
    const screens = await getScreenDetails();
    expect(screens).not.toBe('Read permission denied.');
  });

  it('returns an error when permission denied', async () => {
    session.defaultSession.setPermissionRequestHandler((wc, permission, callback) => {
      if (permission === 'window-management') {
        callback(false);
      } else {
        callback(true);
      }
    });
    const screens = await getScreenDetails();
    expect(screens).toBe('Permission denied.');
  });

  it('returns screens when permission is granted', async () => {
    session.defaultSession.setPermissionRequestHandler((wc, permission, callback) => {
      if (permission === 'window-management') {
        callback(true);
      } else {
        callback(false);
      }
    });
    const screens = await getScreenDetails();
    expect(screens).not.toBe('Permission denied.');
  });
});

describe('navigator.clipboard.read', { tags: ['serial'] }, () => {
  let w: BrowserWindow;
  beforeAll(async () => {
    w = new BrowserWindow();
    await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
  });

  // Reads back known text, so that a read which fails for any reason
  // (including "Document is not focused.") can't pass for a successful one.
  const clipboardText = 'navigator.clipboard.read';
  const readClipboard = async () => {
    clipboard.writeText(clipboardText);
    await focusWebContents(w.webContents);
    return w.webContents.executeJavaScript(
      `
      navigator.clipboard.read()
        .then(async ([item]) => (await item.getType('text/plain')).text())
        .catch(err => err.message);
    `,
      true
    );
  };

  afterAll(closeAllWindows);
  afterEach(() => {
    session.defaultSession.setPermissionRequestHandler(null);
  });

  it('returns clipboard contents when a PermissionRequestHandler is not defined', async () => {
    expect(await readClipboard()).toBe(clipboardText);
  });

  it('returns an error when permission denied', async () => {
    session.defaultSession.setPermissionRequestHandler((wc, permission, callback) => {
      callback(permission !== 'clipboard-read');
    });
    const clipboard = await readClipboard();
    expect(clipboard).toContain('Read permission denied.');
  });

  it('returns clipboard contents when permission is granted', async () => {
    session.defaultSession.setPermissionRequestHandler((wc, permission, callback) => {
      callback(permission === 'clipboard-read');
    });
    expect(await readClipboard()).toBe(clipboardText);
  });
});

describe('navigator.clipboard.write', { tags: ['serial'] }, () => {
  let w: BrowserWindow;
  beforeAll(async () => {
    w = new BrowserWindow();
    await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
  });

  const writeClipboard = async () => {
    await focusWebContents(w.webContents);
    return w.webContents.executeJavaScript(
      `
      navigator.clipboard.writeText('Hello World!').catch(err => err.message);
    `,
      true
    );
  };

  afterAll(closeAllWindows);
  afterEach(() => {
    session.defaultSession.setPermissionRequestHandler(null);
  });

  it('returns clipboard contents when a PermissionRequestHandler is not defined', async () => {
    const clipboard = await writeClipboard();
    expect(clipboard).toBeUndefined();
  });

  it('returns an error when permission denied', async () => {
    session.defaultSession.setPermissionRequestHandler((wc, permission, callback) => {
      if (permission === 'clipboard-sanitized-write') {
        callback(false);
      } else {
        callback(true);
      }
    });
    const clipboard = await writeClipboard();
    expect(clipboard).toContain('Write permission denied.');
  });

  it('returns clipboard contents when permission is granted', async () => {
    session.defaultSession.setPermissionRequestHandler((wc, permission, callback) => {
      if (permission === 'clipboard-sanitized-write') {
        callback(true);
      } else {
        callback(false);
      }
    });
    const clipboard = await writeClipboard();
    expect(clipboard).toBeUndefined();
  });
});

describe('pointer lock permission request', { tags: ['serial'] }, () => {
  const servers: http.Server[] = [];
  let serverUrl: string;
  let otherPortUrl: string;
  beforeAll(async () => {
    for (let i = 0; i < 2; i++) {
      servers.push(
        http.createServer((_req, res) => {
          res.setHeader('content-type', 'text/html');
          res.end('<!doctype html><body>frame</body>');
        })
      );
    }
    [serverUrl, otherPortUrl] = await Promise.all(servers.map(async (s) => (await listen(s)).url));
  });
  afterAll(() => servers.forEach((s) => s.close()));
  afterEach(closeAllWindows);

  const requestFromChildFrame = async (topUrl: string, childUrl: string) => {
    const ses = session.fromPartition(`pointer-lock-${Math.random()}`);
    const w = new BrowserWindow({ show: true, webPreferences: { session: ses } });
    await w.loadURL(topUrl);
    await w.webContents.executeJavaScript(`new Promise((resolve) => {
      const f = document.createElement('iframe');
      f.src = ${JSON.stringify(childUrl)};
      f.onload = resolve;
      document.body.appendChild(f);
    })`);
    const iframe = w.webContents.mainFrame.frames[0];
    const requests: { wc: Electron.WebContents; permission: string; details: any }[] = [];
    ses.setPermissionRequestHandler((wc, permission, callback, details) => {
      requests.push({ wc, permission, details });
      callback(false);
    });
    // content rejects the request with WrongDocumentError, without consulting
    // the permission handler, unless the widget has focus. Focus arrives
    // asynchronously on macOS, so wait for it.
    await focusWebContents(w.webContents);
    const result = await iframe.executeJavaScript(
      "document.body.requestPointerLock().then(() => 'locked', (e) => e.name)",
      true
    );
    // A request denied by the permission handler rejects with SecurityError.
    expect(result).toBe('SecurityError');
    const request = requests.find((r) => r.permission === 'pointerLock');
    expect(request).toEqual(expect.anything());
    expect(request!.wc).toBe(w.webContents);
    return { iframe, details: request!.details };
  };

  it('is attributed to a cross-site frame that called requestPointerLock()', async () => {
    const topUrl = url.pathToFileURL(path.join(fixturesPath, 'pages', 'blank.html')).href;
    const { iframe, details } = await requestFromChildFrame(topUrl, serverUrl);
    expect(iframe.processId).not.toBe(iframe.top!.processId);
    expect(details.requestingUrl).toBe(`${serverUrl}/`);
    expect(details.isMainFrame).toBe(false);
  });

  it('is attributed to a same-site cross-origin frame that called requestPointerLock()', async () => {
    // Same host, different port: cross-origin but same-site, so the child
    // shares the top frame's process and widget.
    const topUrl = `${serverUrl}/top`;
    const childUrl = `${otherPortUrl}/child`;
    const { iframe, details } = await requestFromChildFrame(topUrl, childUrl);
    expect(iframe.processId).toBe(iframe.top!.processId);
    expect(details.requestingUrl).toBe(childUrl);
    expect(details.isMainFrame).toBe(false);
  });
});

describe('paste execCommand', { tags: ['serial'] }, () => {
  const readClipboard = async (w: BrowserWindow) => {
    await focusWebContents(w.webContents);

    // No user gesture: these tests exercise the permission path, and a
    // gesture on the requesting frame allows paste by itself.
    const { focused, text } = await w.webContents.executeJavaScript(
      `
      new Promise((resolve) => {
        const focused = document.hasFocus();
        const timeout = setTimeout(() => {
          resolve({ focused, text: '' });
        }, 2000);
        document.addEventListener('paste', (event) => {
          clearTimeout(timeout);
          event.preventDefault();
          let paste = event.clipboardData.getData("text");
          resolve({ focused, text: paste });
        });
        document.execCommand('paste');
      });
    `,
      false
    );
    // A page without focus pastes nothing either,
    // which would let the tests that expect no paste pass for the wrong reason.
    expect(focused, 'document had focus when pasting').toBe(true);
    return text;
  };

  let ses: Electron.Session;
  beforeEach(() => {
    ses = session.fromPartition(`paste-execCommand-${Math.random()}`);
  });

  afterEach(async () => {
    ses.setPermissionCheckHandler(null);
    await closeAllWindows();
  });

  it('is disabled by default', async () => {
    const w: BrowserWindow = new BrowserWindow({});
    await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
    const text = 'Sync Clipboard Disabled by default';
    await clipboard.writeText(text);
    const paste = await readClipboard(w);
    expect(paste).toHaveLength(0);
    expect(await clipboard.readText()).toBe(text);
  });

  it('does not execute with default permissions', async () => {
    const w: BrowserWindow = new BrowserWindow({
      webPreferences: {
        enableDeprecatedPaste: true,
        session: ses
      }
    });
    await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
    const text = 'Sync Clipboard Disabled by default permissions';
    await clipboard.writeText(text);
    const paste = await readClipboard(w);
    expect(paste).toHaveLength(0);
    expect(await clipboard.readText()).toBe(text);
  });

  it('does not execute with permission denied', async () => {
    const w: BrowserWindow = new BrowserWindow({
      webPreferences: {
        enableDeprecatedPaste: true,
        session: ses
      }
    });
    await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
    ses.setPermissionCheckHandler((webContents, permission) => {
      if (permission === 'deprecated-sync-clipboard-read') {
        return false;
      }
      return true;
    });
    const text = 'Sync Clipboard Disabled by permission denied';
    await clipboard.writeText(text);
    const paste = await readClipboard(w);
    expect(paste).toHaveLength(0);
    expect(await clipboard.readText()).toBe(text);
  });

  it('can trigger paste event when permission is granted', async () => {
    const w: BrowserWindow = new BrowserWindow({
      webPreferences: {
        enableDeprecatedPaste: true,
        session: ses
      }
    });
    await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
    ses.setPermissionCheckHandler((webContents, permission) => {
      if (permission === 'deprecated-sync-clipboard-read') {
        return true;
      }
      return false;
    });
    const text = 'Sync Clipboard Test';
    await clipboard.writeText(text);
    const paste = await readClipboard(w);
    expect(paste).toBe(text);
  });

  it('can trigger paste event when permission is granted for child windows', async () => {
    const w: BrowserWindow = new BrowserWindow({
      webPreferences: {
        session: ses
      }
    });
    await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
    w.webContents.setWindowOpenHandler((details) => {
      if (details.url === 'about:blank') {
        return {
          action: 'allow',
          overrideBrowserWindowOptions: {
            webPreferences: {
              enableDeprecatedPaste: true,
              session: ses
            }
          }
        };
      } else {
        return {
          action: 'deny'
        };
      }
    });
    ses.setPermissionCheckHandler((webContents, permission, requestingOrigin, details) => {
      if (
        requestingOrigin === `${webContents?.opener?.origin}/` &&
        details.requestingUrl === 'about:blank' &&
        permission === 'deprecated-sync-clipboard-read'
      ) {
        return true;
      }
      return false;
    });
    const childPromise = once(w.webContents, 'did-create-window') as Promise<
      [BrowserWindow, Electron.DidCreateWindowDetails]
    >;
    w.webContents.executeJavaScript('window.open("about:blank")', true);
    const [childWindow] = await childPromise;
    expect(childWindow.webContents.opener).toBe(w.webContents.mainFrame);
    const text = 'Sync Clipboard Test for Child Window';
    await clipboard.writeText(text);
    const paste = await readClipboard(childWindow);
    expect(paste).toBe(text);
  });

  describe('user activation', () => {
    // A cross-origin iframe next to a main frame that the user just clicked in.
    let server: http.Server;
    let crossOriginUrl: string;
    beforeAll(async () => {
      server = http.createServer((_req, res) => {
        res.setHeader('content-type', 'text/html');
        res.end('<!doctype html><body contenteditable>frame</body>');
      });
      crossOriginUrl = (await listen(server)).url;
    });
    afterAll(() => server.close());

    const pasteIn = (frame: Electron.WebFrameMain) =>
      frame.executeJavaScript(
        `new Promise((resolve) => {
          const timeout = setTimeout(() => resolve(''), 1000);
          document.addEventListener('paste', (event) => {
            clearTimeout(timeout);
            event.preventDefault();
            resolve(event.clipboardData.getData('text'));
          }, { once: true });
          document.execCommand('paste');
        })`,
        false
      );

    const clickMainFrame = async (w: BrowserWindow) => {
      await focusWebContents(w.webContents);
      w.webContents.sendInputEvent({ type: 'mouseDown', x: 5, y: 5, button: 'left', clickCount: 1 });
      w.webContents.sendInputEvent({ type: 'mouseUp', x: 5, y: 5, button: 'left', clickCount: 1 });
      await waitUntil(
        async () => (await w.webContents.mainFrame.executeJavaScript('navigator.userActivation.isActive')) === true
      );
    };

    it('lets the frame the user interacted with paste without a permission grant', async () => {
      const w = new BrowserWindow({ show: true, webPreferences: { enableDeprecatedPaste: true, session: ses } });
      await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
      const text = 'paste with activation';
      clipboard.writeText(text);
      await clickMainFrame(w);
      expect(await pasteIn(w.webContents.mainFrame)).toBe(text);
    });

    it('does not extend a click in the main frame to a cross-origin iframe', async () => {
      const w = new BrowserWindow({ show: true, webPreferences: { enableDeprecatedPaste: true, session: ses } });
      await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
      await w.webContents.executeJavaScript(`new Promise((resolve) => {
        const f = document.createElement('iframe');
        f.style.cssText = 'position:absolute;top:200px;left:0;width:200px;height:100px';
        f.src = ${JSON.stringify(crossOriginUrl)};
        f.onload = resolve;
        document.body.appendChild(f);
      })`);
      const iframe = w.webContents.mainFrame.frames[0];
      const checks: string[] = [];
      ses.setPermissionCheckHandler((_wc, permission, requestingOrigin) => {
        if (permission === 'deprecated-sync-clipboard-read') checks.push(requestingOrigin);
        return false;
      });
      const text = 'main frame click, iframe paste';
      clipboard.writeText(text);
      await clickMainFrame(w);
      expect(await pasteIn(iframe)).toBe('');
      // The iframe had no activation of its own, so the decision went to the
      // check handler, attributed to the iframe.
      expect(checks).not.toHaveLength(0);
      for (const origin of checks) expect(origin).toBe(`${crossOriginUrl}/`);
      expect(await clipboard.readText()).toBe(text);
    });
  });
});

ifdescribe(process.platform !== 'linux')('navigator.setAppBadge/clearAppBadge', () => {
  let w: BrowserWindow;

  const expectedBadgeCount = 42;

  const fireAppBadgeAction: any = (action: string, value: any) => {
    return w.webContents.executeJavaScript(`
      navigator.${action}AppBadge(${value}).then(() => 'success').catch(err => err.message)`);
  };

  // For some reason on macOS changing the badge count doesn't happen right away, so wait
  // until it changes.
  async function waitForBadgeCount(value: number) {
    let badgeCount = app.getBadgeCount();
    while (badgeCount !== value) {
      await setTimeout(10);
      badgeCount = app.getBadgeCount();
    }
    return badgeCount;
  }

  describe('in the renderer', () => {
    beforeAll(async () => {
      w = new BrowserWindow({
        show: false
      });
      await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
    });

    afterAll(async () => {
      app.badgeCount = 0;
      await closeAllWindows();
    });

    it('setAppBadge can set a numerical value', async () => {
      const result = await fireAppBadgeAction('set', expectedBadgeCount);
      expect(result).toBe('success');
      await expect(waitForBadgeCount(expectedBadgeCount)).resolves.toBe(expectedBadgeCount);
    });

    it('setAppBadge can set an empty(dot) value', async () => {
      const result = await fireAppBadgeAction('set');
      expect(result).toBe('success');
      await expect(waitForBadgeCount(0)).resolves.toBe(0);
    });

    it('clearAppBadge can clear a value', async () => {
      let result = await fireAppBadgeAction('set', expectedBadgeCount);
      expect(result).toBe('success');
      await expect(waitForBadgeCount(expectedBadgeCount)).resolves.toBe(expectedBadgeCount);
      result = await fireAppBadgeAction('clear');
      expect(result).toBe('success');
      await expect(waitForBadgeCount(0)).resolves.toBe(0);
    });
  });

  describe('in a service worker', () => {
    beforeEach(async () => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          partition: 'sw-file-scheme-spec',
          contextIsolation: false
        }
      });
    });

    afterEach(async () => {
      app.badgeCount = 0;
      await closeAllWindows();
    });

    it('setAppBadge can be called in a ServiceWorker', () =>
      new Promise<void>((resolve, reject) => {
        const done = (error?: unknown) => (error ? reject(error) : resolve());
        w.webContents.on('ipc-message', (event, channel, message) => {
          if (channel === 'reload') {
            w.webContents.reload();
          } else if (channel === 'error') {
            done(message);
          } else if (channel === 'response') {
            // The worker sets and then clears the badge before it answers, so
            // there is no count to wait for here.
            expect(message).toBe('SUCCESS setting app badge');
            session
              .fromPartition('sw-file-scheme-spec')
              .clearStorageData({ storages: ['serviceworkers'] })
              .then(() => done(), done);
          }
        });
        w.webContents.on('render-process-gone', () => done(new Error('WebContents crashed.')));
        w.loadFile(path.join(fixturesPath, 'pages', 'service-worker', 'badge-index.html'), { search: '?setBadge' });
      }));

    it('clearAppBadge can be called in a ServiceWorker', () =>
      new Promise<void>((resolve, reject) => {
        const done = (error?: unknown) => (error ? reject(error) : resolve());
        w.webContents.on('ipc-message', (event, channel, message) => {
          if (channel === 'reload') {
            w.webContents.reload();
          } else if (channel === 'setAppBadge') {
            expect(message).toBe('SUCCESS setting app badge');
          } else if (channel === 'error') {
            done(message);
          } else if (channel === 'response') {
            expect(message).toBe('SUCCESS clearing app badge');
            waitForBadgeCount(0)
              .then(() =>
                session.fromPartition('sw-file-scheme-spec').clearStorageData({ storages: ['serviceworkers'] })
              )
              .then(() => done(), done);
          }
        });
        w.webContents.on('render-process-gone', () => done(new Error('WebContents crashed.')));
        w.loadFile(path.join(fixturesPath, 'pages', 'service-worker', 'badge-index.html'), { search: '?clearBadge' });
      }));
  });
});

describe('navigator.bluetooth', () => {
  let w: BrowserWindow;
  beforeAll(async () => {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        enableBlinkFeatures: 'WebBluetooth'
      }
    });
    await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
  });

  afterAll(closeAllWindows);

  it('can request bluetooth devices', async () => {
    const bluetooth = await w.webContents.executeJavaScript(
      `
    navigator.bluetooth.requestDevice({ acceptAllDevices: true }).then(device => "Found a device!").catch(err => err.message);`,
      true
    );
    const requestResponses = [
      'Found a device!',
      'Bluetooth adapter not available.',
      'User cancelled the requestDevice() chooser.',
      'User denied the browser permission to scan for Bluetooth devices.'
    ];
    expect(requestResponses, `Unexpected response: ${bluetooth}`).toContain(bluetooth);
  });
});

describe('navigator.hid', () => {
  let w: BrowserWindow;
  let server: http.Server;
  let serverUrl: string;
  beforeAll(async () => {
    w = new BrowserWindow({
      show: false
    });
    await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
    server = http.createServer((req, res) => {
      res.setHeader('Content-Type', 'text/html');
      res.end('<body>');
    });
    serverUrl = (await listen(server)).url;
  });

  const requestDevices: any = () => {
    return w.webContents.executeJavaScript(
      `
      navigator.hid.requestDevice({filters: []}).then(device => device.toString()).catch(err => err.toString());
    `,
      true
    );
  };

  // A device is only reliably grantable if it exposes both a name and a serial
  // number. Devices lacking them (e.g. system devices whose reports are all
  // protected) are dropped by Chromium's WebHID report filtering after
  // selection, so requestDevice() would resolve empty for them and make the
  // test flaky. Select such a valid device rather than blindly picking the
  // first entry in the list.
  const findValidDevice = (deviceList: Electron.HIDDevice[]) =>
    deviceList.find((device) => device.name && device.name !== '' && device.serialNumber && device.serialNumber !== '');

  afterAll(async () => {
    server.close();
    await closeAllWindows();
  });

  afterEach(() => {
    session.defaultSession.setPermissionCheckHandler(null);
    session.defaultSession.setDevicePermissionHandler(null);
    session.defaultSession.removeAllListeners('select-hid-device');
  });

  it('does not return a device if select-hid-device event is not defined', async () => {
    w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
    const device = await requestDevices();
    expect(device).toBe('');
  });

  it('does not return a device when permission denied', async () => {
    let selectFired = false;
    w.webContents.session.on('select-hid-device', (event, details, callback) => {
      selectFired = true;
      callback();
    });
    session.defaultSession.setPermissionCheckHandler(() => false);
    const device = await requestDevices();
    expect(selectFired).toBe(false);
    expect(device).toBe('');
  });

  it('returns a device when select-hid-device event is defined', async () => {
    let haveDevices = false;
    let selectFired = false;
    w.webContents.session.on('select-hid-device', (event, details, callback) => {
      expect(details.frame).toHaveProperty('frameTreeNodeId', expect.any(Number));
      selectFired = true;
      const foundDevice = findValidDevice(details.deviceList);
      if (foundDevice) {
        haveDevices = true;
        callback(foundDevice.deviceId);
      } else {
        callback();
      }
    });
    const device = await requestDevices();
    expect(selectFired).toBe(true);
    if (haveDevices) {
      expect(device).toContain('[object HIDDevice]');
    } else {
      expect(device).toBe('');
    }
    if (haveDevices) {
      // Verify that navigation will clear device permissions
      const grantedDevices = await w.webContents.executeJavaScript('navigator.hid.getDevices()');
      expect(grantedDevices).not.toHaveLength(0);
      w.loadURL(serverUrl);
      const [, , , , , frameProcessId, frameRoutingId] = await once(w.webContents, 'did-frame-navigate');
      const frame = webFrameMain.fromId(frameProcessId, frameRoutingId);
      expect(!!frame).toBe(true);
      if (frame) {
        const grantedDevicesOnNewPage = await frame.executeJavaScript('navigator.hid.getDevices()');
        expect(grantedDevicesOnNewPage).toHaveLength(0);
      }
    }
  });

  it('returns a device when DevicePermissionHandler is defined', async () => {
    let haveDevices = false;
    let selectFired = false;
    let gotDevicePerms = false;
    w.webContents.session.on('select-hid-device', (event, details, callback) => {
      selectFired = true;
      const foundDevice = findValidDevice(details.deviceList);
      if (foundDevice) {
        haveDevices = true;
        callback(foundDevice.deviceId);
        return;
      }
      callback();
    });
    session.defaultSession.setDevicePermissionHandler(() => {
      gotDevicePerms = true;
      return true;
    });
    await w.webContents.executeJavaScript('navigator.hid.getDevices();', true);
    const device = await requestDevices();
    expect(selectFired).toBe(true);
    if (haveDevices) {
      expect(device).toContain('[object HIDDevice]');
      expect(gotDevicePerms).toBe(true);
    } else {
      expect(device).toBe('');
    }
  });

  it('excludes a device when a exclusionFilter is specified', async () => {
    const exclusionFilters: any[] = [];
    let haveDevices = false;
    let checkForExcludedDevice = false;

    w.webContents.session.on('select-hid-device', (event, details, callback) => {
      if (details.deviceList.length > 0) {
        details.deviceList.find((device) => {
          if (device.name && device.name !== '' && device.serialNumber && device.serialNumber !== '') {
            if (checkForExcludedDevice) {
              const compareDevice = {
                vendorId: device.vendorId,
                productId: device.productId
              };
              expect(compareDevice, 'excluded device should not be returned').not.toBe(exclusionFilters[0]);
            } else {
              haveDevices = true;
              exclusionFilters.push({
                vendorId: device.vendorId,
                productId: device.productId
              });
              return true;
            }
          }
          return false;
        });
      }
      callback();
    });

    await requestDevices();
    if (haveDevices) {
      // We have devices to exclude, so check if exclusionFilters work
      checkForExcludedDevice = true;
      await w.webContents.executeJavaScript(
        `
        navigator.hid.requestDevice({filters: [], exclusionFilters: ${JSON.stringify(exclusionFilters)}}).then(device => device.toString()).catch(err => err.toString());

      `,
        true
      );
    }
  });

  it('supports device.forget()', async () => {
    let deletedDeviceFromEvent;
    let haveDevices = false;
    w.webContents.session.on('select-hid-device', (event, details, callback) => {
      if (details.deviceList.length > 0) {
        haveDevices = true;
        callback(details.deviceList[0].deviceId);
      } else {
        callback();
      }
    });
    w.webContents.session.on('hid-device-revoked', (event, details) => {
      deletedDeviceFromEvent = details.device;
    });
    await requestDevices();
    if (haveDevices) {
      const grantedDevices = await w.webContents.executeJavaScript('navigator.hid.getDevices()');
      if (grantedDevices.length > 0) {
        const deletedDevice = await w.webContents.executeJavaScript(`
          navigator.hid.getDevices().then(devices => {
            devices[0].forget();
            return {
              vendorId: devices[0].vendorId,
              productId: devices[0].productId,
              name: devices[0].productName
            }
          })
        `);
        const grantedDevices2 = await w.webContents.executeJavaScript('navigator.hid.getDevices()');
        expect(grantedDevices2.length).toBeLessThan(grantedDevices.length);
        if (deletedDevice.name !== '' && deletedDevice.productId && deletedDevice.vendorId) {
          expect(deletedDeviceFromEvent).toMatchObject(deletedDevice);
        }
      }
    }
  });

  it('does not crash when the requesting webContents is destroyed from the select-hid-device handler', async () => {
    const guest = (webContents as typeof ElectronInternal.WebContents).create({
      type: 'webview',
      embedder: w.webContents
    });
    await guest.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
    session.defaultSession.setPermissionCheckHandler(() => true);
    session.defaultSession.setDevicePermissionHandler(() => true);
    const selectFired = new Promise<void>((resolve) => {
      w.webContents.session.once('select-hid-device', () => {
        guest.destroy();
        resolve();
      });
    });
    guest.executeJavaScript('navigator.hid.requestDevice({filters: []})', true).catch(() => {});
    await selectFired;
    // destroy() tears the WebContents down in a later task,
    // which on a busy machine can be more than one timer tick away.
    await waitUntil(() => guest.isDestroyed());
  });
});

describe('navigator.usb', () => {
  let w: BrowserWindow;
  let server: http.Server;
  let serverUrl: string;
  beforeAll(async () => {
    w = new BrowserWindow({
      show: false
    });
    await w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
    server = http.createServer((req, res) => {
      res.setHeader('Content-Type', 'text/html');
      res.end('<body>');
    });
    serverUrl = (await listen(server)).url;
  });

  const requestDevices: any = (win = w) => {
    return win.webContents.executeJavaScript(
      `
      navigator.usb.requestDevice({filters: []}).then(device => device.toString()).catch(err => err.toString());
    `,
      true
    );
  };

  const getDevices: any = (win = w) => {
    return win.webContents.executeJavaScript(
      `
      navigator.usb.getDevices().then(devices => devices.map(device => device.toString())).catch(err => err.toString());
    `,
      true
    );
  };

  const notFoundError = "NotFoundError: Failed to execute 'requestDevice' on 'USB': No device selected.";

  afterAll(async () => {
    server.close();
    await closeAllWindows();
  });

  afterEach(() => {
    session.defaultSession.setPermissionCheckHandler(null);
    session.defaultSession.setDevicePermissionHandler(null);
    session.defaultSession.removeAllListeners('select-usb-device');
  });

  it('does not crash when using in-memory partitions', async () => {
    const sesWin = new BrowserWindow({
      show: false,
      webPreferences: {
        partition: 'test-partition'
      }
    });

    await sesWin.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
    const devices = await getDevices(sesWin);
    expect(devices).toEqual([]);

    let selectFired = false;
    sesWin.webContents.session.once('select-usb-device', (event, details, callback) => {
      selectFired = true;
      callback();
    });
    const device = await requestDevices(sesWin);
    expect(selectFired).toBe(true);
    expect(device).toBe(notFoundError);
  });

  it('does not return a device if select-usb-device event is not defined', async () => {
    w.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
    const device = await requestDevices();
    expect(device).toBe(notFoundError);
  });

  it('does not return a device when permission denied', async () => {
    let selectFired = false;
    w.webContents.session.on('select-usb-device', (event, details, callback) => {
      selectFired = true;
      callback();
    });
    session.defaultSession.setPermissionCheckHandler(() => false);
    const device = await requestDevices();
    expect(selectFired).toBe(false);
    expect(device).toBe(notFoundError);
  });

  it('returns a device when select-usb-device event is defined', async () => {
    let haveDevices = false;
    let selectFired = false;
    w.webContents.session.on('select-usb-device', (event, details, callback) => {
      expect(details.frame).toHaveProperty('frameTreeNodeId', expect.any(Number));
      selectFired = true;
      if (details.deviceList.length > 0) {
        haveDevices = true;
        callback(details.deviceList[0].deviceId);
      } else {
        callback();
      }
    });
    const device = await requestDevices();
    expect(selectFired).toBe(true);
    if (haveDevices) {
      expect(device).toContain('[object USBDevice]');
    } else {
      expect(device).toBe(notFoundError);
    }
    if (haveDevices) {
      // Verify that navigation will clear device permissions
      const grantedDevices = await w.webContents.executeJavaScript('navigator.usb.getDevices()');
      expect(grantedDevices).not.toHaveLength(0);
      w.loadURL(serverUrl);
      const [, , , , , frameProcessId, frameRoutingId] = await once(w.webContents, 'did-frame-navigate');
      const frame = webFrameMain.fromId(frameProcessId, frameRoutingId);
      expect(!!frame).toBe(true);
      if (frame) {
        const grantedDevicesOnNewPage = await frame.executeJavaScript('navigator.usb.getDevices()');
        expect(grantedDevicesOnNewPage).toHaveLength(0);
      }
    }
  });

  it('returns a device when DevicePermissionHandler is defined', async () => {
    let haveDevices = false;
    let selectFired = false;
    let gotDevicePerms = false;
    w.webContents.session.on('select-usb-device', (event, details, callback) => {
      selectFired = true;
      if (details.deviceList.length > 0) {
        const foundDevice = details.deviceList.find((device) => {
          if (device.productName && device.productName !== '' && device.serialNumber && device.serialNumber !== '') {
            haveDevices = true;
            return true;
          }
          return false;
        });
        if (foundDevice) {
          callback(foundDevice.deviceId);
          return;
        }
      }
      callback();
    });
    session.defaultSession.setDevicePermissionHandler(() => {
      gotDevicePerms = true;
      return true;
    });
    await w.webContents.executeJavaScript('navigator.usb.getDevices();', true);
    const device = await requestDevices();
    expect(selectFired).toBe(true);
    if (haveDevices) {
      expect(device).toContain('[object USBDevice]');
      expect(gotDevicePerms).toBe(true);
    } else {
      expect(device).toBe(notFoundError);
    }
  });

  it('supports device.forget()', async () => {
    let deletedDeviceFromEvent;
    let haveDevices = false;
    w.webContents.session.on('select-usb-device', (event, details, callback) => {
      if (details.deviceList.length > 0) {
        haveDevices = true;
        callback(details.deviceList[0].deviceId);
      } else {
        callback();
      }
    });
    w.webContents.session.on('usb-device-revoked', (event, details) => {
      deletedDeviceFromEvent = details.device;
    });
    await requestDevices();
    if (haveDevices) {
      const grantedDevices = await w.webContents.executeJavaScript('navigator.usb.getDevices()');
      if (grantedDevices.length > 0) {
        const deletedDevice: Electron.USBDevice = await w.webContents.executeJavaScript(`
          navigator.usb.getDevices().then(devices => {
            devices[0].forget();
            return {
              vendorId: devices[0].vendorId,
              productId: devices[0].productId,
              productName: devices[0].productName
            }
          })
        `);
        const grantedDevices2 = await w.webContents.executeJavaScript('navigator.usb.getDevices()');
        expect(grantedDevices2.length).toBeLessThan(grantedDevices.length);
        if (deletedDevice.productName !== '' && deletedDevice.productId && deletedDevice.vendorId) {
          expect(deletedDeviceFromEvent).toMatchObject(deletedDevice);
        }
      }
    }
  });

  it('does not crash when the requesting webContents is destroyed from the select-usb-device handler', async () => {
    const guest = (webContents as typeof ElectronInternal.WebContents).create({
      type: 'webview',
      embedder: w.webContents
    });
    await guest.loadFile(path.join(fixturesPath, 'pages', 'blank.html'));
    session.defaultSession.setPermissionCheckHandler(() => true);
    session.defaultSession.setDevicePermissionHandler(() => true);
    const selectFired = new Promise<void>((resolve) => {
      w.webContents.session.once('select-usb-device', () => {
        guest.destroy();
        resolve();
      });
    });
    guest.executeJavaScript('navigator.usb.requestDevice({filters: []})', true).catch(() => {});
    await selectFired;
    // destroy() tears the WebContents down in a later task,
    // which on a busy machine can be more than one timer tick away.
    await waitUntil(() => guest.isDestroyed());
  });
});

describe('iframe sandbox external protocols', () => {
  let server: http.Server;
  let serverUrl: string;
  let w: BrowserWindow;
  let openExternalRequests: string[];

  beforeAll(async () => {
    server = http.createServer((req, res) => {
      res.setHeader('Content-Type', 'text/html');
      if (req.url === '/child') {
        res.end('<script>location.href = "magnet:sandbox-test"</script>');
      } else {
        const sandbox = new URL(req.url!, serverUrl).searchParams.get('sandbox') ?? '';
        res.end(`<iframe sandbox="${sandbox}" src="/child"></iframe>`);
      }
    });
    serverUrl = (await listen(server)).url;
  });

  afterAll(() => {
    server.close();
  });

  beforeEach(() => {
    openExternalRequests = [];
    w = new BrowserWindow({ show: false });
    w.webContents.session.setPermissionRequestHandler((_wc, permission, callback, details) => {
      if (permission === 'openExternal') {
        openExternalRequests.push((details as any).externalURL);
      }
      callback(false);
    });
  });

  afterEach(() => {
    w.webContents.session.setPermissionRequestHandler(null);
    return closeAllWindows();
  });

  it('blocks navigation to external protocol from a sandboxed iframe', async () => {
    const consoleMessage = once(w.webContents, 'console-message');
    await w.loadURL(`${serverUrl}/?sandbox=${encodeURIComponent('allow-scripts')}`);
    const [{ message }] = await consoleMessage;
    expect(message).toMatch(/external protocol blocked by sandbox/);
    expect(openExternalRequests).toHaveLength(0);
  });

  it('allows navigation to external protocol with allow-top-navigation-to-custom-protocols', async () => {
    const requested = new Promise<void>((resolve) => {
      w.webContents.session.setPermissionRequestHandler((_wc, permission, callback, details) => {
        if (permission === 'openExternal') {
          openExternalRequests.push((details as any).externalURL);
          resolve();
        }
        callback(false);
      });
    });
    await w.loadURL(
      `${serverUrl}/?sandbox=${encodeURIComponent('allow-scripts allow-top-navigation-to-custom-protocols')}`
    );
    await requested;
    expect(openExternalRequests).toEqual(['magnet:sandbox-test']);
  });

  it('allows navigation to external protocol with allow-popups', async () => {
    const requested = new Promise<void>((resolve) => {
      w.webContents.session.setPermissionRequestHandler((_wc, permission, callback, details) => {
        if (permission === 'openExternal') {
          openExternalRequests.push((details as any).externalURL);
          resolve();
        }
        callback(false);
      });
    });
    await w.loadURL(`${serverUrl}/?sandbox=${encodeURIComponent('allow-scripts allow-popups')}`);
    await requested;
    expect(openExternalRequests).toEqual(['magnet:sandbox-test']);
  });
});

describe('external protocol permission attribution', () => {
  // The document that starts a navigation to an external protocol can be gone
  // by the time the browser handles it: the navigation lives in the navigating
  // frame (here, a popup), not in its initiator. The openExternal permission
  // request must not be attributed to whatever the popup's WebContents happens
  // to have committed (here, the trusted top-level origin).
  let trusted: http.Server;
  let untrusted: http.Server;
  let trustedUrl: string;
  let untrustedUrl: string;

  beforeAll(async () => {
    untrusted = http.createServer((req, res) => {
      if (req.url!.startsWith('/slow-redirect')) {
        // Give the initiating iframe time to navigate away first.
        setTimeout(600).then(() => {
          res.writeHead(302, { Location: 'magnet:attribution-test' });
          res.end();
        });
        return;
      }
      res.setHeader('Content-Type', 'text/html');
      if (req.url === '/self') {
        res.end('<script>location.href = "/slow-redirect"</script>');
        return;
      }
      // Cross-origin iframe content: open a popup on the trusted origin, send
      // it to the slow redirect, then leave.
      res.end(`<script>
        (async () => {
          const trusted = decodeURIComponent(location.hash.slice(1));
          const w = window.open(trusted + '/blank', 'popup');
          await new Promise(r => setTimeout(r, 400));
          w.location = location.origin + '/slow-redirect';
          await new Promise(r => setTimeout(r, 100));
          location.href = 'about:blank';
        })();
      </script>`);
    });
    untrustedUrl = (await listen(untrusted)).url.replace('127.0.0.1', 'localhost');
    trusted = http.createServer((req, res) => {
      res.setHeader('Content-Type', 'text/html');
      if (req.url === '/blank') {
        res.end('<p>trusted popup</p>');
      } else {
        res.end(`<iframe src="${untrustedUrl}/#${encodeURIComponent(trustedUrl)}"></iframe>`);
      }
    });
    trustedUrl = (await listen(trusted)).url;
  });

  afterAll(() => {
    trusted.close();
    untrusted.close();
  });

  afterEach(() => {
    session.defaultSession.setPermissionRequestHandler(null);
    return closeAllWindows();
  });

  it('does not report the popup main frame as the requester when the initiator is gone', async () => {
    const w = new BrowserWindow({ show: false });
    w.webContents.setWindowOpenHandler(() => ({
      action: 'allow',
      overrideBrowserWindowOptions: { show: false }
    }));
    const request = new Promise<any>((resolve) => {
      session.defaultSession.setPermissionRequestHandler((_wc, permission, callback, details) => {
        callback(false);
        if (permission === 'openExternal') resolve(details);
      });
    });
    await w.loadURL(trustedUrl);
    const details = await request;
    expect(details.externalURL).toBe('magnet:attribution-test');
    // Before the fix this was the trusted popup's URL with isMainFrame: true.
    expect(details.requestingUrl).not.toContain(new URL(trustedUrl).host);
    expect(details.requestingUrl).toBe(`${untrustedUrl}/`);
    expect(details.isMainFrame).toBeTypeOf('boolean');
  });

  it('keeps attributing a redirected navigation to the live document that started it', async () => {
    const w = new BrowserWindow({ show: false });
    await w.loadURL(`${trustedUrl}/blank`);
    const request = new Promise<any>((resolve) => {
      session.defaultSession.setPermissionRequestHandler((_wc, permission, callback, details) => {
        callback(false);
        if (permission === 'openExternal') resolve(details);
      });
    });
    // The trusted main frame navigates itself to an untrusted URL that
    // redirects to the external protocol; the requester is still that frame.
    w.webContents.executeJavaScript(`location.href = ${JSON.stringify(untrustedUrl + '/slow-redirect')}; true`);
    const details = await request;
    expect(details.externalURL).toBe('magnet:attribution-test');
    expect(details.requestingUrl).toBe(`${trustedUrl}/blank`);
    expect(details.isMainFrame).toBe(true);
  });

  it('attributes a redirected browser-initiated navigation to the redirecting origin', async () => {
    const w = new BrowserWindow({ show: false });
    await w.loadURL(`${trustedUrl}/blank`);
    const request = new Promise<any>((resolve) => {
      session.defaultSession.setPermissionRequestHandler((_wc, permission, callback, details) => {
        callback(false);
        if (permission === 'openExternal') resolve(details);
      });
    });
    w.loadURL(`${untrustedUrl}/slow-redirect`).catch(() => {});
    const details = await request;
    expect(details.externalURL).toBe('magnet:attribution-test');
    // Not the previously committed trusted page.
    expect(details.requestingUrl).toBe(`${untrustedUrl}/`);
    expect(details.isMainFrame).toBe(true);
  });

  it('attributes a direct browser-initiated navigation to the navigating main frame', async () => {
    const w = new BrowserWindow({ show: false });
    await w.loadURL(`${trustedUrl}/blank`);
    const request = new Promise<any>((resolve) => {
      session.defaultSession.setPermissionRequestHandler((wc, permission, callback, details) => {
        callback(false);
        if (permission === 'openExternal') resolve({ wc, details });
      });
    });
    w.loadURL('magnet:direct-test').catch(() => {});
    const { wc, details } = await request;
    expect(wc).toBe(w.webContents);
    expect(details.externalURL).toBe('magnet:direct-test');
    expect(details.requestingUrl).toBe(`${trustedUrl}/blank`);
    expect(details.isMainFrame).toBe(true);
  });

  it('still attributes to the initiating frame while it is alive', async () => {
    const w = new BrowserWindow({ show: false });
    const request = new Promise<any>((resolve) => {
      session.defaultSession.setPermissionRequestHandler((_wc, permission, callback, details) => {
        callback(false);
        if (permission === 'openExternal') resolve(details);
      });
    });
    await w.loadURL(`data:text/html,<iframe src="${untrustedUrl}/self"></iframe>`);
    // The iframe's own document navigates to the redirect and stays alive
    // until it resolves, so attribution comes from that document.
    const details = await request;
    expect(details.externalURL).toBe('magnet:attribution-test');
    expect(details.requestingUrl).toBe(`${untrustedUrl}/self`);
    expect(details.isMainFrame).toBe(false);
  });
});

describe('links opened into a new window', () => {
  // A modifier-clicked link goes through OpenURLFromTab. The window the app
  // creates for it must start its navigation as the clicking document did
  // (renderer-initiated, with that document as initiator), not as a fresh
  // browser-initiated load.
  let server: http.Server;
  let serverUrl: string;
  const requests: Record<string, http.IncomingHttpHeaders> = {};

  beforeAll(async () => {
    server = http.createServer((req, res) => {
      requests[req.url!] = req.headers;
      res.setHeader('Content-Type', 'text/html');
      if (req.url === '/popup') {
        res.end('<p>popup</p>');
        return;
      }
      res.end(`<a id="a" href="/popup" target="_blank">link</a><script>
        window.clickLink = () => document.getElementById('a').dispatchEvent(new MouseEvent('click', {
          ctrlKey: true, metaKey: true, bubbles: true, cancelable: true, view: window
        }));
      </script>`);
    });
    serverUrl = (await listen(server)).url;
  });
  afterAll(() => server.close());
  afterEach(closeAllWindows);

  it('navigates the new window as the initiating document', async () => {
    const w = new BrowserWindow({ show: false });
    w.webContents.setWindowOpenHandler(() => ({ action: 'allow', overrideBrowserWindowOptions: { show: false } }));
    await w.loadURL(`${serverUrl}/opener`);
    // The navigation starts before did-create-window is emitted, so hook the
    // new webContents as soon as it exists.
    const started = new Promise<any>((resolve) => {
      app.once('web-contents-created', (_event, contents) => {
        contents.once('did-start-navigation', (details: any) => resolve(details));
      });
    });
    const created = once(w.webContents, 'did-create-window') as Promise<[BrowserWindow, any]>;
    await w.webContents.executeJavaScript('window.clickLink(); true');
    const [child] = await created;
    const details = await started;
    if (child.webContents.isLoading()) await once(child.webContents, 'did-finish-load');
    expect(details.url).toBe(`${serverUrl}/popup`);
    // The clicking document is the initiator...
    expect(details.initiator).toEqual(expect.anything());
    expect(details.initiator.routingId).toBe(w.webContents.mainFrame.routingId);
    // ...so the request is same-origin rather than a browser-typed load.
    expect(requests['/popup']['sec-fetch-site']).toBe('same-origin');
  });
});

describe('iframe sandbox popups', () => {
  let server: http.Server;
  let serverUrl: string;
  let w: BrowserWindow;

  const makeChildHtml = (href: string) => `
    <script>
      addEventListener('DOMContentLoaded', () => {
        const a = document.createElement('a');
        a.href = ${JSON.stringify(href)};
        document.body.appendChild(a);
        a.dispatchEvent(new MouseEvent('click', {
          ctrlKey: true, metaKey: true, bubbles: true, cancelable: true, view: window
        }));
      });
    </script>`;

  beforeAll(async () => {
    server = http.createServer((req, res) => {
      res.setHeader('Content-Type', 'text/html');
      if (req.url === '/child') {
        res.end(makeChildHtml('https://example.com/sandbox-bypassed'));
      } else if (req.url === '/child-local-popup') {
        res.end(makeChildHtml('/popup'));
      } else if (req.url === '/csp-sandbox-top') {
        res.setHeader('Content-Security-Policy', 'sandbox allow-scripts allow-popups');
        res.end(makeChildHtml('/popup'));
      } else if (req.url === '/popup') {
        res.end('<title>popup</title>');
      } else {
        const params = new URL(req.url!, serverUrl).searchParams;
        const sandbox = params.get('sandbox') ?? '';
        const child = params.get('child') ?? '/child';
        res.end(`<iframe sandbox="${sandbox}" src="${child}"></iframe>`);
      }
    });
    serverUrl = (await listen(server)).url;
  });

  afterAll(() => {
    server.close();
  });

  beforeEach(() => {
    w = new BrowserWindow({ show: false });
  });

  afterEach(() => closeAllWindows());

  it('does not invoke setWindowOpenHandler from a sandboxed iframe without allow-popups', async () => {
    let handlerCalls = 0;
    w.webContents.setWindowOpenHandler(() => {
      handlerCalls++;
      return { action: 'deny' };
    });
    await w.loadURL(`${serverUrl}/?sandbox=${encodeURIComponent('allow-scripts')}`);
    await setTimeout(200);
    expect(handlerCalls).toBe(0);
  });

  it('does not create a BrowserWindow from a sandboxed iframe without allow-popups (no handler installed)', async () => {
    let created = false;
    w.webContents.on('did-create-window', () => {
      created = true;
    });
    await w.loadURL(`${serverUrl}/?sandbox=${encodeURIComponent('allow-scripts')}`);
    await setTimeout(200);
    expect(created).toBe(false);
  });

  it('invokes setWindowOpenHandler when allow-popups is set', async () => {
    let handlerCalls = 0;
    w.webContents.setWindowOpenHandler(() => {
      handlerCalls++;
      return { action: 'deny' };
    });
    await w.loadURL(`${serverUrl}/?sandbox=${encodeURIComponent('allow-scripts allow-popups')}`);
    await setTimeout(200);
    expect(handlerCalls).toBe(1);
  });

  async function openPopupFromSandboxedIframe(sandbox: string) {
    const didCreateWindow = once(w.webContents, 'did-create-window') as Promise<[BrowserWindow]>;
    await w.loadURL(
      `${serverUrl}/?child=${encodeURIComponent('/child-local-popup')}&sandbox=${encodeURIComponent(sandbox)}`
    );
    const [popup] = await didCreateWindow;
    if (popup.webContents.isLoading()) {
      await once(popup.webContents, 'did-finish-load');
    }
    return popup.webContents.executeJavaScript(
      `(() => {
      const result = { origin: String(self.origin) };
      try { localStorage.setItem('k', 'v'); result.localStorage = 'allowed'; } catch (e) { result.localStorage = e.name; }
      try { document.cookie = 'k=v'; result.cookie = 'allowed'; } catch (e) { result.cookie = e.name; }
      return result;
    })()`,
      true
    );
  }

  it('popups from a sandboxed iframe without allow-popups-to-escape-sandbox inherit the sandbox', async () => {
    const result = await openPopupFromSandboxedIframe('allow-scripts allow-popups');
    expect(result.origin).toBe('null');
    expect(result.localStorage).toBe('SecurityError');
    expect(result.cookie).toBe('SecurityError');
  });

  it('popups from a sandboxed iframe with allow-popups-to-escape-sandbox escape the sandbox', async () => {
    const result = await openPopupFromSandboxedIframe('allow-scripts allow-popups allow-popups-to-escape-sandbox');
    expect(result.origin).toBe(new URL(serverUrl).origin);
    expect(result.localStorage).toBe('allowed');
    expect(result.cookie).toBe('allowed');
  });

  it('popups opened by a sandboxed top-level frame inherit the sandbox', async () => {
    // A document made sandboxed by a CSP header is a sandboxed main frame (no
    // parent); a window it opens must stay sandboxed just like an iframe's does.
    const didCreateWindow = once(w.webContents, 'did-create-window') as Promise<[BrowserWindow]>;
    await w.loadURL(`${serverUrl}/csp-sandbox-top`);
    const [popup] = await didCreateWindow;
    if (popup.webContents.isLoading()) {
      await once(popup.webContents, 'did-finish-load');
    }
    const result = await popup.webContents.executeJavaScript(
      `(() => {
      const result = { origin: String(self.origin) };
      try { localStorage.setItem('k', 'v'); result.localStorage = 'allowed'; } catch (e) { result.localStorage = e.name; }
      try { document.cookie = 'k=v'; result.cookie = 'allowed'; } catch (e) { result.cookie = e.name; }
      return result;
    })()`,
      true
    );
    expect(result.origin).toBe('null');
    expect(result.localStorage).toBe('SecurityError');
    expect(result.cookie).toBe('SecurityError');
  });
});

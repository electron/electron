import {
  app,
  session,
  webFrameMain,
  BrowserWindow,
  ipcMain,
  type WebContents,
  type Extension,
  type Session,
  type ServiceWorkerInfo,
  type ServiceWorkersRunningStatusChangedEventParams
} from 'electron/main';

import { afterAll, afterEach, beforeAll, beforeEach, describe, expect, it } from 'vitest';
import { WebSocketServer } from 'ws';

import { spawn } from 'node:child_process';
import { randomUUID } from 'node:crypto';
import { once } from 'node:events';
import * as fs from 'node:fs/promises';
import * as http from 'node:http';
import * as os from 'node:os';
import * as path from 'node:path';
import { pathToFileURL } from 'node:url';

import { emittedNTimes, emittedUntil } from './lib/events-helpers.ts';
import { defer, ifit, listen, startRemoteControlApp, waitUntil } from './lib/spec-helpers.ts';
import { expectWarningMessages } from './lib/warning-helpers.ts';
import { closeAllWindows, closeWindow, cleanupWebContents } from './lib/window-helpers.ts';

const fixtures = path.join(import.meta.dirname, 'fixtures');

describe('chrome extensions', () => {
  const emptyPage = '<html><body><h1>EMPTY PAGE</h1></body></html>';

  // NB. extensions are only allowed on http://, https:// and ftp:// (!) urls by default.
  let server: http.Server;
  let url: string;
  let port: number;
  let wss: WebSocketServer;
  beforeAll(async () => {
    server = http.createServer((req, res) => {
      if (req.url === '/cors') {
        res.setHeader('Access-Control-Allow-Origin', 'http://example.com');
      }
      res.end(emptyPage);
    });

    wss = new WebSocketServer({ noServer: true });
    wss.on('connection', function connection(ws) {
      ws.on('message', function incoming(message) {
        if (message.toString() === 'foo') {
          ws.send('bar');
        }
      });
    });

    ({ port, url } = await listen(server));
  });
  afterAll(async () => {
    server.close();
    wss.close();
    await cleanupWebContents();
  });
  afterEach(closeAllWindows);
  afterEach(() => {
    for (const e of session.defaultSession.extensions.getAllExtensions()) {
      session.defaultSession.extensions.removeExtension(e.id);
    }
  });

  it('does not crash when using chrome.management', async () => {
    const customSession = session.fromPartition(`persist:${randomUUID()}`);
    const w = new BrowserWindow({ show: false, webPreferences: { session: customSession, sandbox: true } });
    await w.loadURL('about:blank');

    const promise = once(app, 'web-contents-created') as Promise<[any, WebContents]>;
    await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'persistent-background-page'));
    const args: any = await promise;
    const wc: Electron.WebContents = args[1];
    await expect(
      wc.executeJavaScript(`
      (() => {
        return new Promise((resolve) => {
          chrome.management.getSelf((info) => {
            resolve(info);
          });
        })
      })();
    `)
    ).resolves.toHaveProperty('id');
  });

  it('supports chrome.management with the management permission', async () => {
    const customSession = session.fromPartition(`persist:${crypto.randomUUID()}`);
    const extension = await customSession.extensions.loadExtension(
      path.join(fixtures, 'extensions', 'chrome-management')
    );
    const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
    await w.loadURL(`${extension.url}page.html`);

    const all = await w.webContents.executeJavaScript('chrome.management.getAll()');
    expect(all.map((info: any) => info.id)).toContain(extension.id);
    const info = await w.webContents.executeJavaScript(`chrome.management.get('${extension.id}')`);
    expect(info).toMatchObject({ id: extension.id, name: 'chrome-management' });
  });

  describe('Chrome Web Store origin', () => {
    // The core //extensions feature files expose some APIs to web pages on the
    // Chrome Web Store origin. Electron does not support chrome.webstorePrivate
    // and overrides it to be unavailable everywhere; make sure that sticks.
    const webstoreUrl = 'https://chromewebstore.google.com/category/extensions';
    let customSession: Session;
    let w: BrowserWindow;

    beforeEach(() => {
      customSession = session.fromPartition(`webstore-${randomUUID()}`);
      // Serve the origin locally so the test does not touch the network.
      customSession.protocol.handle(
        'https',
        () => new Response(emptyPage, { headers: { 'content-type': 'text/html' } })
      );
      w = new BrowserWindow({ show: false, webPreferences: { session: customSession, sandbox: true } });
    });

    afterEach(async () => {
      customSession.protocol.unhandle('https');
      await closeAllWindows();
    });

    it('does not expose chrome.webstorePrivate to web pages', async () => {
      await w.loadURL(webstoreUrl);
      const type = await w.webContents.executeJavaScript(
        "typeof chrome === 'undefined' ? 'undefined' : typeof chrome.webstorePrivate"
      );
      expect(type).toBe('undefined');
    });

    it('does not crash if the page tries to call chrome.webstorePrivate', async () => {
      await w.loadURL(webstoreUrl);
      // If bindings were ever exposed again this would reach the browser process,
      // which must respond with an error rather than crash.
      const result = await w.webContents.executeJavaScript(`new Promise((resolve) => {
        try {
          chrome.webstorePrivate.getReferrerChain(() => resolve(chrome.runtime.lastError ? chrome.runtime.lastError.message : 'ok'));
        } catch (e) {
          resolve('threw: ' + e.message);
        }
      })`);
      expect(result).toBeTypeOf('string');
      expect(result).toMatch(/^threw: |not supported/);
      expect(w.webContents.isCrashed()).toBe(false);
      // The browser process is still alive if we get here; do a round trip to be sure.
      expect(await w.webContents.executeJavaScript('1 + 1')).toBe(2);
    });
  });

  describe('host_permissions', async () => {
    let customSession: Session;
    let w: BrowserWindow;

    beforeEach(() => {
      customSession = session.fromPartition(`persist:${randomUUID()}`);
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          session: customSession,
          sandbox: true
        }
      });
    });

    afterEach(closeAllWindows);

    it('recognize malformed host permissions', async () => {
      await w.loadURL(url);

      await expectWarningMessages(
        async () => {
          const extPath = path.join(fixtures, 'extensions', 'host-permissions', 'malformed');
          await customSession.extensions.loadExtension(extPath);
        },
        { name: 'ExtensionLoadWarning', message: /URL pattern 'malformed_host' is malformed/ }
      );
    });

    it('can grant special privileges to urls with host permissions', async () => {
      const extPath = path.join(fixtures, 'extensions', 'host-permissions', 'privileged-tab-info');
      await customSession.extensions.loadExtension(extPath);

      await w.loadURL(url);

      const message = { method: 'query' };
      w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

      const [{ message: responseString }] = await once(w.webContents, 'console-message');
      const response = JSON.parse(responseString);

      expect(response).toHaveLength(1);

      const tab = response[0];
      expect(tab).toHaveProperty('url', expect.any(String));
      expect(tab).toHaveProperty('title', expect.any(String));
      expect(tab).toHaveProperty('active', expect.any(Boolean));
      expect(tab).toHaveProperty('autoDiscardable', expect.any(Boolean));
      expect(tab).toHaveProperty('discarded', expect.any(Boolean));
      expect(tab).toHaveProperty('groupId', expect.any(Number));
      expect(tab).toHaveProperty('highlighted', expect.any(Boolean));
      expect(tab).toHaveProperty('id', expect.any(Number));
      expect(tab).toHaveProperty('incognito', expect.any(Boolean));
      expect(tab).toHaveProperty('index', expect.any(Number));
      expect(tab).toHaveProperty('pinned', expect.any(Boolean));
      expect(tab).toHaveProperty('selected', expect.any(Boolean));
      expect(tab).toHaveProperty('windowId', expect.any(Number));
    });
  });

  it('supports minimum_chrome_version manifest key', async () => {
    const customSession = session.fromPartition(`persist:${randomUUID()}`);
    const w = new BrowserWindow({
      show: false,
      webPreferences: {
        session: customSession,
        sandbox: true
      }
    });

    await w.loadURL('about:blank');

    const extPath = path.join(fixtures, 'extensions', 'minimum-chrome-version');
    const load = customSession.extensions.loadExtension(extPath);
    await expect(load).rejects.toThrow(
      `Loading extension at ${extPath} failed with: This extension requires Chromium version 999 or greater.`
    );
  });

  function fetch(contents: WebContents, url: string) {
    return contents.executeJavaScript(`fetch(${JSON.stringify(url)})`);
  }

  it('bypasses CORS in requests made from extensions', async () => {
    const customSession = session.fromPartition(`persist:${randomUUID()}`);
    const w = new BrowserWindow({ show: false, webPreferences: { session: customSession, sandbox: true } });
    const extension = await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'ui-page'));
    await w.loadURL(`${extension.url}bare-page.html`);
    await fetch(w.webContents, `${url}/cors`);
  });

  it('loads an extension', async () => {
    // NB. we have to use a persist: session (i.e. non-OTR) because the
    // extension registry is redirected to the main session. so installing an
    // extension in an in-memory session results in it being installed in the
    // default session.
    const customSession = session.fromPartition(`persist:${randomUUID()}`);
    await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'red-bg'));
    const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
    await w.loadURL(url);
    const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor');
    expect(bg).toBe('red');
  });

  it('does not crash when loading an extension with missing manifest', async () => {
    const customSession = session.fromPartition(`persist:${randomUUID()}`);
    const promise = customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'missing-manifest'));
    await expect(promise).rejects.toThrow(/Manifest file is missing or unreadable/);
  });

  it('does not crash when failing to load an extension', async () => {
    const customSession = session.fromPartition(`persist:${randomUUID()}`);
    const promise = customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'load-error'));
    await expect(promise).rejects.toThrow();
  });

  it('serializes a loaded extension', async () => {
    const extensionPath = path.join(fixtures, 'extensions', 'red-bg');
    const manifest = JSON.parse(await fs.readFile(path.join(extensionPath, 'manifest.json'), 'utf-8'));
    const customSession = session.fromPartition(`persist:${randomUUID()}`);
    const extension = await customSession.extensions.loadExtension(extensionPath);
    expect(extension.id).toBeTypeOf('string');
    expect(extension.name).toBeTypeOf('string');
    expect(extension.path).toBeTypeOf('string');
    expect(extension.version).toBeTypeOf('string');
    expect(extension.url).toBeTypeOf('string');
    expect(extension.manifest).toEqual(manifest);
  });

  it('removes an extension', async () => {
    const customSession = session.fromPartition(`persist:${randomUUID()}`);
    const { id } = await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'red-bg'));
    {
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
      await w.loadURL(url);
      const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor');
      expect(bg).toBe('red');
    }
    customSession.extensions.removeExtension(id);
    {
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
      await w.loadURL(url);
      const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor');
      expect(bg).toBe('');
    }
  });

  it('emits extension lifecycle events', async () => {
    const customSession = session.fromPartition(`persist:${randomUUID()}`);

    const loadedPromise = once(customSession.extensions, 'extension-loaded');
    const readyPromise = emittedUntil(
      customSession.extensions,
      'extension-ready',
      (event: Event, extension: Extension) => {
        return extension.name !== 'Chromium PDF Viewer';
      }
    );
    const extension = await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'red-bg'));
    const [, loadedExtension] = await loadedPromise;
    const [, readyExtension] = await readyPromise;

    expect(loadedExtension).toEqual(extension);
    expect(readyExtension).toEqual(extension);

    const unloadedPromise = once(customSession.extensions, 'extension-unloaded');
    await customSession.extensions.removeExtension(extension.id);
    const [, unloadedExtension] = await unloadedPromise;
    expect(unloadedExtension).toEqual(extension);
  });

  it('lists loaded extensions in getAllExtensions', async () => {
    const customSession = session.fromPartition(`persist:${randomUUID()}`);
    const e = await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'red-bg'));
    expect(customSession.extensions.getAllExtensions()).toEqual([e]);
    customSession.extensions.removeExtension(e.id);
    expect(customSession.extensions.getAllExtensions()).toEqual([]);
  });

  it('gets an extension by id', async () => {
    const customSession = session.fromPartition(`persist:${randomUUID()}`);
    const e = await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'red-bg'));
    expect(customSession.extensions.getExtension(e.id)).toEqual(e);
  });

  it('confines an extension to the session it was loaded in', async () => {
    const customSession = session.fromPartition(`persist:${randomUUID()}`);
    await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'red-bg'));
    const w = new BrowserWindow({ show: false }); // not in the session
    await w.loadURL(url);
    const bg = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor');
    expect(bg).toBe('');
  });

  it('loading an extension in a temporary session throws an error', async () => {
    const customSession = session.fromPartition(randomUUID());
    await expect(customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'red-bg'))).rejects.toThrow(
      'Extensions cannot be loaded in a temporary session'
    );
  });

  describe('chrome.i18n', () => {
    let w: BrowserWindow;
    let extension: Extension;
    const exec = async (name: string) => {
      const p = once(ipcMain, 'success');
      await w.webContents.executeJavaScript(`exec('${name}')`);
      const [, result] = await p;
      return result;
    };
    beforeEach(async () => {
      const customSession = session.fromPartition(`persist:${randomUUID()}`);
      extension = await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'chrome-i18n', 'v2'));
      w = new BrowserWindow({
        show: false,
        webPreferences: { session: customSession, nodeIntegration: true, contextIsolation: false }
      });
      await w.loadURL(url);
    });
    afterEach(() => {
      w.close();
      w = null as unknown as BrowserWindow;
    });
    it('getAcceptLanguages()', async () => {
      const result = await exec('getAcceptLanguages');
      expect(result).toBeInstanceOf(Array);
      expect(result).toEqual(['en-US', 'en']);
    });
    it('getMessage()', async () => {
      const result = await exec('getMessage');
      expect(result.id).toBeTypeOf('string');
      expect(result.id).toBe(extension.id);
      expect(result.name).toBeTypeOf('string');
      expect(result.name).toBe('chrome-i18n');
    });
  });

  describe('chrome.runtime', () => {
    let w: BrowserWindow;
    const exec = async (name: string) => {
      const p = once(ipcMain, 'success');
      await w.webContents.executeJavaScript(`exec('${name}')`);
      const [, result] = await p;
      return result;
    };
    beforeEach(async () => {
      const customSession = session.fromPartition(`persist:${randomUUID()}`);
      await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'chrome-runtime'));
      w = new BrowserWindow({
        show: false,
        webPreferences: { session: customSession, nodeIntegration: true, contextIsolation: false }
      });
      await w.loadURL(url);
    });
    afterEach(async () => {
      w.close();
      w = null as unknown as BrowserWindow;
    });
    it('getManifest()', async () => {
      const result = await exec('getManifest');
      expect(result).toBeTypeOf('object');
      expect(result).toHaveProperty('name', 'chrome-runtime');
    });
    it('id', async () => {
      const result = await exec('id');
      expect(result).toBeTypeOf('string');
      expect(result).toHaveLength(32);
    });
    it('getURL()', async () => {
      const result = await exec('getURL');
      expect(result).toBeTypeOf('string');
      expect(result).toMatch(/^chrome-extension:\/\/.*main.js$/);
    });
    it('getPlatformInfo()', async () => {
      const result = await exec('getPlatformInfo');
      expect(result).toBeTypeOf('object');
      expect(result.os).toBeTypeOf('string');
      expect(result.arch).toBeTypeOf('string');
    });
  });

  describe('chrome.cookies', () => {
    let customSession: Session;
    let w: BrowserWindow;

    beforeEach(async () => {
      customSession = session.fromPartition(`persist:${crypto.randomUUID()}`);
      const extension = await customSession.extensions.loadExtension(
        path.join(fixtures, 'extensions', 'chrome-cookies')
      );
      w = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
      await w.loadURL(`${extension.url}page.html`);
    });

    it('lists the session as a cookie store', async () => {
      const stores = await w.webContents.executeJavaScript('chrome.cookies.getAllCookieStores()');
      expect(stores).toEqual([{ id: '0', tabIds: [w.webContents.id] }]);
    });

    it('sets and gets cookies and emits onChanged', async () => {
      const cookieUrl = `${url}/`;
      const { change, cookie } = await w.webContents.executeJavaScript(`(async () => {
        const changed = new Promise((resolve) => chrome.cookies.onChanged.addListener(resolve));
        await chrome.cookies.set({ url: ${JSON.stringify(cookieUrl)}, name: 'key', value: 'value' });
        return { change: await changed, cookie: await chrome.cookies.get({ url: ${JSON.stringify(cookieUrl)}, name: 'key' }) };
      })()`);
      expect(cookie).toMatchObject({ name: 'key', value: 'value', storeId: '0' });
      expect(change).toMatchObject({ removed: false, cause: 'explicit', cookie: { name: 'key', value: 'value' } });

      const cookies = await customSession.cookies.get({ url: cookieUrl, name: 'key' });
      expect(cookies.map((c) => c.value)).toEqual(['value']);
    });
  });

  describe('chrome.storage', () => {
    it('stores and retrieves a key', async () => {
      const customSession = session.fromPartition(`persist:${randomUUID()}`);
      await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'chrome-storage'));
      const w = new BrowserWindow({
        show: false,
        webPreferences: { session: customSession, nodeIntegration: true, contextIsolation: false }
      });
      try {
        const p = once(ipcMain, 'storage-success');
        await w.loadURL(url);
        const [, v] = await p;
        expect(v).toBe('value');
      } finally {
        w.destroy();
      }
    });

    it('emits onChanged in a service worker', async () => {
      const customSession = session.fromPartition(`persist:${randomUUID()}`);
      const extension = await customSession.extensions.loadExtension(
        path.join(fixtures, 'extensions', 'chrome-storage-service-worker')
      );
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
      try {
        await w.loadURL(`${extension.url}page.html`);
        const change = await w.webContents.executeJavaScript("chrome.runtime.sendMessage('set-and-wait-for-change')");
        expect(change).toEqual({ changes: { key: { newValue: 'value' } }, areaName: 'local' });
      } finally {
        w.destroy();
      }
    });

    it('supports chrome.storage.sync as a storage area separate from local', async () => {
      const customSession = session.fromPartition(`persist:${randomUUID()}`);
      await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'chrome-storage-sync'));
      const w = new BrowserWindow({
        show: false,
        webPreferences: { session: customSession, nodeIntegration: true, contextIsolation: false }
      });
      try {
        const p = once(ipcMain, 'storage-sync-result');
        await w.loadURL(url);
        const [, json] = await p;
        const result = JSON.parse(json);
        expect(result.error).toBeUndefined();
        expect(result.change).toEqual({
          key: { newValue: 'sync-value' },
          other: { newValue: 'sync-other' }
        });
        expect(result.sync).toEqual({ key: 'sync-value', other: 'sync-other' });
        expect(result.local).toEqual({ key: 'local-value' });
        expect(result.syncAfterRemove).toEqual({ key: 'sync-value' });
        expect(result.bytesInUse).toBeTypeOf('number');
        expect(result.bytesInUse).toBeGreaterThan(0);
        expect(result.perItemError).toBeTypeOf('string');
        expect(result.perItemError).toContain('kQuotaBytesPerItem quota exceeded');
        expect(result.syncAfterClear).toEqual({});
        expect(result.localAfterSyncClear).toEqual({ key: 'local-value' });
        expect(result.constants).toEqual({
          QUOTA_BYTES: 102400,
          QUOTA_BYTES_PER_ITEM: 8192,
          MAX_ITEMS: 512
        });
      } finally {
        w.destroy();
      }
    });
  });

  describe('chrome.webRequest', () => {
    function fetch(contents: WebContents, url: string) {
      return contents.executeJavaScript(`fetch(${JSON.stringify(url)})`);
    }

    let customSession: Session;
    let w: BrowserWindow;

    beforeEach(() => {
      customSession = session.fromPartition(`persist:${randomUUID()}`);
      w = new BrowserWindow({
        show: false,
        webPreferences: { session: customSession, sandbox: true, contextIsolation: true }
      });
    });

    afterEach(() => {
      w.close();
      w = null as unknown as BrowserWindow;
    });

    describe('onBeforeRequest', () => {
      async function haveRejectedFetch() {
        try {
          await fetch(w.webContents, url);
        } catch (ex: any) {
          return ex.message === 'Failed to fetch';
        }
        return false;
      }

      it('can cancel http requests', async () => {
        await w.loadURL(url);
        await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'chrome-webRequest'));
        await waitUntil(haveRejectedFetch);
      });

      it('does not cancel http requests when no extension loaded', async () => {
        await w.loadURL(url);
        await fetch(w.webContents, url);
      });

      it('loads a file worker when only an extension uses webRequest', async () => {
        await w.loadURL(url);
        const extension = await customSession.extensions.loadExtension(
          path.join(fixtures, 'extensions', 'chrome-webRequest')
        );

        const workerDataUrl = pathToFileURL(path.join(fixtures, 'pages', 'worker-data.txt')).toString();
        const seenProtocolUrls: string[] = [];
        customSession.protocol.handle('file', (req) => {
          seenProtocolUrls.push(req.url);
          if (req.url === workerDataUrl) {
            return new Response('pong');
          }
          return customSession.fetch(req, { bypassCustomProtocolHandlers: true });
        });

        try {
          await waitUntil(haveRejectedFetch);
          await w.loadFile(path.join(fixtures, 'pages', 'a.html'));

          const reply = await w.webContents.executeJavaScript(`
            new Promise((resolve, reject) => {
              const worker = new Worker(
                new URL('../workers/worker.js', location.href).href
              );
              worker.onmessage = async (event) => {
                try {
                  const res = await fetch(${JSON.stringify(workerDataUrl)});
                  const text = await res.text();
                  worker.terminate();
                  resolve(\`\${event.data}:\${text}\`);
                } catch (err) {
                  worker.terminate();
                  reject(err);
                }
              };
              worker.onerror = (event) => {
                worker.terminate();
                reject(new Error(event.message || 'worker error'));
              };
              worker.postMessage('ping');
            })
          `);

          expect(reply).toBe('ping:pong');
          expect(seenProtocolUrls).toContain(workerDataUrl);
        } finally {
          customSession.protocol.unhandle('file');
          customSession.extensions.removeExtension(extension.id);
        }
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

          await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'chrome-webRequest'));
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
          await w.loadFile(path.join(fixtures, 'api', 'webrequest.html'), { query: { port: `${port}` } });
          await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'chrome-webRequest-wss'));
        })();
      });
    });

    describe('WebSocket', () => {
      it('can be proxied', async () => {
        await w.loadFile(path.join(fixtures, 'api', 'webrequest.html'), { query: { port: `${port}` } });
        await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'chrome-webRequest-wss'));
        customSession.webRequest.onSendHeaders((details) => {
          if (details.url.startsWith('ws://')) {
            expect(details.requestHeaders.foo).toBe('bar');
          }
        });
      });
    });

    it('reports the tab id of the requesting WebContents', async () => {
      const extension = await customSession.extensions.loadExtension(
        path.join(fixtures, 'extensions', 'request-tab-id')
      );
      const page = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
      await page.loadURL(`${extension.url}page.html`);
      // Wait for the service worker to register its listener.
      await page.webContents.executeJavaScript("chrome.runtime.sendMessage({ type: 'ping' })");

      await w.loadURL(`${url}/?tab-id`);
      const request = await page.webContents.executeJavaScript(
        "chrome.runtime.sendMessage({ type: 'wait-for-request', marker: 'tab-id' })"
      );
      expect(request.tabId).toBe(w.webContents.id);
    });
  });

  describe('chrome.declarativeNetRequest', () => {
    it('matches rules restricted to a tab', async () => {
      const customSession = session.fromPartition(`persist:${crypto.randomUUID()}`);
      const extension = await customSession.extensions.loadExtension(
        path.join(fixtures, 'extensions', 'request-tab-id')
      );
      const page = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
      const tab = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
      const otherTab = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
      await Promise.all([page.loadURL(`${extension.url}page.html`), tab.loadURL(url), otherTab.loadURL(url)]);

      await page.webContents.executeJavaScript(
        `chrome.runtime.sendMessage({ type: 'block-in-tab', marker: 'blocked-in-tab', tabId: ${tab.webContents.id} })`
      );

      const canFetch = (contents: WebContents) =>
        contents.executeJavaScript(`fetch('${url}/blocked-in-tab').then(() => true, () => false)`);
      expect(await canFetch(otherTab.webContents)).toBe(true);
      expect(await canFetch(tab.webContents)).toBe(false);
    });
  });

  describe('tab identity', () => {
    const fixture = (name: string) => path.join(fixtures, 'extensions', 'tab-identity', name);
    let ses: Session;
    let extension: Extension;
    // An extension page the app has not registered, used to reach the
    // extension's service worker.
    let driver: BrowserWindow;

    const createWindow = () => new BrowserWindow({ show: false, webPreferences: { session: ses } });
    const ask = (message: object, from: WebContents = driver.webContents) =>
      from.executeJavaScript(
        `new Promise((resolve) => chrome.runtime.sendMessage(${JSON.stringify(message)}, resolve))`
      );
    const call = (api: string, ...args: unknown[]) => ask({ type: 'call', api, args });
    const tabError = (contents: WebContents, details: object) => () => ses.extensions.addTab(contents, details as any);

    beforeEach(async () => {
      ses = session.fromPartition(`persist:${crypto.randomUUID()}`);
      extension = await ses.extensions.loadExtension(fixture('mv3'));
      driver = createWindow();
      await driver.loadURL(`${extension.url}page.html`);
    });

    it('reports the window of a registered tab to webRequest and sender.tab', async () => {
      ses.extensions.setTabMode('registered');
      const watched = createWindow();
      const other = createWindow();
      // 7 is the window the fixture filters its second webRequest listener on.
      ses.extensions.addTab(watched.webContents, { windowId: 7, index: 2, active: true });
      ses.extensions.addTab(other.webContents, { windowId: 8, index: 0 });
      await other.loadURL(`${url}/?other-window`);
      await watched.loadURL(`${url}/?watched-window`);

      const request = await ask({ type: 'wait-for', list: 'windowRequests', marker: 'watched-window' });
      expect(request.tabId).toBe(watched.webContents.id);
      // The unfiltered listener saw the other window's request, which was sent
      // first, so only the windowId filter can have kept it from this one.
      const unfiltered = await ask({ type: 'wait-for', list: 'requests', marker: 'other-window' });
      expect(unfiltered.tabId).toBe(other.webContents.id);
      const windowRequests: { url: string }[] = await ask({ type: 'list', list: 'windowRequests' });
      expect(windowRequests.filter((r) => r.url.includes('other-window'))).toEqual([]);

      const sender = await ask({ type: 'wait-for', list: 'senders', marker: 'watched-window' });
      expect(sender.tab).toMatchObject({ id: watched.webContents.id, windowId: 7, index: 2, active: true });
      const otherSender = await ask({ type: 'wait-for', list: 'senders', marker: 'other-window' });
      expect(otherSender.tab).toMatchObject({ id: other.webContents.id, windowId: 8, index: 0, active: false });

      ses.extensions.updateTab(other.webContents, { windowId: 7, index: 3 });
      await other.loadURL(`${url}/?moved-window`);
      const moved = await ask({ type: 'wait-for', list: 'windowRequests', marker: 'moved-window' });
      expect(moved.tabId).toBe(other.webContents.id);
    });

    it('gives no sender.tab to a popup or to an unregistered webContents in registered mode', async () => {
      ses.extensions.setTabMode('registered');
      const tab = createWindow();
      const popup = createWindow();
      ses.extensions.addTab(tab.webContents, { windowId: 1, index: 0 });
      ses.extensions.addTab(popup.webContents, { type: 'popup', windowId: 1 });
      await Promise.all([tab.loadURL(`${extension.url}page.html`), popup.loadURL(`${extension.url}page.html`)]);

      const fromTab = await ask({ type: 'sender' }, tab.webContents);
      expect(fromTab.tab).toMatchObject({ id: tab.webContents.id, windowId: 1 });
      expect((await ask({ type: 'sender' }, popup.webContents)).hasTab).toBe(false);
      expect((await ask({ type: 'sender' })).hasTab).toBe(false);

      ses.extensions.removeTab(tab.webContents);
      expect((await ask({ type: 'sender' }, tab.webContents)).hasTab).toBe(false);
    });

    it('still treats an unregistered webContents as a tab by default', async () => {
      const popup = createWindow();
      ses.extensions.addTab(popup.webContents, { type: 'popup', windowId: 1 });
      await popup.loadURL(`${extension.url}page.html`);

      const fromDriver = await ask({ type: 'sender' });
      expect(fromDriver.tab).toHaveProperty('id', driver.webContents.id);
      expect((await call('tabs.get', driver.webContents.id)).result).toHaveProperty('id', driver.webContents.id);
      expect((await ask({ type: 'sender' }, popup.webContents)).hasTab).toBe(false);
    });

    for (const mode of ['all-web-contents', 'registered'] as const) {
      it(`rejects a view added as a popup wherever a tab is expected in ${mode} mode`, async () => {
        ses.extensions.setTabMode(mode);
        const popup = createWindow();
        ses.extensions.addTab(popup.webContents, { type: 'popup', windowId: 1 });
        await popup.loadURL(`${url}/?popup`);
        const { id } = popup.webContents;

        expect(await call('tabs.get', id)).toEqual({ error: 'No such tab' });
        expect(await call('tabs.update', id, { muted: true })).toEqual({ error: 'No such tab' });
        expect((await call('tabs.sendMessage', id, 'ping')).error).toMatch(/Receiving end does not exist/);
        expect((await ask({ type: 'execute-script', tabId: id })).error).toMatch(/No tab with id/);
        const tabs: { id: number }[] = (await call('tabs.query', {})).result;
        expect(tabs.map((tab) => tab.id)).not.toContain(id);

        const request = await ask({ type: 'wait-for', list: 'requests', marker: 'popup' });
        expect(request.tabId).toBe(-1);
        const sender = await ask({ type: 'wait-for', list: 'senders', marker: 'popup' });
        expect(sender.hasTab).toBe(false);
      });
    }

    it('rejects an unregistered webContents wherever a tab is expected in registered mode', async () => {
      ses.extensions.setTabMode('registered');
      const w = createWindow();
      await w.loadURL(`${url}/?unregistered`);
      const { id } = w.webContents;

      expect(await call('tabs.get', id)).toEqual({ error: 'No such tab' });
      expect((await call('tabs.sendMessage', id, 'ping')).error).toMatch(/Receiving end does not exist/);
      expect((await ask({ type: 'execute-script', tabId: id })).error).toMatch(/No tab with id/);
      expect((await call('tabs.query', {})).result).toEqual([]);
      const request = await ask({ type: 'wait-for', list: 'requests', marker: 'unregistered' });
      expect(request.tabId).toBe(-1);

      ses.extensions.addTab(w.webContents, { windowId: 1, index: 0 });
      expect((await call('tabs.get', id)).result).toMatchObject({ id, windowId: 1, index: 0 });
      expect((await call('tabs.sendMessage', id, 'ping')).result).toEqual({ echo: 'ping' });
    });

    it('runs tabs.executeScript without a tab ID in the active tab of the current window', async () => {
      const mv2 = await ses.extensions.loadExtension(fixture('mv2'));
      ses.extensions.setTabMode('registered');
      const mv2Driver = createWindow();
      const [a, b, c, d, e] = [0, 1, 2, 3, 4].map(createWindow);
      ses.extensions.addTab(a.webContents, { windowId: 1, index: 0 });
      ses.extensions.addTab(b.webContents, { windowId: 1, index: 1, active: true });
      ses.extensions.addTab(c.webContents, { windowId: 2, index: 0, active: true });
      ses.extensions.addTab(d.webContents, { windowId: 3, index: 0 });
      ses.extensions.addTab(e.webContents, { windowId: 3, index: 1, active: true });
      await Promise.all([
        mv2Driver.loadURL(`${mv2.url}page.html`),
        a.loadURL(`${url}/?a`),
        b.loadURL(`${url}/?b`),
        c.loadURL(`${url}/?c`),
        d.loadURL(`${mv2.url}page.html`),
        e.loadURL(`${url}/?e`)
      ]);
      // The background page is not in a window, so its current window is the
      // focused one.
      const run = () => ask({ type: 'execute-script', code: 'location.search' }, mv2Driver.webContents);

      expect(await run()).toEqual({ error: 'No current window' });
      ses.extensions.setFocusedWindow(1);
      expect(await run()).toEqual({ results: ['?b'] });
      ses.extensions.updateTab(a.webContents, { active: true });
      expect(await run()).toEqual({ results: ['?a'] });
      ses.extensions.setFocusedWindow(2);
      expect(await run()).toEqual({ results: ['?c'] });

      // A caller in a registered tab acts on its own window, focused or not.
      const fromTab = await d.webContents.executeJavaScript(`new Promise((resolve) => {
        chrome.tabs.executeScript({ code: 'location.search' }, resolve);
      })`);
      expect(fromTab).toEqual(['?e']);

      ses.extensions.updateTab(c.webContents, { active: false });
      expect(await run()).toEqual({ error: 'No active tab' });
      ses.extensions.setFocusedWindow(null);
      expect(await run()).toEqual({ error: 'No current window' });
    });

    it('matches chrome.tabs.query window filters against registered tabs', async () => {
      ses.extensions.setTabMode('registered');
      const [a, b, c, d] = [0, 1, 2, 3].map(createWindow);
      // Added out of order: results follow the window and index.
      ses.extensions.addTab(c.webContents, { windowId: 2, index: 0, active: true });
      ses.extensions.addTab(b.webContents, { windowId: 1, index: 1, active: true });
      ses.extensions.addTab(a.webContents, { windowId: 1, index: 0 });
      ses.extensions.addTab(d.webContents, { windowId: 1, index: 2 });
      await d.loadURL(`${extension.url}page.html`);
      ses.extensions.setFocusedWindow(2);
      const ids = (tabs: { id: number }[]) => tabs.map((tab) => tab.id);
      const query = async (queryInfo: object) => ids((await call('tabs.query', queryInfo)).result);
      const [idA, idB, idC, idD] = [a, b, c, d].map((w) => w.webContents.id);

      expect(await query({})).toEqual([idA, idB, idD, idC]);
      expect(await query({ windowId: 1 })).toEqual([idA, idB, idD]);
      expect(await query({ windowId: 1, index: 1 })).toEqual([idB]);
      expect(await query({ active: true })).toEqual([idB, idC]);
      expect(await query({ active: true, currentWindow: true })).toEqual([idC]);
      expect(await query({ lastFocusedWindow: false })).toEqual([idA, idB, idD]);

      // The current window of a caller in a registered tab is its own window.
      const fromTab = await d.webContents.executeJavaScript('chrome.tabs.query({ active: true, currentWindow: true })');
      expect(ids(fromTab)).toEqual([idB]);
    });

    it('reports the type of a registered view to runtime.getContexts', async () => {
      // Every extension page open here is registered: a build with DCHECKs
      // crashes in runtime.getContexts() while one has no view type.
      ses.extensions.addTab(driver.webContents, { windowId: 1, index: 0 });
      const popup = createWindow();
      const sidePanel = createWindow();
      ses.extensions.addTab(popup.webContents, { type: 'popup', windowId: 1 });
      ses.extensions.addTab(sidePanel.webContents, { type: 'side-panel', windowId: 1 });
      await Promise.all([popup.loadURL(`${extension.url}page.html`), sidePanel.loadURL(`${extension.url}page.html`)]);

      const contexts: { contextType: string }[] = (await call('runtime.getContexts', {})).result;
      expect(contexts.map((context) => context.contextType).sort()).toEqual([
        'BACKGROUND',
        'POPUP',
        'SIDE_PANEL',
        'TAB'
      ]);
    });

    it('adds custom properties to sender.tab and to chrome.tabs results', async () => {
      ses.extensions.setTabMode('registered');
      const tab = createWindow();
      const { id } = tab.webContents;
      const custom = { workspace: { name: 'work', pinnedIds: [1, 2] }, container: 'personal' };
      ses.extensions.addTab(tab.webContents, { windowId: 1, index: 0, pinned: true, ...custom });
      await tab.loadURL(`${url}/?custom`);
      const expected = { id, windowId: 1, index: 0, pinned: true, url: `${url}/?custom`, ...custom };

      const sender = await ask({ type: 'wait-for', list: 'senders', marker: 'custom' });
      expect(sender.tab).toMatchObject(expected);
      expect((await call('tabs.get', id)).result).toMatchObject(expected);
      expect((await call('tabs.query', { windowId: 1 })).result).toMatchObject([expected]);
      expect((await call('tabs.update', id, { muted: true })).result).toMatchObject(expected);

      ses.extensions.updateTab(tab.webContents, { workspace: 'home', container: null } as any);
      const updated = (await call('tabs.get', id)).result;
      expect(updated).toHaveProperty('workspace', 'home');
      expect(updated).toHaveProperty('pinned', true);
      expect(updated).not.toHaveProperty('container');
    });

    it('throws when details set a field Electron derives', () => {
      const tab = createWindow();
      const derived = [
        'audible',
        'height',
        'id',
        'incognito',
        'mutedInfo',
        'pendingUrl',
        'selected',
        'status',
        'title',
        'url',
        'width'
      ];
      for (const key of derived) {
        expect(tabError(tab.webContents, { windowId: 1, index: 0, [key]: 'spoofed' })).toThrow(
          `'${key}' is set by Electron`
        );
      }

      ses.extensions.addTab(tab.webContents, { windowId: 1, index: 0 });
      expect(() => ses.extensions.updateTab(tab.webContents, { url: 'https://example.com/' } as any)).toThrow(
        "'url' is set by Electron"
      );
    });

    it('throws for a webContents or details it cannot register', async () => {
      const tab = createWindow();
      const valid = { windowId: 1, index: 0 };
      expect(tabError(tab.webContents, { index: 0 })).toThrow("'windowId' is required");
      expect(tabError(tab.webContents, { windowId: 1 })).toThrow("'index' is required for a tab");
      expect(tabError(tab.webContents, { windowId: -1, index: 0 })).toThrow(
        "'windowId' must be a non-negative integer"
      );
      expect(tabError(tab.webContents, { ...valid, type: 'window' })).toThrow("'type' must be");
      expect(tabError(tab.webContents, { ...valid, type: 'popup' })).toThrow("'index' cannot be set on a view");
      expect(tabError(tab.webContents, { windowId: 1, type: 'side-panel', pinned: true })).toThrow(
        "'pinned' cannot be set on a view"
      );
      expect(tabError(tab.webContents, { ...valid, pinned: 'yes' })).toThrow("Invalid value for 'pinned'");
      expect(tabError(tab.webContents, { ...valid, groupId: -2 })).toThrow("Invalid value for 'groupId'");
      expect(tabError(tab.webContents, { ...valid, blob: 'x'.repeat(70_000) })).toThrow('too large');

      const otherSession = session.fromPartition(`persist:${crypto.randomUUID()}`);
      const foreign = new BrowserWindow({ show: false, webPreferences: { session: otherSession } });
      expect(() => ses.extensions.addTab(foreign.webContents, valid)).toThrow('belongs to another session');
      expect(() => ses.extensions.updateTab(foreign.webContents, { index: 1 })).toThrow('is not registered');

      const created = once(app, 'web-contents-created') as Promise<[unknown, WebContents]>;
      await ses.extensions.loadExtension(fixture('mv2'));
      const [, background] = await created;
      expect(background.getType()).toBe('backgroundPage');
      expect(tabError(background, valid)).toThrow('extension background page');

      const closed = createWindow();
      const closedContents = closed.webContents;
      closed.destroy();
      await waitUntil(() => closedContents.isDestroyed());
      expect(tabError(closedContents, valid)).toThrow('not destroyed');
      expect(() => ses.extensions.removeTab(closedContents)).not.toThrow();

      ses.extensions.addTab(tab.webContents, valid);
      expect(tabError(tab.webContents, valid)).toThrow('already registered');
      expect(() => ses.extensions.updateTab(tab.webContents, { type: 'popup' } as any)).toThrow(
        "'type' cannot be changed"
      );
      expect(() => ses.extensions.setTabMode('tabs' as any)).toThrow('mode must be');
      expect(() => ses.extensions.setFocusedWindow(-1)).toThrow('windowId must be');
    });
  });

  describe('chrome.tabs', () => {
    let customSession: Session;
    beforeAll(async () => {
      customSession = session.fromPartition(`persist:${randomUUID()}`);
      await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'chrome-api'));
    });
    afterEach(closeAllWindows);

    it('executeScript', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession, nodeIntegration: true } });
      await w.loadURL(url);

      const message = { method: 'executeScript', args: ['1 + 2'] };
      w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

      const [{ message: responseString }] = await once(w.webContents, 'console-message');
      const response = JSON.parse(responseString);

      expect(response).toBe(3);
    });

    it('connect', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession, nodeIntegration: true } });
      await w.loadURL(url);

      const portName = randomUUID();
      const message = { method: 'connectTab', args: [portName] };
      w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

      const [{ message: responseString }] = await once(w.webContents, 'console-message');
      const response = responseString.split(',');
      expect(response[0]).toBe(portName);
      expect(response[1]).toBe('howdy');
    });

    it('sendMessage receives the response', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession, nodeIntegration: true } });
      await w.loadURL(url);

      const message = { method: 'sendMessage', args: ['Hello World!'] };
      w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

      const [{ message: responseString }] = await once(w.webContents, 'console-message');
      const response = JSON.parse(responseString);

      expect(response.message).toBe('Hello World!');
      expect(response.tabId).toBe(w.webContents.id);
    });

    it('update', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession, nodeIntegration: true } });
      await w.loadURL(url);

      const w2 = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
      await w2.loadURL('about:blank');

      const w2Navigated = once(w2.webContents, 'did-navigate');

      const message = { method: 'update', args: [w2.webContents.id, { url }] };
      w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

      const [{ message: responseString }] = await once(w.webContents, 'console-message');
      const response = JSON.parse(responseString);

      await w2Navigated;

      expect(new URL(w2.getURL()).toString()).toBe(new URL(url).toString());

      expect(response.id).toBe(w2.webContents.id);
    });
  });

  describe('background pages', () => {
    afterEach(closeAllWindows);
    it('loads a lazy background page when sending a message', async () => {
      const customSession = session.fromPartition(`persist:${randomUUID()}`);
      await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'lazy-background-page'));
      const w = new BrowserWindow({
        show: false,
        webPreferences: { session: customSession, nodeIntegration: true, contextIsolation: false }
      });
      try {
        w.loadURL(url);
        const [, resp] = await once(ipcMain, 'bg-page-message-response');
        expect(resp.message).toEqual({ some: 'message' });
        expect(resp.sender.id).toBeTypeOf('string');
        expect(resp.sender.origin).toBe(url);
        expect(resp.sender.url).toBe(url + '/');
      } finally {
        w.destroy();
      }
    });

    it('can use extension.getBackgroundPage from a ui page', async () => {
      const customSession = session.fromPartition(`persist:${randomUUID()}`);
      const { id } = await customSession.extensions.loadExtension(
        path.join(fixtures, 'extensions', 'lazy-background-page')
      );
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
      await w.loadURL(`chrome-extension://${id}/page-get-background.html`);
      const receivedMessage = await w.webContents.executeJavaScript('window.completionPromise');
      expect(receivedMessage).toEqual({ some: 'message' });
    });

    it('can use extension.getBackgroundPage from a ui page', async () => {
      const customSession = session.fromPartition(`persist:${randomUUID()}`);
      const { id } = await customSession.extensions.loadExtension(
        path.join(fixtures, 'extensions', 'lazy-background-page')
      );
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
      await w.loadURL(`chrome-extension://${id}/page-get-background.html`);
      const receivedMessage = await w.webContents.executeJavaScript('window.completionPromise');
      expect(receivedMessage).toEqual({ some: 'message' });
    });

    it('can use runtime.getBackgroundPage from a ui page', async () => {
      const customSession = session.fromPartition(`persist:${randomUUID()}`);
      const { id } = await customSession.extensions.loadExtension(
        path.join(fixtures, 'extensions', 'lazy-background-page')
      );
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
      await w.loadURL(`chrome-extension://${id}/page-runtime-get-background.html`);
      const receivedMessage = await w.webContents.executeJavaScript('window.completionPromise');
      expect(receivedMessage).toEqual({ some: 'message' });
    });

    it('has session in background page', async () => {
      const customSession = session.fromPartition(`persist:${randomUUID()}`);
      const promise = once(app, 'web-contents-created') as Promise<[any, WebContents]>;
      const { id } = await customSession.extensions.loadExtension(
        path.join(fixtures, 'extensions', 'persistent-background-page')
      );
      const [, bgPageContents] = await promise;
      expect(bgPageContents.getType()).toBe('backgroundPage');
      await once(bgPageContents, 'did-finish-load');
      expect(bgPageContents.getURL()).toBe(`chrome-extension://${id}/_generated_background_page.html`);
      expect(bgPageContents.session).not.toBe(undefined);
    });

    it('can open devtools of background page', async () => {
      const customSession = session.fromPartition(`persist:${randomUUID()}`);
      const promise = once(app, 'web-contents-created') as Promise<[any, WebContents]>;
      await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'persistent-background-page'));
      const [, bgPageContents] = await promise;
      expect(bgPageContents.getType()).toBe('backgroundPage');
      bgPageContents.openDevTools();
      bgPageContents.closeDevTools();
    });
  });

  describe('devtools extensions', () => {
    let showPanelTimeoutId: any = null;
    afterEach(async () => {
      if (showPanelTimeoutId) clearTimeout(showPanelTimeoutId);
      await closeAllWindows();
    });
    const showLastDevToolsPanel = (w: BrowserWindow) => {
      w.webContents.once('devtools-opened', () => {
        const show = () => {
          if (w == null || w.isDestroyed()) return;
          const { devToolsWebContents } = w as unknown as { devToolsWebContents: WebContents | undefined };
          if (devToolsWebContents == null || devToolsWebContents.isDestroyed()) {
            return;
          }

          // Executed in the DevTools page.
          const showLastPanel = `(async () => {
            const { InspectorView } = await import('./ui/legacy/legacy.js');
            const instance = InspectorView.InspectorView.instance();
            const tabs = instance.tabbedPane.tabs;
            instance.showPanel(tabs[tabs.length - 1].id);
          })()`;
          devToolsWebContents.executeJavaScript(showLastPanel, false).then(() => {
            showPanelTimeoutId = setTimeout(show, 100);
          });
        };
        showPanelTimeoutId = setTimeout(show, 100);
      });
    };

    // TODO(jkleinsc) fix this flaky test on WOA
    ifit(process.platform !== 'win32' || process.arch !== 'arm64')('loads a devtools extension', async () => {
      const customSession = session.fromPartition(`persist:${randomUUID()}`);
      customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'devtools-extension'));
      const winningMessage = once(ipcMain, 'winning');
      const w = new BrowserWindow({
        show: true,
        webPreferences: { session: customSession, nodeIntegration: true, contextIsolation: false }
      });
      await w.loadURL(url);
      w.webContents.openDevTools();
      showLastDevToolsPanel(w);
      await winningMessage;
    });
  });

  describe('chrome extension content scripts', () => {
    const fixtures = path.resolve(import.meta.dirname, 'fixtures');
    const extensionPath = path.resolve(fixtures, 'extensions');

    const addExtension = (name: string) =>
      session.defaultSession.extensions.loadExtension(path.resolve(extensionPath, name));
    const removeAllExtensions = () => {
      for (const { id } of session.defaultSession.extensions.getAllExtensions()) {
        session.defaultSession.extensions.removeExtension(id);
      }
    };

    let responseIdCounter = 0;
    const executeJavaScriptInFrame = (webContents: WebContents, frameToken: string, code: string) => {
      return new Promise((resolve) => {
        const responseId = responseIdCounter++;
        ipcMain.once(`executeJavaScriptInFrame_${responseId}`, (event, result) => {
          resolve(result);
        });
        webContents.send('executeJavaScriptInFrame', frameToken, code, responseId);
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

          afterEach(async () => {
            removeAllExtensions();
            await closeWindow(w);
            w = null as unknown as BrowserWindow;
          });

          it('should run content script at document_start', async () => {
            await addExtension('content-script-document-start');
            w.webContents.once('dom-ready', async () => {
              const result = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor');
              expect(result).toBe('red');
            });
            w.loadURL(url);
          });

          const getWorldDiscoveryState = async () => {
            const worldDiscoveryState = once(ipcMain, 'world-discovery-state');
            w.webContents.send('get-world-discovery-state');
            const [, state] = await worldDiscoveryState;
            return state as {
              startupWorlds: number[];
              createdWorlds: number[];
              isolatedWorlds: number[];
            };
          };

          const expectExtensionWorldDiscovered = ({
            startupWorlds,
            createdWorlds,
            isolatedWorlds
          }: {
            startupWorlds: number[];
            createdWorlds: number[];
            isolatedWorlds: number[];
          }) => {
            expect(isolatedWorlds.some((worldId) => worldId >= 1 << 20)).toBe(true);
            expect([...startupWorlds, ...createdWorlds].some((worldId) => worldId >= 1 << 20)).toBe(true);
          };

          const createWorldDiscoveryWindow = async () => {
            await closeWindow(w);
            w = new BrowserWindow({
              show: false,
              width: 400,
              height: 400,
              webPreferences: {
                contextIsolation: contextIsolationEnabled,
                sandbox: sandboxEnabled,
                preload: path.join(fixtures, 'extensions', 'content-script-world-discovery', 'preload.js')
              }
            });
          };

          it('discovers extension isolated worlds before document_start scripts run', async () => {
            await createWorldDiscoveryWindow();
            await addExtension('content-script-world-discovery');
            await w.loadURL(url);

            const discoveryResult = await w.webContents.executeJavaScript(
              'document.documentElement.dataset.worldDiscovered'
            );
            expect(discoveryResult).toBe('yes');

            expectExtensionWorldDiscovered(await getWorldDiscoveryState());
          });

          it('discovers extension isolated worlds after reload', async () => {
            await createWorldDiscoveryWindow();
            await addExtension('content-script-world-discovery');
            await w.loadURL(url);

            const finish = once(w.webContents, 'did-finish-load');
            w.webContents.reload();
            await finish;

            const reloadedDiscoveryResult = await w.webContents.executeJavaScript(
              'document.documentElement.dataset.worldDiscovered'
            );
            expect(reloadedDiscoveryResult).toBe('yes');

            expectExtensionWorldDiscovered(await getWorldDiscoveryState());
          });

          it('should run content script at document_idle', async () => {
            await addExtension('content-script-document-idle');
            w.loadURL(url);
            const result = await w.webContents.executeJavaScript('document.body.style.backgroundColor');
            expect(result).toBe('red');
          });

          it('should run content script at document_end', async () => {
            await addExtension('content-script-document-end');
            w.webContents.once('did-finish-load', async () => {
              const result = await w.webContents.executeJavaScript('document.documentElement.style.backgroundColor');
              expect(result).toBe('red');
            });
            w.loadURL(url);
          });
        });

        describe('supports "all_frames" option', () => {
          const contentScript = path.resolve(fixtures, 'extensions/content-script');
          const contentPath = path.join(contentScript, 'frame-with-frame.html');

          // Computed style values
          const COLOR_RED = 'rgb(255, 0, 0)';
          const COLOR_BLUE = 'rgb(0, 0, 255)';
          const COLOR_TRANSPARENT = 'rgba(0, 0, 0, 0)';

          let server: http.Server;
          let port: number;
          beforeAll(async () => {
            server = http.createServer(async (_, res) => {
              try {
                const content = await fs.readFile(contentPath, 'utf-8');
                res.writeHead(200, { 'Content-Type': 'text/html' });
                res.end(content, 'utf-8');
              } catch (error) {
                res.writeHead(500);
                res.end(`Failed to load ${contentPath} : ${(error as NodeJS.ErrnoException).code}`);
              }
            });

            ({ port } = await listen(server));
          });

          afterAll(() => {
            server.close();
          });

          let extensionId: string;
          beforeEach(async () => {
            ({ id: extensionId } = await session.defaultSession.extensions.loadExtension(contentScript));
            w = new BrowserWindow({
              show: false,
              webPreferences: {
                // enable content script injection in subframes
                nodeIntegrationInSubFrames: true,
                preload: path.join(contentScript, 'all_frames-preload.js')
              }
            });
          });

          afterEach(async () => {
            await closeWindow(w);
            w = null as unknown as BrowserWindow;
            session.defaultSession.extensions.removeExtension(extensionId);
          });

          it('applies matching rules in subframes', async () => {
            const detailsPromise = emittedNTimes(w.webContents, 'did-frame-finish-load', 2);

            w.loadURL(`http://127.0.0.1:${port}`);
            const frameEvents = await detailsPromise;
            await Promise.all(
              frameEvents.map(async (frameEvent) => {
                const [, isMainFrame, frameProcessId, frameRoutingId] = frameEvent;
                const frame = webFrameMain.fromId(frameProcessId, frameRoutingId);
                expect(frame).not.toBeUndefined();
                const result: any = await executeJavaScriptInFrame(
                  w.webContents,
                  frame!.frameToken,
                  `(() => {
                    const a = document.getElementById('all_frames_enabled')
                    const b = document.getElementById('all_frames_disabled')
                    return {
                      enabledColor: getComputedStyle(a).backgroundColor,
                      disabledColor: getComputedStyle(b).backgroundColor
                    }
                  })()`
                );

                expect(result.enabledColor).toBe(COLOR_RED);
                expect(result.disabledColor).toBe(isMainFrame ? COLOR_BLUE : COLOR_TRANSPARENT);
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
    afterEach(async () => {
      for (const e of session.defaultSession.extensions.getAllExtensions()) {
        session.defaultSession.extensions.removeExtension(e.id);
      }
      await closeAllWindows();
    });

    it('loads a ui page of an extension', async () => {
      const { id } = await session.defaultSession.extensions.loadExtension(
        path.join(fixtures, 'extensions', 'ui-page')
      );
      const w = new BrowserWindow({ show: false });
      await w.loadURL(`chrome-extension://${id}/bare-page.html`);
      const textContent = await w.webContents.executeJavaScript('document.body.textContent');
      expect(textContent).toBe('ui page loaded ok\n');
    });

    it('can load resources', async () => {
      const { id } = await session.defaultSession.extensions.loadExtension(
        path.join(fixtures, 'extensions', 'ui-page')
      );
      const w = new BrowserWindow({ show: false });
      await w.loadURL(`chrome-extension://${id}/page-script-load.html`);
      const textContent = await w.webContents.executeJavaScript('document.body.textContent');
      expect(textContent).toBe('script loaded ok\n');
    });

    it('isolates only manifest-sandboxed pages from the extension process', async () => {
      const { id } = await session.defaultSession.extensions.loadExtension(
        path.join(fixtures, 'extensions', 'sandboxed-pages')
      );
      const w = new BrowserWindow({ show: false });
      await w.loadURL(`chrome-extension://${id}/page.html`);

      const { mainFrame } = w.webContents;
      const findFrame = (file: string) => mainFrame.frames.find((frame) => frame.url.endsWith(file))!;
      const hasExtensionApi = (frame: Electron.WebFrameMain) =>
        frame.executeJavaScript("typeof globalThis.chrome?.runtime?.getManifest === 'function'");

      // Listed under "sandbox" in the manifest: its own process, no API access.
      const sandboxed = findFrame('sandboxed.html');
      expect(sandboxed.processId).not.toBe(mainFrame.processId);
      expect(await hasExtensionApi(sandboxed)).toBe(false);

      // Sandboxed by its embedder only: stays in the extension's process.
      const inherited = findFrame('frame.html');
      expect(inherited.processId).toBe(mainFrame.processId);
      expect(await hasExtensionApi(inherited)).toBe(true);
    });

    it('localizes messages in stylesheets', async () => {
      const { id } = await session.defaultSession.extensions.loadExtension(
        path.join(fixtures, 'extensions', 'css-localization')
      );
      const w = new BrowserWindow({ show: false });
      await w.loadURL(`chrome-extension://${id}/page.html`);
      const content = await w.webContents.executeJavaScript("getComputedStyle(document.body, '::after').content");
      expect(content).toBe(`"${id}"`);
    });
  });

  describe('manifest v3', () => {
    afterEach(closeAllWindows);
    it('registers background service worker', async () => {
      const customSession = session.fromPartition(`persist:${randomUUID()}`);
      const registrationPromise = new Promise<string>((resolve) => {
        customSession.serviceWorkers.once('registration-completed', (event, { scope }) => resolve(scope));
      });
      const extension = await customSession.extensions.loadExtension(
        path.join(fixtures, 'extensions', 'mv3-service-worker')
      );
      const scope = await registrationPromise;
      expect(scope).toBe(extension.url);
    });

    it('launches background service worker', async () => {
      const customSession = session.fromPartition(`persist:${randomUUID()}`);
      const launchPromise = new Promise<void>((resolve) => {
        customSession.serviceWorkers.on('running-status-changed', ({ runningStatus }) => {
          if (runningStatus === 'running') resolve();
        });
      });
      const extension = await customSession.extensions.loadExtension(
        path.join(fixtures, 'extensions', 'mv3-service-worker')
      );
      await launchPromise;
      const serviceWorkers = customSession.serviceWorkers.getAllRunning();
      expect(Object.values(serviceWorkers).some((worker) => worker.scope === extension.url)).toBe(true);
    });

    it('launches background service worker when the extension is loaded again without restarting the app', async () => {
      const customSession = session.fromPartition(`persist:${randomUUID()}`);
      const extensionPath = path.join(fixtures, 'extensions', 'mv3-service-worker');

      const isWorkerRunning = (extension: Extension) => {
        const serviceWorkers = customSession.serviceWorkers.getAllRunning();
        return Object.values(serviceWorkers).some((worker) => worker.scope === extension.url);
      };

      const loadAndUnloadExtension = async () => {
        const launchPromise = new Promise<void>((resolve) => {
          customSession.serviceWorkers.on('running-status-changed', ({ runningStatus }) => {
            if (runningStatus === 'running') resolve();
          });
        });
        const extension = await customSession.extensions.loadExtension(extensionPath);
        await launchPromise;
        expect(isWorkerRunning(extension)).toBe(true);

        const stopPromise = new Promise<void>((resolve) => {
          customSession.serviceWorkers.on('running-status-changed', ({ runningStatus }) => {
            if (runningStatus === 'stopped') resolve();
          });
        });
        customSession.extensions.removeExtension(extension.id);
        await stopPromise;
        expect(isWorkerRunning(extension)).toBe(false);
      };

      for (let i = 0; i < 3; i++) {
        await loadAndUnloadExtension();
      }
    });

    // Regression test for https://github.com/electron/electron/issues/41613
    it('launches background service worker after restarting the app', async () => {
      const partition = `persist:${randomUUID()}`;
      const extensionPath = path.join(fixtures, 'extensions', 'mv3-service-worker');

      const runRemoteControlApp = async () => {
        const rc = await startRemoteControlApp();

        const exitPromise = once(rc.process, 'exit');

        const { workerScopes, extensionUrl } = await rc.remotely(
          async (partition: string, extensionPath: string) => {
            const { session } = require('electron/main');
            const { setTimeout } = require('node:timers/promises');

            const customSession = session.fromPartition(partition);

            const launchPromise = new Promise<void>((resolve) => {
              customSession.serviceWorkers.on(
                'running-status-changed',
                ({ runningStatus }: ServiceWorkersRunningStatusChangedEventParams) => {
                  if (runningStatus === 'running') resolve();
                }
              );
            });
            const extension = await customSession.extensions.loadExtension(extensionPath);
            await launchPromise;
            const serviceWorkers = customSession.serviceWorkers.getAllRunning();

            const workerScopes = Object.values(serviceWorkers).map((worker) => (worker as ServiceWorkerInfo).scope);
            const extensionUrl = extension.url;

            // Give Chromium some time to update extensions::kPrefHasStartedServiceWorker on disk
            await setTimeout(500);

            global.setTimeout(() => require('electron').app.quit());

            return { workerScopes, extensionUrl };
          },
          partition,
          extensionPath
        );

        await exitPromise;

        expect(workerScopes.includes(extensionUrl)).toBe(true);
      };

      for (let i = 0; i < 3; i++) {
        await runRemoteControlApp();
      }
    });

    // Loads a generated extension in a fresh app, reports what its worker saw
    // and quits. `write` replaces the extension's files between launches.
    const setUpRestartingExtension = async () => {
      const partition = `persist:${crypto.randomUUID()}`;
      const extensionPath = await fs.mkdtemp(path.join(os.tmpdir(), 'electron-extension-'));
      defer(() => fs.rm(extensionPath, { recursive: true, force: true }));

      const writeExtension = async (version: string, marker: string) => {
        await fs.writeFile(
          path.join(extensionPath, 'manifest.json'),
          JSON.stringify({
            name: 'restart',
            version,
            manifest_version: 3,
            background: { service_worker: 'background.js' }
          })
        );
        await fs.writeFile(path.join(extensionPath, 'page.html'), '<!doctype html>');
        await fs.writeFile(
          path.join(extensionPath, 'background.js'),
          `chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
            sendResponse({ marker: ${JSON.stringify(marker)} });
          });`
        );
      };

      const runRemoteControlApp = async () => {
        const rc = await startRemoteControlApp();
        const exitPromise = once(rc.process, 'exit');

        const state = await rc.remotely(
          async (partition: string, extensionPath: string) => {
            const { BrowserWindow, session } = require('electron/main');
            const { setTimeout } = require('node:timers/promises');

            const customSession = session.fromPartition(partition);
            const extension = await customSession.extensions.loadExtension(extensionPath);
            const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
            await w.loadURL(`${extension.url}page.html`);

            const state = await w.webContents.executeJavaScript('chrome.runtime.sendMessage({})');

            // Give Chromium some time to write the extension's prefs to disk.
            await setTimeout(500);

            global.setTimeout(() => require('electron').app.quit());

            return state;
          },
          partition,
          extensionPath
        );

        await exitPromise;
        return state;
      };

      return { write: writeExtension, launch: runRemoteControlApp };
    };

    it('runs the worker script on disk after restarting the app', async () => {
      const extension = await setUpRestartingExtension();

      await extension.write('1.0', 'first');
      expect((await extension.launch()).marker).toBe('first');

      // Same version, new script.
      await extension.write('1.0', 'second');
      expect((await extension.launch()).marker).toBe('second');
    });

    it('persists alarms and dynamic content scripts across restarts', async () => {
      const partition = `persist:${crypto.randomUUID()}`;
      const extensionPath = path.join(fixtures, 'extensions', 'persistent-state');

      const runRemoteControlApp = async (script: string) => {
        const rc = await startRemoteControlApp();
        const exitPromise = once(rc.process, 'exit');

        const result = await rc.remotely(
          async (partition: string, extensionPath: string, script: string) => {
            const { BrowserWindow, session } = require('electron/main');
            const { setTimeout } = require('node:timers/promises');

            const customSession = session.fromPartition(partition);
            const extension = await customSession.extensions.loadExtension(extensionPath);
            const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
            await w.loadURL(`${extension.url}page.html`);
            const result = await w.webContents.executeJavaScript(script);

            // Give Chromium some time to write the extension's state to disk.
            await setTimeout(500);

            global.setTimeout(() => require('electron').app.quit());

            return result;
          },
          partition,
          extensionPath,
          script
        );

        await exitPromise;
        return result;
      };

      const getState = `(async () => ({
        alarms: (await chrome.alarms.getAll()).map((alarm) => alarm.name),
        scripts: (await chrome.scripting.getRegisteredContentScripts()).map((script) => script.id)
      }))()`;

      const created = await runRemoteControlApp(`(async () => {
        await chrome.alarms.create('persisted', { delayInMinutes: 60 });
        await chrome.scripting.registerContentScripts([
          { id: 'persisted', js: ['content.js'], matches: ['<all_urls>'], persistAcrossSessions: true }
        ]);
        return ${getState};
      })()`);
      expect(created).toEqual({ alarms: ['persisted'], scripts: ['persisted'] });

      expect(await runRemoteControlApp(getState)).toEqual({ alarms: ['persisted'], scripts: ['persisted'] });
    });

    it('keeps the background service worker when storage data is cleared', async () => {
      const customSession = session.fromPartition(`persist:${crypto.randomUUID()}`);
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession, nodeIntegration: true } });
      const extension = await customSession.extensions.loadExtension(
        path.join(fixtures, 'extensions', 'mv3-service-worker')
      );

      const messageWorker = async () => {
        await w.loadURL(url);
        w.webContents.executeJavaScript("window.postMessage('fetch-confirmation', '*')");
        const [{ message: responseString }] = await once(w.webContents, 'console-message');
        return JSON.parse(responseString).message;
      };

      // A reply means the worker has finished registering. Until then the
      // registration is not protected.
      expect(await messageWorker()).toBe('Hello from background.js');

      await customSession.clearStorageData();

      const worker = await customSession.serviceWorkers.startWorkerForScope(extension.url);
      expect(worker.scope).toBe(extension.url);
      expect(await messageWorker()).toBe('Hello from background.js');
    });

    it('can write to the clipboard from the background service worker', { tags: ['serial'] }, async () => {
      const customSession = session.fromPartition(`persist:${crypto.randomUUID()}`);
      const extension = await customSession.extensions.loadExtension(
        path.join(fixtures, 'extensions', 'clipboard-service-worker')
      );
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
      await w.loadURL(`${extension.url}page.html`);
      const result = await w.webContents.executeJavaScript("chrome.runtime.sendMessage('from a service worker')");
      expect(result).toEqual({ written: true });
    });

    it('can run chrome extension APIs', async () => {
      const customSession = session.fromPartition(`persist:${randomUUID()}`);
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession, nodeIntegration: true } });
      await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'mv3-service-worker'));

      await w.loadURL(url);

      w.webContents.executeJavaScript("window.postMessage('fetch-confirmation', '*')");

      const [{ message: responseString }] = await once(w.webContents, 'console-message');
      const { message } = JSON.parse(responseString);

      expect(message).toBe('Hello from background.js');
    });

    describe('chrome.i18n', () => {
      let customSession: Session;
      let w = null as unknown as BrowserWindow;

      beforeAll(async () => {
        customSession = session.fromPartition(`persist:${randomUUID()}`);
        await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'chrome-i18n', 'v3'));
      });

      beforeEach(() => {
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            session: customSession,
            nodeIntegration: true
          }
        });
      });

      afterEach(closeAllWindows);

      it('getAcceptLanguages', async () => {
        await w.loadURL(url);

        const message = { method: 'getAcceptLanguages' };
        w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

        const [{ message: responseString }] = await once(w.webContents, 'console-message');
        const response = JSON.parse(responseString);

        expect(response, 'languages array is empty').toBeInstanceOf(Array);
        expect(response, 'languages array is empty').not.toHaveLength(0);
      });

      it('getUILanguage', async () => {
        await w.loadURL(url);

        const message = { method: 'getUILanguage' };
        w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

        const [{ message: responseString }] = await once(w.webContents, 'console-message');
        const response = JSON.parse(responseString);

        expect(response).toBeTypeOf('string');
      });

      it('getMessage', async () => {
        await w.loadURL(url);

        const message = { method: 'getMessage' };
        w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

        const [{ message: responseString }] = await once(w.webContents, 'console-message');
        const response = JSON.parse(responseString);

        expect(response).toBe('Hola mundo!!');
      });

      it('detectLanguage', async () => {
        await w.loadURL(url);

        const greetings = [
          'Ich liebe dich', // German
          'Mahal kita', // Filipino
          '愛してます', // Japanese
          'دوستت دارم', // Persian
          'Minä rakastan sinua' // Finnish
        ];
        const message = { method: 'detectLanguage', args: [greetings] };
        w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

        const [{ message: responseString }] = await once(w.webContents, 'console-message');
        const response = JSON.parse(responseString);

        expect(response).toBeInstanceOf(Array);

        for (const item of response) {
          expect(Object.keys(item)).toEqual(['isReliable', 'languages']);
        }

        const languages = response.map((r: { isReliable: boolean; languages: any[] }) => r.languages[0]);
        expect(languages).toEqual([
          { language: 'de', percentage: 100 },
          { language: 'fil', percentage: 100 },
          { language: 'ja', percentage: 100 },
          { language: 'ps', percentage: 100 },
          { language: 'fi', percentage: 100 }
        ]);
      });
    });

    // chrome.action is not supported in Electron. These tests only ensure
    // it does not explode.
    describe('chrome.action', () => {
      let customSession: Session;
      let w = null as unknown as BrowserWindow;

      beforeAll(async () => {
        customSession = session.fromPartition(`persist:${randomUUID()}`);
        await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'chrome-action-fail'));
      });

      beforeEach(() => {
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            session: customSession
          }
        });
      });

      afterEach(closeAllWindows);

      it('isEnabled', async () => {
        await w.loadURL(url);

        const message = { method: 'isEnabled' };
        w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

        const [{ message: responseString }] = await once(w.webContents, 'console-message');

        const response = JSON.parse(responseString);
        expect(response).toBe(false);
      });

      it('setIcon', async () => {
        await w.loadURL(url);

        const message = { method: 'setIcon' };
        w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

        const [{ message: responseString }] = await once(w.webContents, 'console-message');

        const response = JSON.parse(responseString);
        expect(response).toBe(null);
      });

      it('getBadgeText', async () => {
        await w.loadURL(url);

        const message = { method: 'getBadgeText' };
        w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

        const [{ message: responseString }] = await once(w.webContents, 'console-message');

        const response = JSON.parse(responseString);
        expect(response).toBe('');
      });
    });

    describe('chrome.tabs', () => {
      let customSession: Session;
      let w = null as unknown as BrowserWindow;

      beforeAll(async () => {
        customSession = session.fromPartition(`persist:${randomUUID()}`);
        await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'chrome-tabs', 'api-async'));
      });

      beforeEach(() => {
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            session: customSession
          }
        });
      });

      afterEach(closeAllWindows);

      it('getZoom', async () => {
        await w.loadURL(url);

        const message = { method: 'getZoom' };
        w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

        const [{ message: responseString }] = await once(w.webContents, 'console-message');

        const response = JSON.parse(responseString);
        expect(response).toBe(1);
      });

      it('setZoom', async () => {
        await w.loadURL(url);

        const message = { method: 'setZoom', args: [2] };
        w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

        const [{ message: responseString }] = await once(w.webContents, 'console-message');

        const response = JSON.parse(responseString);
        expect(response).toEqual(2);
      });

      it('getZoomSettings', async () => {
        await w.loadURL(url);

        const message = { method: 'getZoomSettings' };
        w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

        const [{ message: responseString }] = await once(w.webContents, 'console-message');

        const response = JSON.parse(responseString);
        expect(response).toEqual({
          defaultZoomFactor: 1,
          mode: 'automatic',
          scope: 'per-origin'
        });
      });

      it('setZoomSettings', async () => {
        await w.loadURL(url);

        const message = { method: 'setZoomSettings', args: [{ mode: 'disabled' }] };
        w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

        const [{ message: responseString }] = await once(w.webContents, 'console-message');

        const response = JSON.parse(responseString);
        expect(response).toEqual({
          defaultZoomFactor: 1,
          mode: 'disabled',
          scope: 'per-tab'
        });
      });

      describe('get', () => {
        afterEach(closeAllWindows);
        it('returns tab properties', async () => {
          await w.loadURL(url);

          const message = { method: 'get' };
          w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

          const [{ message: responseString }] = await once(w.webContents, 'console-message');

          const response = JSON.parse(responseString);
          expect(response).toHaveProperty('url', expect.any(String));
          expect(response).toHaveProperty('title', expect.any(String));
          expect(response).toHaveProperty('active', expect.any(Boolean));
          expect(response).toHaveProperty('autoDiscardable', expect.any(Boolean));
          expect(response).toHaveProperty('discarded', expect.any(Boolean));
          expect(response).toHaveProperty('groupId', expect.any(Number));
          expect(response).toHaveProperty('highlighted', expect.any(Boolean));
          expect(response).toHaveProperty('id', expect.any(Number));
          expect(response).toHaveProperty('incognito', expect.any(Boolean));
          expect(response).toHaveProperty('index', expect.any(Number));
          expect(response).toHaveProperty('pinned', expect.any(Boolean));
          expect(response).toHaveProperty('selected', expect.any(Boolean));
          expect(response).toHaveProperty('windowId', expect.any(Number));
        });

        it('does not return privileged properties without tabs permission', async () => {
          const noPrivilegeSes = session.fromPartition(`persist:${randomUUID()}`);
          await noPrivilegeSes.extensions.loadExtension(
            path.join(fixtures, 'extensions', 'chrome-tabs', 'no-privileges')
          );

          w = new BrowserWindow({ show: false, webPreferences: { session: noPrivilegeSes } });
          await w.loadURL(url);

          w.webContents.executeJavaScript("window.postMessage('{}', '*')");
          const [{ message: responseString }] = await once(w.webContents, 'console-message');
          const response = JSON.parse(responseString);
          expect(response).not.toHaveProperty('url');
          expect(response).not.toHaveProperty('title');
          expect(response).toHaveProperty('active', expect.any(Boolean));
          expect(response).toHaveProperty('autoDiscardable', expect.any(Boolean));
          expect(response).toHaveProperty('discarded', expect.any(Boolean));
          expect(response).toHaveProperty('groupId', expect.any(Number));
          expect(response).toHaveProperty('highlighted', expect.any(Boolean));
          expect(response).toHaveProperty('id', expect.any(Number));
          expect(response).toHaveProperty('incognito', expect.any(Boolean));
          expect(response).toHaveProperty('index', expect.any(Number));
          expect(response).toHaveProperty('pinned', expect.any(Boolean));
          expect(response).toHaveProperty('selected', expect.any(Boolean));
          expect(response).toHaveProperty('windowId', expect.any(Number));
        });

        it('does not return privileged properties from query without tabs permission', async () => {
          const noPrivilegeSes = session.fromPartition(`persist:${randomUUID()}`);
          await noPrivilegeSes.extensions.loadExtension(
            path.join(fixtures, 'extensions', 'chrome-tabs', 'no-privileges')
          );

          w = new BrowserWindow({ show: false, webPreferences: { session: noPrivilegeSes } });
          await w.loadURL(url);

          const message = { method: 'query' };
          w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);
          const [{ message: responseString }] = await once(w.webContents, 'console-message');
          const response = JSON.parse(responseString);
          expect(response).toBeInstanceOf(Array);
          expect(response).not.toHaveLength(0);
          for (const tab of response) {
            expect(tab).not.toHaveProperty('url');
            expect(tab).not.toHaveProperty('title');
            expect(tab).toHaveProperty('id', expect.any(Number));
          }
        });
      });

      it('reload', async () => {
        await w.loadURL(url);

        const finish = once(w.webContents, 'did-finish-load');
        const message = { method: 'reload' };
        w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);
        await finish;

        // chrome.tabs.reload() replaces the document that sent the 'reload'
        // message, so the reply to that message races the navigation commit and
        // is dropped when the commit wins. Ask the reloaded document instead.
        const consoleMessage = once(w.webContents, 'console-message');
        const statusMessage = { method: 'getReloadStatus' };
        w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(statusMessage)}', '*')`);

        const [{ message: responseString }] = await consoleMessage;
        const response = JSON.parse(responseString);
        expect(response.status).toBe('reloaded');
      });

      describe('update', () => {
        it('can update muted status', async () => {
          await w.loadURL(url);

          const message = { method: 'update', args: [{ muted: true }] };
          w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

          const [{ message: responseString }] = await once(w.webContents, 'console-message');
          const response = JSON.parse(responseString);

          expect(response).toHaveProperty('mutedInfo', expect.any(Object));
          const { mutedInfo } = response;
          expect(mutedInfo).toEqual({
            muted: true,
            reason: 'user'
          });
        });

        it('fails when navigating to an invalid url', async () => {
          await w.loadURL(url);

          const message = { method: 'update', args: [{ url: 'chrome://crash' }] };
          w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

          const [{ message: responseString }] = await once(w.webContents, 'console-message');
          const { error } = JSON.parse(responseString);
          expect(error).toBe("I'm sorry. I'm afraid I can't do that.");
        });

        it('fails when navigating to prohibited url', async () => {
          await w.loadURL(url);

          const message = { method: 'update', args: [{ url: 'chrome://crash' }] };
          w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

          const [{ message: responseString }] = await once(w.webContents, 'console-message');
          const { error } = JSON.parse(responseString);
          expect(error).toBe("I'm sorry. I'm afraid I can't do that.");
        });

        it('fails when navigating to a devtools url without permission', async () => {
          await w.loadURL(url);

          const message = { method: 'update', args: [{ url: 'devtools://blah' }] };
          w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

          const [{ message: responseString }] = await once(w.webContents, 'console-message');
          const { error } = JSON.parse(responseString);
          expect(error).toBe(
            'Cannot navigate to a devtools:// page without either the devtools or debugger permission.'
          );
        });

        it('fails when navigating to a chrome-untrusted url', async () => {
          await w.loadURL(url);

          const message = { method: 'update', args: [{ url: 'chrome-untrusted://blah' }] };
          w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

          const [{ message: responseString }] = await once(w.webContents, 'console-message');
          const { error } = JSON.parse(responseString);
          expect(error).toBe('Cannot navigate to a chrome-untrusted:// page.');
        });

        it('fails when navigating to a file url withotut file access', async () => {
          await w.loadURL(url);

          const message = { method: 'update', args: [{ url: 'file://blah' }] };
          w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

          const [{ message: responseString }] = await once(w.webContents, 'console-message');
          const { error } = JSON.parse(responseString);
          expect(error).toBe('Cannot navigate to a file URL without local file access.');
        });
      });

      describe('query', () => {
        afterEach(closeAllWindows);
        it('can query for a tab with specific properties', async () => {
          await w.loadURL(url);

          expect(w.webContents.isAudioMuted(), 'muted').toBe(false);
          w.webContents.setAudioMuted(true);
          expect(w.webContents.isAudioMuted(), 'not muted').toBe(true);

          const message = { method: 'query', args: [{ muted: true }] };
          w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

          const [{ message: responseString }] = await once(w.webContents, 'console-message');
          const response = JSON.parse(responseString);
          expect(response).toHaveLength(1);

          const tab = response[0];
          expect(tab.mutedInfo).toEqual({
            muted: true,
            reason: 'user'
          });
        });

        it('only returns tabs in the same session', async () => {
          await w.loadURL(url);
          w.webContents.setAudioMuted(true);

          const sameSessionWin = new BrowserWindow({
            show: false,
            webPreferences: {
              session: customSession
            }
          });

          sameSessionWin.webContents.setAudioMuted(true);

          const newSession = session.fromPartition(`persist:${randomUUID()}`);
          const differentSessionWin = new BrowserWindow({
            show: false,
            webPreferences: {
              session: newSession
            }
          });

          differentSessionWin.webContents.setAudioMuted(true);

          const message = { method: 'query', args: [{ muted: true }] };
          w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

          const [{ message: responseString }] = await once(w.webContents, 'console-message');
          const response = JSON.parse(responseString);
          expect(response).toHaveLength(2);
          for (const tab of response) {
            expect(tab.mutedInfo).toEqual({
              muted: true,
              reason: 'user'
            });
          }
        });
      });
    });

    describe('chrome.scripting', () => {
      let customSession: Session;
      let w = null as unknown as BrowserWindow;

      beforeAll(async () => {
        customSession = session.fromPartition(`persist:${randomUUID()}`);
        await customSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'chrome-scripting'));
      });

      beforeEach(() => {
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            session: customSession,
            nodeIntegration: true
          }
        });
      });

      afterEach(closeAllWindows);

      it('executeScript', async () => {
        await w.loadURL(url);

        const message = { method: 'executeScript' };
        w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

        const updated = await once(w.webContents, 'page-title-updated');
        expect(updated[1]).toBe('HEY HEY HEY');
      });

      it('registerContentScripts', async () => {
        await w.loadURL(url);

        const message = { method: 'registerContentScripts' };
        w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

        const [{ message: responseString }] = await once(w.webContents, 'console-message');
        const response = JSON.parse(responseString);
        expect(response).toBeInstanceOf(Array);
        expect(response).toHaveLength(1);
        expect(response[0]).toEqual({
          allFrames: false,
          id: 'session-script',
          js: ['content.js'],
          matchOriginAsFallback: false,
          matches: ['<all_urls>'],
          persistAcrossSessions: false,
          runAt: 'document_start',
          world: 'ISOLATED'
        });
      });

      it('globalParams', async () => {
        await w.loadURL(url);

        const message = { method: 'globalParams' };
        w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);
        const [{ message: responseString }] = await once(w.webContents, 'console-message');
        const response = JSON.parse(responseString);
        expect(response).toEqual({ changed: true });
      });

      it('insertCSS', async () => {
        await w.loadURL(url);

        const bgBefore = await w.webContents.executeJavaScript(
          'window.getComputedStyle(document.body).backgroundColor'
        );
        expect(bgBefore).toBe('rgba(0, 0, 0, 0)');

        const message = { method: 'insertCSS' };
        w.webContents.executeJavaScript(`window.postMessage('${JSON.stringify(message)}', '*')`);

        const [{ message: responseString }] = await once(w.webContents, 'console-message');
        const response = JSON.parse(responseString);
        expect(response.success).toBe(true);

        const bgAfter = await w.webContents.executeJavaScript('window.getComputedStyle(document.body).backgroundColor');
        expect(bgAfter).toBe('rgb(255, 0, 0)');
      });
    });

    describe('cross-session isolation', () => {
      let extSession: Session;
      let otherSession: Session;
      let driver: BrowserWindow;
      let victim: BrowserWindow;

      beforeAll(async () => {
        extSession = session.fromPartition(`persist:${randomUUID()}`);
        otherSession = session.fromPartition(`persist:${randomUUID()}`);
        await extSession.extensions.loadExtension(path.join(fixtures, 'extensions', 'tabs-cross-session'));
      });

      beforeEach(async () => {
        driver = new BrowserWindow({ show: false, webPreferences: { session: extSession } });
        victim = new BrowserWindow({ show: false, webPreferences: { session: otherSession } });
        await driver.loadURL(url);
        await victim.loadURL(url);
      });

      afterEach(closeAllWindows);

      const callExtension = async (method: string, tabId: number, args: any[] = []) => {
        const message = JSON.stringify({ method, tabId, args });
        const p = once(driver.webContents, 'console-message');
        await driver.webContents.executeJavaScript(`window.postMessage('${message}', '*')`);
        const [{ message: responseString }] = await p;
        return JSON.parse(responseString);
      };

      it('chrome.tabs.get cannot resolve a tab from another session', async () => {
        const sameSession = await callExtension('get', driver.webContents.id);
        expect(sameSession.ok).toBe(true);
        expect(sameSession.result.id).toBe(driver.webContents.id);

        const crossSession = await callExtension('get', victim.webContents.id);
        expect(crossSession.ok).toBe(false);
        expect(crossSession.error).toMatch(/No such tab|No tab with id/);
      });

      it('chrome.tabs.update cannot navigate a tab in another session', async () => {
        const before = victim.webContents.getURL();
        const crossSession = await callExtension('update', victim.webContents.id, [{ url }]);
        expect(crossSession.ok).toBe(false);
        expect(crossSession.error).toMatch(/No such tab|No tab with id/);
        expect(victim.webContents.getURL()).toBe(before);
      });

      it('chrome.scripting.executeScript cannot target a tab in another session', async () => {
        const crossSession = await callExtension('executeScript', victim.webContents.id);
        expect(crossSession.ok).toBe(false);
        expect(crossSession.error).toMatch(/No tab with id/);
      });

      it('chrome.tabs.sendMessage cannot reach a tab in another session', async () => {
        const crossSession = await callExtension('sendMessage', victim.webContents.id, ['ping']);
        expect(crossSession.ok).toBe(false);
      });
    });
  });

  describe('custom protocol', () => {
    async function runFixture(name: string) {
      const args = [path.join(fixtures, 'extensions', name, 'main.js')];
      if (process.platform === 'darwin') args.push('--use-mock-keychain');
      const appProcess = spawn(process.execPath, args);

      let output = '';
      appProcess.stdout.on('data', (data) => {
        output += data;
      });
      await once(appProcess.stdout, 'end');

      return output.trim();
    }

    it('loads DevTools extensions on custom protocols with allowExtensions privileges and runs content and background scripts', async () => {
      const output = await runFixture('custom-protocol');
      expect(output).toBe('Title: MESSAGE RECEIVED');
    });

    it('loads DevTools panels on custom protocols with allowExtensions privileges', async () => {
      const output = await runFixture('custom-protocol-panel');
      expect(output).toBe('ELECTRON TEST PANEL created');
    });
  });
});

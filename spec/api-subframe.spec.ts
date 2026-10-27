import { app, BrowserWindow, ipcMain, session } from 'electron/main';

import { afterAll, afterEach, beforeAll, beforeEach, describe, expect, it } from 'vitest';

import { once } from 'node:events';
import * as http from 'node:http';
import * as path from 'node:path';

import { emittedNTimes } from './lib/events-helpers.ts';
import { defer, ifdescribe, listen } from './lib/spec-helpers.ts';
import { closeWindow } from './lib/window-helpers.ts';

describe('renderer nodeIntegrationInSubFrames', () => {
  const generateTests = (description: string, webPreferences: any) => {
    describe(description, () => {
      const fixtureSuffix = webPreferences.webviewTag ? '-webview' : '';
      let w: BrowserWindow;

      beforeEach(async () => {
        await closeWindow(w);
        w = new BrowserWindow({
          show: false,
          width: 400,
          height: 400,
          webPreferences
        });
      });

      afterEach(async () => {
        await closeWindow(w);
        w = null as unknown as BrowserWindow;
      });

      it('should load preload scripts in top level iframes', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 2);
        w.loadFile(path.resolve(import.meta.dirname, `fixtures/sub-frames/frame-container${fixtureSuffix}.html`));
        const [event1, event2] = await detailsPromise;
        expect(event1[0].senderFrame.frameToken).not.toBe(event2[0].senderFrame.frameToken);
        expect(event1[0].senderFrame.frameToken).toBe(event1[2]);
        expect(event2[0].senderFrame.frameToken).toBe(event2[2]);
      });

      it('should load preload scripts in nested iframes', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 3);
        w.loadFile(
          path.resolve(import.meta.dirname, `fixtures/sub-frames/frame-with-frame-container${fixtureSuffix}.html`)
        );
        const [event1, event2, event3] = await detailsPromise;
        expect(event1[0].senderFrame.frameToken).not.toBe(event2[0].senderFrame.frameToken);
        expect(event1[0].senderFrame.frameToken).not.toBe(event3[0].senderFrame.frameToken);
        expect(event2[0].senderFrame.frameToken).not.toBe(event3[0].senderFrame.frameToken);
        expect(event1[0].senderFrame.frameToken).toBe(event1[2]);
        expect(event2[0].senderFrame.frameToken).toBe(event2[2]);
        expect(event3[0].senderFrame.frameToken).toBe(event3[2]);
      });

      it('should correctly reply to the main frame with using event.reply', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 2);
        w.loadFile(path.resolve(import.meta.dirname, `fixtures/sub-frames/frame-container${fixtureSuffix}.html`));
        const [event1] = await detailsPromise;
        const pongPromise = once(ipcMain, 'preload-pong');
        event1[0].reply('preload-ping');
        const [, frameToken] = await pongPromise;
        expect(frameToken).toBe(event1[0].senderFrame.frameToken);
      });

      it('should correctly reply to the main frame with using event.senderFrame.send', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 2);
        w.loadFile(path.resolve(import.meta.dirname, `fixtures/sub-frames/frame-container${fixtureSuffix}.html`));
        const [event1] = await detailsPromise;
        const pongPromise = once(ipcMain, 'preload-pong');
        event1[0].senderFrame.send('preload-ping');
        const [, frameToken] = await pongPromise;
        expect(frameToken).toBe(event1[0].senderFrame.frameToken);
      });

      it('should correctly reply to the sub-frames with using event.reply', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 2);
        w.loadFile(path.resolve(import.meta.dirname, `fixtures/sub-frames/frame-container${fixtureSuffix}.html`));
        const [, event2] = await detailsPromise;
        const pongPromise = once(ipcMain, 'preload-pong');
        event2[0].reply('preload-ping');
        const [, frameToken] = await pongPromise;
        expect(frameToken).toBe(event2[0].senderFrame.frameToken);
      });

      it('should correctly reply to the sub-frames with using event.senderFrame.send', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 2);
        w.loadFile(path.resolve(import.meta.dirname, `fixtures/sub-frames/frame-container${fixtureSuffix}.html`));
        const [, event2] = await detailsPromise;
        const pongPromise = once(ipcMain, 'preload-pong');
        event2[0].senderFrame.send('preload-ping');
        const [, frameToken] = await pongPromise;
        expect(frameToken).toBe(event2[0].senderFrame.frameToken);
      });

      it('should correctly reply to the nested sub-frames with using event.reply', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 3);
        w.loadFile(
          path.resolve(import.meta.dirname, `fixtures/sub-frames/frame-with-frame-container${fixtureSuffix}.html`)
        );
        const [, , event3] = await detailsPromise;
        const pongPromise = once(ipcMain, 'preload-pong');
        event3[0].reply('preload-ping');
        const [, frameToken] = await pongPromise;
        expect(frameToken).toBe(event3[0].senderFrame.frameToken);
      });

      it('should correctly reply to the nested sub-frames with using event.senderFrame.send', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 3);
        w.loadFile(
          path.resolve(import.meta.dirname, `fixtures/sub-frames/frame-with-frame-container${fixtureSuffix}.html`)
        );
        const [, , event3] = await detailsPromise;
        const pongPromise = once(ipcMain, 'preload-pong');
        event3[0].senderFrame.send('preload-ping');
        const [, frameToken] = await pongPromise;
        expect(frameToken).toBe(event3[0].senderFrame.frameToken);
      });

      it('should not expose globals in main world', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 2);
        w.loadFile(path.resolve(import.meta.dirname, `fixtures/sub-frames/frame-container${fixtureSuffix}.html`));
        const details = await detailsPromise;
        const senders = details.map((event) => event[0].sender);
        const isolatedGlobals = await Promise.all(
          senders.map((sender) => sender.executeJavaScript('window.isolatedGlobal'))
        );
        for (const result of isolatedGlobals) {
          if (webPreferences.contextIsolation === undefined || webPreferences.contextIsolation) {
            expect(result).toBeUndefined();
          } else {
            expect(result).toBe(true);
          }
        }
      });
    });
  };

  const generateConfigs = (webPreferences: any, ...permutations: { name: string; webPreferences: any }[]) => {
    const configs = [{ webPreferences, names: [] as string[] }];
    for (const permutation of permutations) {
      const length = configs.length;
      for (let j = 0; j < length; j++) {
        const newConfig = Object.assign({}, configs[j]);
        newConfig.webPreferences = Object.assign({}, newConfig.webPreferences, permutation.webPreferences);
        newConfig.names = newConfig.names.slice(0);
        newConfig.names.push(permutation.name);
        configs.push(newConfig);
      }
    }

    return configs.map((config: any) => {
      if (config.names.length > 0) {
        config.title = `with ${config.names.join(', ')} on`;
      } else {
        config.title = 'without anything special turned on';
      }
      delete config.names;

      return config as { title: string; webPreferences: any };
    });
  };

  const configs = generateConfigs(
    {
      preload: path.resolve(import.meta.dirname, 'fixtures/sub-frames/preload.js'),
      nodeIntegrationInSubFrames: true
    },
    {
      name: 'sandbox',
      webPreferences: { sandbox: true }
    },
    {
      name: 'context isolation disabled',
      webPreferences: { contextIsolation: false }
    },
    {
      name: 'webview',
      webPreferences: { webviewTag: true, preload: false }
    }
  );

  for (const config of configs) {
    generateTests(config.title, config.webPreferences);
  }

  describe('internal <iframe> inside of <webview>', () => {
    let w: BrowserWindow;

    beforeEach(async () => {
      await closeWindow(w);
      w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        webPreferences: {
          preload: path.resolve(import.meta.dirname, 'fixtures/sub-frames/webview-iframe-preload.js'),
          nodeIntegrationInSubFrames: true,
          webviewTag: true,
          contextIsolation: false
        }
      });
    });

    afterEach(async () => {
      await closeWindow(w);
      w = null as unknown as BrowserWindow;
    });

    it('should not load preload scripts', async () => {
      const promisePass = once(ipcMain, 'webview-loaded');
      const promiseFail = once(ipcMain, 'preload-in-frame').then(() => {
        throw new Error('preload loaded in internal frame');
      });
      await w.loadURL('about:blank');
      return Promise.race([promisePass, promiseFail]);
    });
  });
});

describe('renderer allFrames preload scripts', () => {
  for (const sandbox of [false, true]) {
    describe(`with sandbox: ${sandbox}`, () => {
      let w: BrowserWindow;
      let ses: Electron.Session;

      beforeEach(async () => {
        await closeWindow(w);
        ses = session.fromPartition(`all-frames-preload-${sandbox}`);
        const id = ses.registerPreloadScript({
          type: 'frame',
          filePath: path.resolve(import.meta.dirname, 'fixtures/sub-frames/preload.js'),
          allFrames: true
        });
        defer(() => ses.unregisterPreloadScript(id));
        // No nodeIntegrationInSubFrames.
        w = new BrowserWindow({
          show: false,
          width: 400,
          height: 400,
          webPreferences: { sandbox, session: ses }
        });
      });

      afterEach(async () => {
        await closeWindow(w);
        w = null as unknown as BrowserWindow;
      });

      it('loads the preload script in top level iframes', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 2);
        w.loadFile(path.resolve(import.meta.dirname, 'fixtures/sub-frames/frame-container.html'));
        const [event1, event2] = await detailsPromise;
        expect(event1[0].senderFrame.frameToken).not.toBe(event2[0].senderFrame.frameToken);
        expect(event1[0].senderFrame.frameToken).toBe(event1[2]);
        expect(event2[0].senderFrame.frameToken).toBe(event2[2]);
      });

      it('loads the preload script in nested iframes', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 3);
        w.loadFile(path.resolve(import.meta.dirname, 'fixtures/sub-frames/frame-with-frame-container.html'));
        const [event1, event2, event3] = await detailsPromise;
        const tokens = new Set([event1, event2, event3].map((event) => event[0].senderFrame.frameToken));
        expect(tokens.size).toBe(3);
        expect(event3[0].senderFrame.frameToken).toBe(event3[2]);
      });

      it('lets a subframe use ipcRenderer in both directions', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 3);
        w.loadFile(path.resolve(import.meta.dirname, 'fixtures/sub-frames/frame-with-frame-container.html'));
        const details = await detailsPromise;
        const nested = details.find((event) => event[0].senderFrame.parent?.parent);
        expect(nested).toBeDefined();
        const pongPromise = once(ipcMain, 'preload-pong');
        nested![0].reply('preload-ping');
        const [, frameToken] = await pongPromise;
        expect(frameToken).toBe(nested![0].senderFrame.frameToken);
      });

      it('does not run other preload scripts in subframes', async () => {
        const preload = path.resolve(import.meta.dirname, 'fixtures/module/preload-location.js');
        const id = ses.registerPreloadScript({ type: 'frame', filePath: preload });
        defer(() => ses.unregisterPreloadScript(id));
        await closeWindow(w);
        // A separate file: with Node.js integration, require() caches a
        // module by path, so the same file would only run once.
        const webPreferencesPreload = path.resolve(import.meta.dirname, 'fixtures/module/preload-location-2.js');
        w = new BrowserWindow({
          show: false,
          webPreferences: { sandbox, session: ses, preload: webPreferencesPreload }
        });
        const locations: string[] = [];
        ipcMain.on('preload-location', (_e, href: string) => locations.push(href));
        defer(() => ipcMain.removeAllListeners('preload-location'));
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 2);
        await w.loadFile(path.resolve(import.meta.dirname, 'fixtures/sub-frames/frame-container.html'));
        await detailsPromise;
        // The session preload and webPreferences.preload, in the main frame only.
        const mainUrl = w.webContents.getURL();
        expect(locations).toEqual([mainUrl, mainUrl]);
      });
    });
  }
});

describe('subframe with non-standard schemes', () => {
  it('should not crash when changing subframe src to about:blank and back', async () => {
    const w = new BrowserWindow({ show: false, width: 400, height: 400 });

    const fwfPath = path.resolve(import.meta.dirname, 'fixtures/sub-frames/frame-with-frame.html');
    await w.loadFile(fwfPath);

    const originalSrc = await w.webContents.executeJavaScript(`
      const iframe = document.querySelector('iframe');
      iframe.src;
    `);

    const updatedSrc = await w.webContents.executeJavaScript(`
      new Promise((resolve, reject) => {
        const iframe = document.querySelector('iframe');
        iframe.src = 'about:blank';
        resolve(iframe.src);
      })
    `);

    expect(updatedSrc).toBe('about:blank');

    const restoredSrc = await w.webContents.executeJavaScript(`
      new Promise((resolve, reject) => {
        const iframe = document.querySelector('iframe');
        iframe.src = '${originalSrc}';
        resolve(iframe.src);
      })
    `);

    expect(restoredSrc).toBe(originalSrc);
  });
});

// app.getAppMetrics() does not return sandbox information on Linux.
ifdescribe(process.platform !== 'linux')('cross-site frame sandboxing', () => {
  let server: http.Server;
  let crossSiteUrl: string;
  let serverUrl: string;

  beforeAll(async () => {
    server = http.createServer((req, res) => {
      res.end(`<iframe name="frame" src="${crossSiteUrl}" />`);
    });
    serverUrl = (await listen(server)).url;
    crossSiteUrl = serverUrl.replace('127.0.0.1', 'localhost');
  });

  afterAll(() => {
    server.close();
    server = null as unknown as http.Server;
  });

  let w: BrowserWindow;

  afterEach(async () => {
    await closeWindow(w);
    w = null as unknown as BrowserWindow;
  });

  const generateSpecs = (description: string, webPreferences: any) => {
    describe(description, () => {
      it('iframe process is sandboxed if possible', async () => {
        w = new BrowserWindow({
          show: false,
          webPreferences
        });

        await w.loadURL(serverUrl);

        const pidMain = w.webContents.getOSProcessId();
        const pidFrame = w.webContents.mainFrame.frames.find((f) => f.name === 'frame')!.osProcessId;

        const metrics = app.getAppMetrics();
        const isProcessSandboxed = function (pid: number) {
          const entry = metrics.find((metric) => metric.pid === pid);
          return entry && entry.sandboxed;
        };

        const sandboxMain = !!(webPreferences.sandbox || process.mas);
        const sandboxFrame = sandboxMain || !webPreferences.nodeIntegrationInSubFrames;

        expect(isProcessSandboxed(pidMain)).toBe(sandboxMain);
        expect(isProcessSandboxed(pidFrame)).toBe(sandboxFrame);
      });
    });
  };

  it('keeps a cross-site iframe sandboxed when it runs an allFrames preload', async () => {
    const ses = session.fromPartition('cross-site-all-frames');
    const id = ses.registerPreloadScript({
      type: 'frame',
      filePath: path.resolve(import.meta.dirname, 'fixtures/sub-frames/all-frames-sandbox-preload.js'),
      matches: [`${new URL(crossSiteUrl).origin}/*`],
      allFrames: true
    });
    defer(() => ses.unregisterPreloadScript(id));
    w = new BrowserWindow({ show: false, webPreferences: { sandbox: false, session: ses } });

    const preloadRan = once(ipcMain, 'all-frames-preload');
    await w.loadURL(serverUrl);
    const [event, href, sandboxed, canRequireFs] = await preloadRan;

    // The preload ran in the cross-site frame, in the sandboxed preload
    // environment.
    const frame = w.webContents.mainFrame.frames.find((f) => f.name === 'frame')!;
    expect(event.senderFrame.frameToken).toBe(frame.frameToken);
    expect(href).toBe(new URL(crossSiteUrl).href);
    expect(sandboxed).toBe(true);
    expect(canRequireFs).toBe(false);

    const metrics = app.getAppMetrics();
    const entry = metrics.find((metric) => metric.pid === frame.osProcessId);
    expect(entry && entry.sandboxed).toBe(true);
  });

  generateSpecs('nodeIntegrationInSubFrames = false, sandbox = false', {
    nodeIntegrationInSubFrames: false,
    sandbox: false
  });

  generateSpecs('nodeIntegrationInSubFrames = false, sandbox = true', {
    nodeIntegrationInSubFrames: false,
    sandbox: true
  });

  generateSpecs('nodeIntegrationInSubFrames = true, sandbox = false', {
    nodeIntegrationInSubFrames: true,
    sandbox: false
  });

  generateSpecs('nodeIntegrationInSubFrames = true, sandbox = true', {
    nodeIntegrationInSubFrames: true,
    sandbox: true
  });
});

import { app, BrowserWindow, ipcMain } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';
import * as http from 'node:http';
import * as path from 'node:path';

import { emittedNTimes, emittedUntil } from './lib/events-helpers';
import { ifdescribe, listen } from './lib/spec-helpers';
import { closeWindow } from './lib/window-helpers';

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
        w.loadFile(path.resolve(__dirname, `fixtures/sub-frames/frame-container${fixtureSuffix}.html`));
        const [event1, event2] = await detailsPromise;
        expect(event1[0].senderFrame.frameToken).to.not.equal(event2[0].senderFrame.frameToken);
        expect(event1[0].senderFrame.frameToken).to.equal(event1[2]);
        expect(event2[0].senderFrame.frameToken).to.equal(event2[2]);
      });

      it('should load preload scripts in nested iframes', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 3);
        w.loadFile(path.resolve(__dirname, `fixtures/sub-frames/frame-with-frame-container${fixtureSuffix}.html`));
        const [event1, event2, event3] = await detailsPromise;
        expect(event1[0].senderFrame.frameToken).to.not.equal(event2[0].senderFrame.frameToken);
        expect(event1[0].senderFrame.frameToken).to.not.equal(event3[0].senderFrame.frameToken);
        expect(event2[0].senderFrame.frameToken).to.not.equal(event3[0].senderFrame.frameToken);
        expect(event1[0].senderFrame.frameToken).to.equal(event1[2]);
        expect(event2[0].senderFrame.frameToken).to.equal(event2[2]);
        expect(event3[0].senderFrame.frameToken).to.equal(event3[2]);
      });

      it('should correctly reply to the main frame with using event.reply', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 2);
        w.loadFile(path.resolve(__dirname, `fixtures/sub-frames/frame-container${fixtureSuffix}.html`));
        const [event1] = await detailsPromise;
        const pongPromise = once(ipcMain, 'preload-pong');
        event1[0].reply('preload-ping');
        const [, frameToken] = await pongPromise;
        expect(frameToken).to.equal(event1[0].senderFrame.frameToken);
      });

      it('should correctly reply to the main frame with using event.senderFrame.send', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 2);
        w.loadFile(path.resolve(__dirname, `fixtures/sub-frames/frame-container${fixtureSuffix}.html`));
        const [event1] = await detailsPromise;
        const pongPromise = once(ipcMain, 'preload-pong');
        event1[0].senderFrame.send('preload-ping');
        const [, frameToken] = await pongPromise;
        expect(frameToken).to.equal(event1[0].senderFrame.frameToken);
      });

      it('should correctly reply to the sub-frames with using event.reply', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 2);
        w.loadFile(path.resolve(__dirname, `fixtures/sub-frames/frame-container${fixtureSuffix}.html`));
        const [, event2] = await detailsPromise;
        const pongPromise = once(ipcMain, 'preload-pong');
        event2[0].reply('preload-ping');
        const [, frameToken] = await pongPromise;
        expect(frameToken).to.equal(event2[0].senderFrame.frameToken);
      });

      it('should correctly reply to the sub-frames with using event.senderFrame.send', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 2);
        w.loadFile(path.resolve(__dirname, `fixtures/sub-frames/frame-container${fixtureSuffix}.html`));
        const [, event2] = await detailsPromise;
        const pongPromise = once(ipcMain, 'preload-pong');
        event2[0].senderFrame.send('preload-ping');
        const [, frameToken] = await pongPromise;
        expect(frameToken).to.equal(event2[0].senderFrame.frameToken);
      });

      it('should correctly reply to the nested sub-frames with using event.reply', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 3);
        w.loadFile(path.resolve(__dirname, `fixtures/sub-frames/frame-with-frame-container${fixtureSuffix}.html`));
        const [, , event3] = await detailsPromise;
        const pongPromise = once(ipcMain, 'preload-pong');
        event3[0].reply('preload-ping');
        const [, frameToken] = await pongPromise;
        expect(frameToken).to.equal(event3[0].senderFrame.frameToken);
      });

      it('should correctly reply to the nested sub-frames with using event.senderFrame.send', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 3);
        w.loadFile(path.resolve(__dirname, `fixtures/sub-frames/frame-with-frame-container${fixtureSuffix}.html`));
        const [, , event3] = await detailsPromise;
        const pongPromise = once(ipcMain, 'preload-pong');
        event3[0].senderFrame.send('preload-ping');
        const [, frameToken] = await pongPromise;
        expect(frameToken).to.equal(event3[0].senderFrame.frameToken);
      });

      it('should not expose globals in main world', async () => {
        const detailsPromise = emittedNTimes(ipcMain, 'preload-ran', 2);
        w.loadFile(path.resolve(__dirname, `fixtures/sub-frames/frame-container${fixtureSuffix}.html`));
        const details = await detailsPromise;
        const senders = details.map((event) => event[0].sender);
        const isolatedGlobals = await Promise.all(
          senders.map((sender) => sender.executeJavaScript('window.isolatedGlobal'))
        );
        for (const result of isolatedGlobals) {
          if (webPreferences.contextIsolation === undefined || webPreferences.contextIsolation) {
            expect(result).to.be.undefined();
          } else {
            expect(result).to.equal(true);
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
      preload: path.resolve(__dirname, 'fixtures/sub-frames/preload.js'),
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
          preload: path.resolve(__dirname, 'fixtures/sub-frames/webview-iframe-preload.js'),
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

// Blink reuses the V8 context of a frame's initial empty document for the
// document that commits into the frame, skipping the LocalWindowProxy
// initialization that Electron installs the preload from.
describe('renderer nodeIntegrationInSubFrames context reuse', () => {
  const fixturesDir = path.resolve(__dirname, 'fixtures/sub-frames');

  let w: BrowserWindow;

  afterEach(async () => {
    await closeWindow(w);
    w = null as unknown as BrowserWindow;
  });

  for (const contextIsolation of [true, false]) {
    it(`runs the preload once, before the first page script (contextIsolation: ${contextIsolation})`, async () => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          preload: path.resolve(__dirname, 'fixtures/context-reuse-preload.js'),
          nodeIntegrationInSubFrames: true,
          contextIsolation
        }
      });

      const preloadRuns: string[] = [];
      const onPreloadRan = (_event: Electron.IpcMainEvent, href: string) => preloadRuns.push(href);
      ipcMain.on('context-reuse-preload-ran', onPreloadRan);
      try {
        const childLoaded = emittedUntil(
          w.webContents,
          'did-frame-finish-load',
          (_e: Electron.Event, isMainFrame: boolean) => !isMainFrame
        );
        await w.loadFile(path.join(fixturesDir, 'frame-container-context-reuse.html'));
        await childLoaded;

        const frame = w.webContents.mainFrame.frames.find((f) => f.name === 'frameA')!;
        const [reusedMarker, preloadMarkerType] = (await frame.executeJavaScript(
          '[window.reusedMarkerAtFirstScript, window.preloadMarkerAtFirstScript]'
        )) as [string, string];
        expect(reusedMarker).to.equal('yes', 'the initial empty document context should have been reused');
        expect(preloadMarkerType).to.equal('string', 'preload should run before the first page script');
        expect(preloadRuns.filter((href) => href.endsWith('frame-context-reuse.html'))).to.have.lengthOf(1);
      } finally {
        ipcMain.removeListener('context-reuse-preload-ran', onPreloadRan);
      }
    });
  }
});

describe('subframe with non-standard schemes', () => {
  it('should not crash when changing subframe src to about:blank and back', async () => {
    const w = new BrowserWindow({ show: false, width: 400, height: 400 });

    const fwfPath = path.resolve(__dirname, 'fixtures/sub-frames/frame-with-frame.html');
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

    expect(updatedSrc).to.equal('about:blank');

    const restoredSrc = await w.webContents.executeJavaScript(`
      new Promise((resolve, reject) => {
        const iframe = document.querySelector('iframe');
        iframe.src = '${originalSrc}';
        resolve(iframe.src);
      })
    `);

    expect(restoredSrc).to.equal(originalSrc);
  });
});

// app.getAppMetrics() does not return sandbox information on Linux.
ifdescribe(process.platform !== 'linux')('cross-site frame sandboxing', () => {
  let server: http.Server;
  let crossSiteUrl: string;
  let serverUrl: string;

  before(async function () {
    server = http.createServer((req, res) => {
      res.end(`<iframe name="frame" src="${crossSiteUrl}" />`);
    });
    serverUrl = (await listen(server)).url;
    crossSiteUrl = serverUrl.replace('127.0.0.1', 'localhost');
  });

  after(() => {
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

        expect(isProcessSandboxed(pidMain)).to.equal(sandboxMain);
        expect(isProcessSandboxed(pidFrame)).to.equal(sandboxFrame);
      });
    });
  };

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

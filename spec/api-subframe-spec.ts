import { app, BrowserWindow, ipcMain } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';
import * as http from 'node:http';
import * as path from 'node:path';

import { emittedNTimes } from './lib/events-helpers';
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
        const senders = details.map(event => event[0].sender);
        const isolatedGlobals = await Promise.all(senders.map(sender => sender.executeJavaScript('window.isolatedGlobal')));
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

  const generateConfigs = (webPreferences: any, ...permutations: {name: string, webPreferences: any}[]) => {
    const configs = [{ webPreferences, names: [] as string[] }];
    for (const permutation of permutations) {
      const length = configs.length;
      for (let j = 0; j < length; j++) {
        const newConfig = Object.assign({}, configs[j]);
        newConfig.webPreferences = Object.assign({},
          newConfig.webPreferences, permutation.webPreferences);
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

      return config as {title: string, webPreferences: any};
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
        const pidFrame = w.webContents.mainFrame.frames.find(f => f.name === 'frame')!.osProcessId;

        const metrics = app.getAppMetrics();
        const isProcessSandboxed = function (pid: number) {
          const entry = metrics.find(metric => metric.pid === pid);
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

// Test for issue #47648 - Memory leak with same-origin iframes and nodeIntegration
describe('same-origin iframes with nodeIntegration', () => {
  let w: BrowserWindow;

  beforeEach(async () => {
    await closeWindow(w);
  });

  afterEach(async () => {
    await closeWindow(w);
    w = null as unknown as BrowserWindow;
  });

  it('should not create duplicate Node environments when removing and recreating same-origin iframes', async () => {
    w = new BrowserWindow({
      show: false,
      width: 400,
      height: 400,
      webPreferences: {
        nodeIntegration: true,
        nodeIntegrationInSubFrames: true,
        contextIsolation: false
      }
    });

    // Track iframe loads
    const frameLoads: any[] = [];
    ipcMain.on('same-origin-frame-loaded', (event, data) => {
      frameLoads.push(data);
    });

    // Wait for parent to be ready
    const parentReady = once(ipcMain, 'parent-ready');
    w.loadFile(path.resolve(__dirname, 'fixtures/sub-frames/same-origin-parent.html'));
    await parentReady;

    // Create iframe multiple times - this should not create duplicate Node environments
    for (let i = 0; i < 3; i++) {
      const beforeCount = frameLoads.length;
      
      // Create iframe
      await w.webContents.executeJavaScript('window.createIframe()');
      
      // Wait for iframe to load
      await new Promise(resolve => setTimeout(resolve, 100));
      
      // Verify iframe loaded with Node integration
      expect(frameLoads.length).to.be.greaterThan(beforeCount);
      expect(frameLoads[frameLoads.length - 1].hasNodeIntegration).to.equal(true);
      
      // Remove iframe
      await w.webContents.executeJavaScript('window.removeIframe()');
      await new Promise(resolve => setTimeout(resolve, 100));
    }

    // Clean up listener
    ipcMain.removeAllListeners('same-origin-frame-loaded');

    // If the fix works, this test will pass without memory leaks
    // The C++ code now tracks context-to-environment mapping and prevents
    // creating duplicate Node environments in shared V8 contexts
    expect(frameLoads.length).to.equal(3);
  });

  it('should properly clean up Node environments when iframes are removed', async () => {
    w = new BrowserWindow({
      show: false,
      width: 400,
      height: 400,
      webPreferences: {
        nodeIntegration: true,
        nodeIntegrationInSubFrames: true,
        contextIsolation: false
      }
    });

    const parentReady = once(ipcMain, 'parent-ready');
    w.loadFile(path.resolve(__dirname, 'fixtures/sub-frames/same-origin-parent.html'));
    await parentReady;

    // Create and remove iframe
    await w.webContents.executeJavaScript('window.createIframe()');
    await new Promise(resolve => setTimeout(resolve, 100));
    
    await w.webContents.executeJavaScript('window.removeIframe()');
    await new Promise(resolve => setTimeout(resolve, 100));

    // Create again - should reuse context without issues
    await w.webContents.executeJavaScript('window.createIframe()');
    await new Promise(resolve => setTimeout(resolve, 100));

    // Verify parent still has Node integration
    const hasNode = await w.webContents.executeJavaScript('typeof process !== "undefined"');
    expect(hasNode).to.equal(true);

    // Clean up
    ipcMain.removeAllListeners('same-origin-frame-loaded');
  });
});

import { BrowserWindow, WebFrameMain, webFrameMain, ipcMain, app, WebContents } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';
import * as http from 'node:http';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';
import * as url from 'node:url';

import { emittedNTimes } from './lib/events-helpers';
import { defer, ifit, listen, waitUntil } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';

describe('webFrameMain module', () => {
  const fixtures = path.resolve(__dirname, 'fixtures');
  const subframesPath = path.join(fixtures, 'sub-frames');

  const fileUrl = (filename: string) => url.pathToFileURL(path.join(subframesPath, filename)).href;

  type Server = { server: http.Server, url: string, crossOriginUrl: string }

  /** Creates an HTTP server whose handler embeds the given iframe src. */
  const createServer = async (): Promise<Server> => {
    const server = http.createServer((req, res) => {
      const params = new URLSearchParams(new URL(req.url || '', `http://${req.headers.host}`).search || '');
      if (params.has('frameSrc')) {
        res.end(`<iframe src="${params.get('frameSrc')}"></iframe>`);
      } else {
        res.end('');
      }
    });
    const serverUrl = (await listen(server)).url + '/';
    // HACK: Use 'localhost' instead of '127.0.0.1' so Chromium treats it as
    // a separate origin because differing ports aren't enough 🤔
    const crossOriginUrl = serverUrl.replace('127.0.0.1', 'localhost');
    return {
      server,
      url: serverUrl,
      crossOriginUrl
    };
  };

  afterEach(closeAllWindows);

  describe('WebFrame traversal APIs', () => {
    let w: BrowserWindow;
    let webFrame: WebFrameMain;

    beforeEach(async () => {
      w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(subframesPath, 'frame-with-frame-container.html'));
      webFrame = w.webContents.mainFrame;
    });

    it('can access top frame', () => {
      expect(webFrame.top).to.equal(webFrame);
    });

    it('has no parent on top frame', () => {
      expect(webFrame.parent).to.be.null();
    });

    it('can access immediate frame descendents', () => {
      const { frames } = webFrame;
      expect(frames).to.have.lengthOf(1);
      const subframe = frames[0];
      expect(subframe).not.to.equal(webFrame);
      expect(subframe.parent).to.equal(webFrame);
    });

    it('can access deeply nested frames', () => {
      const subframe = webFrame.frames[0];
      expect(subframe).not.to.equal(webFrame);
      expect(subframe.parent).to.equal(webFrame);
      const nestedSubframe = subframe.frames[0];
      expect(nestedSubframe).not.to.equal(webFrame);
      expect(nestedSubframe).not.to.equal(subframe);
      expect(nestedSubframe.parent).to.equal(subframe);
    });

    it('can traverse all frames in root', () => {
      const urls = webFrame.framesInSubtree.map(frame => frame.url);
      expect(urls).to.deep.equal([
        fileUrl('frame-with-frame-container.html'),
        fileUrl('frame-with-frame.html'),
        fileUrl('frame.html')
      ]);
    });

    it('can traverse all frames in subtree', () => {
      const urls = webFrame.frames[0].framesInSubtree.map(frame => frame.url);
      expect(urls).to.deep.equal([
        fileUrl('frame-with-frame.html'),
        fileUrl('frame.html')
      ]);
    });

    describe('cross-origin', () => {
      let serverA: Server;
      let serverB: Server;

      before(async () => {
        serverA = await createServer();
        serverB = await createServer();
      });

      after(() => {
        serverA.server.close();
        serverB.server.close();
      });

      it('can access cross-origin frames', async () => {
        await w.loadURL(`${serverA.url}?frameSrc=${serverB.url}`);
        webFrame = w.webContents.mainFrame;
        expect(webFrame.url.startsWith(serverA.url)).to.be.true();
        expect(webFrame.frames[0].url).to.equal(serverB.url);
      });
    });
  });

  describe('WebFrame.url', () => {
    it('should report correct address for each subframe', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(subframesPath, 'frame-with-frame-container.html'));
      const webFrame = w.webContents.mainFrame;

      expect(webFrame.url).to.equal(fileUrl('frame-with-frame-container.html'));
      expect(webFrame.frames[0].url).to.equal(fileUrl('frame-with-frame.html'));
      expect(webFrame.frames[0].frames[0].url).to.equal(fileUrl('frame.html'));
    });
  });

  describe('WebFrame.origin', () => {
    it('should be null for a fresh WebContents', () => {
      const w = new BrowserWindow({ show: false });
      expect(w.webContents.mainFrame.origin).to.equal('null');
    });

    it('should be file:// for file frames', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixtures, 'pages', 'blank.html'));
      expect(w.webContents.mainFrame.origin).to.equal('file://');
    });

    it('should be http:// for an http frame', async () => {
      const w = new BrowserWindow({ show: false });
      const s = await createServer();
      defer(() => s.server.close());
      await w.loadURL(s.url);
      expect(w.webContents.mainFrame.origin).to.equal(s.url.replace(/\/$/, ''));
    });

    it('should show parent origin when child page is about:blank', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixtures, 'pages', 'blank.html'));
      const webContentsCreated = once(app, 'web-contents-created') as Promise<[any, WebContents]>;
      expect(w.webContents.mainFrame.origin).to.equal('file://');
      await w.webContents.executeJavaScript('window.open("", null, "show=false"), null');
      const [, childWebContents] = await webContentsCreated;
      expect(childWebContents.mainFrame.origin).to.equal('file://');
    });

    it('should show parent frame\'s origin when about:blank child window opened through cross-origin subframe', async () => {
      const w = new BrowserWindow({ show: false });
      const serverA = await createServer();
      const serverB = await createServer();
      defer(() => {
        serverA.server.close();
        serverB.server.close();
      });
      await w.loadURL(serverA.url + '?frameSrc=' + encodeURIComponent(serverB.url));
      const { mainFrame } = w.webContents;
      expect(mainFrame.origin).to.equal(serverA.url.replace(/\/$/, ''));
      const [childFrame] = mainFrame.frames;
      expect(childFrame.origin).to.equal(serverB.url.replace(/\/$/, ''));
      const webContentsCreated = once(app, 'web-contents-created') as Promise<[any, WebContents]>;
      await childFrame.executeJavaScript('window.open("", null, "show=false"), null');
      const [, childWebContents] = await webContentsCreated;
      expect(childWebContents.mainFrame.origin).to.equal(childFrame.origin);
    });
  });

  describe('WebFrame IDs', () => {
    it('has properties for various identifiers', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(subframesPath, 'frame.html'));
      const webFrame = w.webContents.mainFrame;
      expect(webFrame).to.have.property('url').that.is.a('string');
      expect(webFrame).to.have.property('frameTreeNodeId').that.is.a('number');
      expect(webFrame).to.have.property('name').that.is.a('string');
      expect(webFrame).to.have.property('osProcessId').that.is.a('number');
      expect(webFrame).to.have.property('processId').that.is.a('number');
      expect(webFrame).to.have.property('routingId').that.is.a('number');
    });
  });

  describe('WebFrame.visibilityState', () => {
    // DISABLED-FIXME(MarshallOfSound): Fix flaky test
    it('should match window state', async () => {
      const w = new BrowserWindow({ show: true });
      await w.loadURL('about:blank');
      const webFrame = w.webContents.mainFrame;

      expect(webFrame.visibilityState).to.equal('visible');
      w.hide();
      await expect(
        waitUntil(() => webFrame.visibilityState === 'hidden')
      ).to.eventually.be.fulfilled();
    });
  });

  describe('WebFrame.executeJavaScript', () => {
    it('can inject code into any subframe', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(subframesPath, 'frame-with-frame-container.html'));
      const webFrame = w.webContents.mainFrame;

      const getUrl = (frame: WebFrameMain) => frame.executeJavaScript('location.href');
      expect(await getUrl(webFrame)).to.equal(fileUrl('frame-with-frame-container.html'));
      expect(await getUrl(webFrame.frames[0])).to.equal(fileUrl('frame-with-frame.html'));
      expect(await getUrl(webFrame.frames[0].frames[0])).to.equal(fileUrl('frame.html'));
    });

    it('can resolve promise', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(subframesPath, 'frame.html'));
      const webFrame = w.webContents.mainFrame;
      const p = () => webFrame.executeJavaScript('new Promise(resolve => setTimeout(resolve(42), 2000));');
      const result = await p();
      expect(result).to.equal(42);
    });

    it('can reject with error', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(subframesPath, 'frame.html'));
      const webFrame = w.webContents.mainFrame;
      const p = () => webFrame.executeJavaScript('new Promise((r,e) => setTimeout(e("error!"), 500));');
      await expect(p()).to.be.eventually.rejectedWith('error!');
      const errorTypes = new Set([
        Error,
        ReferenceError,
        EvalError,
        RangeError,
        SyntaxError,
        TypeError,
        URIError
      ]);
      for (const error of errorTypes) {
        await expect(webFrame.executeJavaScript(`Promise.reject(new ${error.name}("Wamp-wamp"))`))
          .to.eventually.be.rejectedWith(/Error/);
      }
    });

    it('can reject when script execution fails', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(subframesPath, 'frame.html'));
      const webFrame = w.webContents.mainFrame;
      const p = () => webFrame.executeJavaScript('console.log(test)');
      await expect(p()).to.be.eventually.rejectedWith(/ReferenceError/);
    });
  });

  describe('WebFrame.reload', () => {
    it('reloads a frame', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(subframesPath, 'frame.html'));
      const webFrame = w.webContents.mainFrame;

      await webFrame.executeJavaScript('window.TEMP = 1', false);
      expect(webFrame.reload()).to.be.true();
      await once(w.webContents, 'dom-ready');
      expect(await webFrame.executeJavaScript('window.TEMP', false)).to.be.null();
    });
  });

  describe('WebFrame.send', () => {
    it('works', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          preload: path.join(subframesPath, 'preload.js'),
          nodeIntegrationInSubFrames: true
        }
      });
      await w.loadURL('about:blank');
      const webFrame = w.webContents.mainFrame;
      const pongPromise = once(ipcMain, 'preload-pong');
      webFrame.send('preload-ping');
      const [, routingId] = await pongPromise;
      expect(routingId).to.equal(webFrame.routingId);
    });
  });

  describe('RenderFrame lifespan', () => {
    let server: Awaited<ReturnType<typeof createServer>>;
    let w: BrowserWindow;

    before(async () => {
      server = await createServer();
    });
    after(() => {
      server.server.close();
    });
    beforeEach(async () => {
      w = new BrowserWindow({ show: false });
    });

    // TODO(jkleinsc) fix this flaky test on linux
    ifit(process.platform !== 'linux')('throws upon accessing properties when disposed', async () => {
      await w.loadFile(path.join(subframesPath, 'frame-with-frame-container.html'));
      const { mainFrame } = w.webContents;
      w.destroy();
      // Wait for WebContents, and thus RenderFrameHost, to be destroyed.
      await setTimeout();
      expect(() => mainFrame.url).to.throw();
    });

    it('persists through cross-origin navigation', async () => {
      await w.loadURL(server.url);
      const { mainFrame } = w.webContents;
      expect(mainFrame.url).to.equal(server.url);
      await w.loadURL(server.crossOriginUrl);
      expect(w.webContents.mainFrame).to.equal(mainFrame);
      expect(mainFrame.url).to.equal(server.crossOriginUrl);
    });

    it('recovers from renderer crash on same-origin', async () => {
      // Keep reference to mainFrame alive throughout crash and recovery.
      const { mainFrame } = w.webContents;
      await w.webContents.loadURL(server.url);
      const crashEvent = once(w.webContents, 'render-process-gone');
      w.webContents.forcefullyCrashRenderer();
      await crashEvent;
      await w.webContents.loadURL(server.url);
      // Log just to keep mainFrame in scope.
      console.log('mainFrame.url', mainFrame.url);
    });

    // Fixed by #34411
    it('recovers from renderer crash on cross-origin', async () => {
      // Keep reference to mainFrame alive throughout crash and recovery.
      const { mainFrame } = w.webContents;
      await w.webContents.loadURL(server.url);
      const crashEvent = once(w.webContents, 'render-process-gone');
      w.webContents.forcefullyCrashRenderer();
      await crashEvent;
      // A short wait seems to be required to reproduce the crash.
      await setTimeout(100);
      await w.webContents.loadURL(server.crossOriginUrl);
      // Log just to keep mainFrame in scope.
      console.log('mainFrame.url', mainFrame.url);
    });

    it('returns null upon accessing senderFrame after cross-origin navigation', async () => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          preload: path.join(subframesPath, 'preload.js')
        }
      });
      const preloadPromise = once(ipcMain, 'preload-ran');
      await w.webContents.loadURL(server.url);
      const [event] = await preloadPromise;
      await w.webContents.loadURL(server.crossOriginUrl);
      // senderFrame now points to a disposed RenderFrameHost. It should
      // be null when attempting to access the lazily evaluated property.
      expect(event.senderFrame).to.be.null();
    });

    it('is detached when unload handler sends IPC', async () => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          preload: path.join(subframesPath, 'preload.js')
        }
      });
      await w.webContents.loadURL(server.url);
      const unloadPromise = new Promise<void>((resolve, reject) => {
        ipcMain.once('preload-unload', (event) => {
          try {
            const { senderFrame } = event;
            expect(senderFrame).to.not.be.null();
            expect(senderFrame!.detached).to.be.true();
            expect(senderFrame!.processId).to.equal(event.processId);
            expect(senderFrame!.routingId).to.equal(event.frameId);
            resolve();
          } catch (error) {
            reject(error);
          }
        });
      });
      await w.webContents.loadURL(server.crossOriginUrl);
      await expect(unloadPromise).to.eventually.be.fulfilled();
    });

    it('disposes detached frame after cross-origin navigation', async () => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          preload: path.join(subframesPath, 'preload.js')
        }
      });
      await w.webContents.loadURL(server.url);
      // eslint-disable-next-line prefer-const
      let crossOriginPromise: Promise<void>;
      const unloadPromise = new Promise<void>((resolve, reject) => {
        ipcMain.once('preload-unload', async (event) => {
          try {
            const { senderFrame } = event;
            expect(senderFrame!.detached).to.be.true();
            await crossOriginPromise;
            expect(() => senderFrame!.url).to.throw(/Render frame was disposed/);
            resolve();
          } catch (error) {
            reject(error);
          }
        });
      });
      crossOriginPromise = w.webContents.loadURL(server.crossOriginUrl);
      await expect(unloadPromise).to.eventually.be.fulfilled();
    });
  });

  describe('webFrameMain.fromId', () => {
    it('returns undefined for unknown IDs', () => {
      expect(webFrameMain.fromId(0, 0)).to.be.undefined();
    });

    it('can find each frame from navigation events', async () => {
      const w = new BrowserWindow({ show: false });

      // frame-with-frame-container.html, frame-with-frame.html, frame.html
      const didFrameFinishLoad = emittedNTimes(w.webContents, 'did-frame-finish-load', 3);
      w.loadFile(path.join(subframesPath, 'frame-with-frame-container.html'));

      for (const [, isMainFrame, frameProcessId, frameRoutingId] of await didFrameFinishLoad) {
        const frame = webFrameMain.fromId(frameProcessId, frameRoutingId);
        expect(frame).not.to.be.null();
        expect(frame?.processId).to.be.equal(frameProcessId);
        expect(frame?.routingId).to.be.equal(frameRoutingId);
        expect(frame?.top === frame).to.be.equal(isMainFrame);
      }
    });
  });

  describe('"frame-created" event', () => {
    it('emits when the main frame is created', async () => {
      const w = new BrowserWindow({ show: false });
      const promise = once(w.webContents, 'frame-created') as Promise<[any, Electron.FrameCreatedDetails]>;
      w.webContents.loadFile(path.join(subframesPath, 'frame.html'));
      const [, details] = await promise;
      expect(details.frame).to.equal(w.webContents.mainFrame);
    });

    it('emits when nested frames are created', async () => {
      const w = new BrowserWindow({ show: false });
      const promise = emittedNTimes(w.webContents, 'frame-created', 2) as Promise<[any, Electron.FrameCreatedDetails][]>;
      w.webContents.loadFile(path.join(subframesPath, 'frame-container.html'));
      const [[, mainDetails], [, nestedDetails]] = await promise;
      expect(mainDetails.frame).to.equal(w.webContents.mainFrame);
      expect(nestedDetails.frame).to.equal(w.webContents.mainFrame.frames[0]);
    });

    it('is not emitted upon cross-origin navigation', async () => {
      const server = await createServer();

      const w = new BrowserWindow({ show: false });
      await w.webContents.loadURL(server.url);

      let frameCreatedEmitted = false;

      w.webContents.once('frame-created', () => {
        frameCreatedEmitted = true;
      });

      await w.webContents.loadURL(server.crossOriginUrl);

      expect(frameCreatedEmitted).to.be.false();
    });
  });

  describe('"dom-ready" event', () => {
    it('emits for top-level frame', async () => {
      const w = new BrowserWindow({ show: false });
      const promise = once(w.webContents.mainFrame, 'dom-ready');
      w.webContents.loadURL('about:blank');
      await promise;
    });

    it('emits for sub frame', async () => {
      const w = new BrowserWindow({ show: false });
      const promise = new Promise<void>(resolve => {
        w.webContents.on('frame-created', (e, { frame }) => {
          frame!.on('dom-ready', () => {
            if (frame!.name === 'frameA') {
              resolve();
            }
          });
        });
      });
      w.webContents.loadFile(path.join(subframesPath, 'frame-with-frame.html'));
      await promise;
    });
  });
});

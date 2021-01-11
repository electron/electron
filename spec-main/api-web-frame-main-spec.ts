import { expect } from 'chai';
import * as http from 'http';
import * as path from 'path';
import * as url from 'url';
import { BrowserWindow, WebFrameMain, webFrameMain } from 'electron/main';
import { closeAllWindows } from './window-helpers';
import { emittedOnce, emittedNTimes } from './events-helpers';
import { AddressInfo } from 'net';

describe('webFrameMain module', () => {
  const fixtures = path.resolve(__dirname, '..', 'spec-main', 'fixtures');
  const subframesPath = path.join(fixtures, 'sub-frames');

  const fileUrl = (filename: string) => url.pathToFileURL(path.join(subframesPath, filename)).href;

  afterEach(closeAllWindows);

  describe('WebFrame traversal APIs', () => {
    let w: BrowserWindow;
    let webFrame: WebFrameMain;

    beforeEach(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
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
      type Server = { server: http.Server, url: string }

      /** Creates an HTTP server whose handler embeds the given iframe src. */
      const createServer = () => new Promise<Server>(resolve => {
        const server = http.createServer((req, res) => {
          const params = new URLSearchParams(url.parse(req.url || '').search || '');
          if (params.has('frameSrc')) {
            res.end(`<iframe src="${params.get('frameSrc')}"></iframe>`);
          } else {
            res.end('');
          }
        });
        server.listen(0, '127.0.0.1', () => {
          const url = `http://127.0.0.1:${(server.address() as AddressInfo).port}/`;
          resolve({ server, url });
        });
      });

      let serverA = null as unknown as Server;
      let serverB = null as unknown as Server;

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
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
      await w.loadFile(path.join(subframesPath, 'frame-with-frame-container.html'));
      const webFrame = w.webContents.mainFrame;

      expect(webFrame.url).to.equal(fileUrl('frame-with-frame-container.html'));
      expect(webFrame.frames[0].url).to.equal(fileUrl('frame-with-frame.html'));
      expect(webFrame.frames[0].frames[0].url).to.equal(fileUrl('frame.html'));
    });
  });

  describe('WebFrame IDs', () => {
    it('has properties for various identifiers', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
      await w.loadFile(path.join(subframesPath, 'frame.html'));
      const webFrame = w.webContents.mainFrame;
      expect(webFrame).to.haveOwnProperty('frameTreeNodeId');
      expect(webFrame).to.haveOwnProperty('name');
      expect(webFrame).to.haveOwnProperty('osProcessId');
      expect(webFrame).to.haveOwnProperty('processId');
      expect(webFrame).to.haveOwnProperty('routingId');
    });
  });

  describe('WebFrame.executeJavaScript', () => {
    it('can inject code into any subframe', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
      await w.loadFile(path.join(subframesPath, 'frame-with-frame-container.html'));
      const webFrame = w.webContents.mainFrame;

      const getUrl = (frame: WebFrameMain) => frame.executeJavaScript('location.href');
      expect(await getUrl(webFrame)).to.equal(fileUrl('frame-with-frame-container.html'));
      expect(await getUrl(webFrame.frames[0])).to.equal(fileUrl('frame-with-frame.html'));
      expect(await getUrl(webFrame.frames[0].frames[0])).to.equal(fileUrl('frame.html'));
    });
  });

  describe('WebFrame.executeJavaScriptInIsolatedWorld', () => {
    it('can inject code into any subframe', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
      await w.loadFile(path.join(subframesPath, 'frame-with-frame-container.html'));
      const webFrame = w.webContents.mainFrame;

      const getUrl = (frame: WebFrameMain) => frame.executeJavaScriptInIsolatedWorld(999, 'location.href');
      expect(await getUrl(webFrame)).to.equal(fileUrl('frame-with-frame-container.html'));
      expect(await getUrl(webFrame.frames[0])).to.equal(fileUrl('frame-with-frame.html'));
      expect(await getUrl(webFrame.frames[0].frames[0])).to.equal(fileUrl('frame.html'));
    });
  });

  describe('WebFrame.reload', () => {
    it('reloads a frame', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
      await w.loadFile(path.join(subframesPath, 'frame.html'));
      const webFrame = w.webContents.mainFrame;

      await webFrame.executeJavaScript('window.TEMP = 1', false);
      expect(webFrame.reload()).to.be.true();
      await emittedOnce(w.webContents, 'dom-ready');
      expect(await webFrame.executeJavaScript('window.TEMP', false)).to.be.null();
    });
  });

  describe('disposed WebFrames', () => {
    let w: BrowserWindow;
    let webFrame: WebFrameMain;

    before(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
      await w.loadFile(path.join(subframesPath, 'frame-with-frame-container.html'));
      webFrame = w.webContents.mainFrame;
      w.destroy();
      // Wait for WebContents, and thus RenderFrameHost, to be destroyed.
      await new Promise(resolve => setTimeout(resolve, 0));
    });

    it('throws upon accessing properties', () => {
      expect(() => webFrame.url).to.throw();
    });
  });

  describe('webFrameMain.fromId', () => {
    it('returns undefined for unknown IDs', () => {
      expect(webFrameMain.fromId(0, 0)).to.be.undefined();
    });

    it('can find each frame from navigation events', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });

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
});

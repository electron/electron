import { expect } from 'chai';
import * as http from 'http';
import * as path from 'path';
import * as url from 'url';
import { BrowserWindow } from 'electron/main';
import { closeAllWindows } from './window-helpers';
import { emittedOnce } from './events-helpers';
import { AddressInfo } from 'net';

type TODO = any;

describe.only('webFrameMain module', () => {
  const fixtures = path.resolve(__dirname, '..', 'spec-main', 'fixtures');
  const subframesPath = path.join(fixtures, 'sub-frames');

  const fileUrl = (filename: string) => url.pathToFileURL(path.join(subframesPath, filename)).href;

  afterEach(closeAllWindows);

  describe('WebFrame traversal APIs', () => {
    let w: BrowserWindow;
    let webFrame: TODO;

    beforeEach(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
      await w.loadFile(path.join(subframesPath, 'frame-with-frame-container.html'));
      webFrame = (w.webContents as TODO).webFrame;
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
      const urls = webFrame.framesInSubtree.map((frame: TODO) => frame.url);
      expect(urls).to.deep.equal([
        fileUrl('frame-with-frame-container.html'),
        fileUrl('frame-with-frame.html'),
        fileUrl('frame.html')
      ]);
    });

    it('can traverse all frames in subtree', () => {
      const urls = webFrame.frames[0].framesInSubtree.map((frame: TODO) => frame.url);
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
        webFrame = (w.webContents as TODO).webFrame;
        expect(webFrame.url.startsWith(serverA.url)).to.be.true();
        expect(webFrame.frames[0].url).to.equal(serverB.url);
      });
    });
  });

  describe('WebFrame.url', () => {
    it('should report correct address for each subframe', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
      await w.loadFile(path.join(subframesPath, 'frame-with-frame-container.html'));
      const webFrame = (w.webContents as TODO).webFrame;

      expect(webFrame.url).to.equal(fileUrl('frame-with-frame-container.html'));
      expect(webFrame.frames[0].url).to.equal(fileUrl('frame-with-frame.html'));
      expect(webFrame.frames[0].frames[0].url).to.equal(fileUrl('frame.html'));
    });
  });

  describe('WebFrame IDs', () => {
    it('has properties for various identifiers', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
      await w.loadFile(path.join(subframesPath, 'frame.html'));
      const webFrame = (w.webContents as TODO).webFrame;
      expect(webFrame).to.haveOwnProperty('frameTreeNodeId');
      expect(webFrame).to.haveOwnProperty('processId');
      expect(webFrame).to.haveOwnProperty('routingId');
    });
  });

  describe('WebFrame.executeJavaScript', () => {
    it('can inject code into any subframe', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
      await w.loadFile(path.join(subframesPath, 'frame-with-frame-container.html'));
      const webFrame = (w.webContents as TODO).webFrame;

      const getUrl = (frame: TODO) => frame.executeJavaScript('location.href', false);
      expect(await getUrl(webFrame)).to.equal(fileUrl('frame-with-frame-container.html'));
      expect(await getUrl(webFrame.frames[0])).to.equal(fileUrl('frame-with-frame.html'));
      expect(await getUrl(webFrame.frames[0].frames[0])).to.equal(fileUrl('frame.html'));
    });
  });

  describe('WebFrame.reload', () => {
    it('reloads a frame', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
      await w.loadFile(path.join(subframesPath, 'frame.html'));
      const webFrame = (w.webContents as TODO).webFrame;

      await webFrame.executeJavaScript('window.TEMP = 1', false);
      expect(webFrame.reload()).to.be.true();
      await emittedOnce(w.webContents, 'dom-ready');
      expect(await webFrame.executeJavaScript('window.TEMP', false)).to.be.null();
    });
  });

  describe('disposed WebFrames', () => {
    let w: BrowserWindow;
    let webFrame: TODO;

    before(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
      await w.loadFile(path.join(subframesPath, 'frame-with-frame-container.html'));
      webFrame = (w.webContents as TODO).webFrame;
      w.destroy();
      // Wait for WebContents, and thus RenderFrameHost, to be destroyed.
      await new Promise(resolve => setTimeout(resolve, 0));
    });

    it('throws upon accessing properties', () => {
      expect(() => webFrame.url).to.throw();
    });
  });
});

import { expect } from 'chai';
import * as path from 'path';
import * as url from 'url';
import { BrowserWindow } from 'electron/main';
import { closeAllWindows } from './window-helpers';

type TODO = any;

describe.only('webFrameMain module', () => {
  const fixtures = path.resolve(__dirname, '..', 'spec-main', 'fixtures');
  const subframesPath = path.join(fixtures, 'sub-frames');

  afterEach(closeAllWindows);

  describe('WebFrame traversal api', () => {
    let webFrame: TODO;

    beforeEach(async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
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
  });

  describe('WebFrame.url', () => {
    it('should report correct address for each subframe', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
      await w.loadFile(path.join(subframesPath, 'frame-with-frame-container.html'));
      const webFrame = (w.webContents as TODO).webFrame;

      const fileUrl = (filename: string) => url.pathToFileURL(path.join(subframesPath, filename)).href;
      expect(webFrame.url).to.equal(fileUrl('frame-with-frame-container.html'));
      expect(webFrame.frames[0].url).to.equal(fileUrl('frame-with-frame.html'));
      expect(webFrame.frames[0].frames[0].url).to.equal(fileUrl('frame.html'));
    });
  });
});

import { clipboard } from 'electron/common';
import { BrowserWindow, WebFrameMain } from 'electron/main';

import { expect } from 'chai';
import { afterEach, beforeEach, describe } from 'vitest';

import * as path from 'node:path';
import * as url from 'node:url';

import { ifit, waitUntil } from '../lib/spec-helpers';
import { closeAllWindows } from '../lib/window-helpers';

const fixtures = path.resolve(__dirname, '..', 'fixtures');
const subframesPath = path.join(fixtures, 'sub-frames');

describe('webFrameMain (serial)', () => {
  afterEach(closeAllWindows);

  describe('webFrameMain.copyVideoFrameAt', () => {
    const insertVideoInFrame = async (frame: WebFrameMain) => {
      const videoFilePath = url.pathToFileURL(path.join(fixtures, 'cat-spin.mp4')).href;
      await frame.executeJavaScript(`
        const video = document.createElement('video');
        video.src = '${videoFilePath}';
        video.muted = true;
        video.loop = true;
        video.play();
        document.body.appendChild(video);
      `);
    };

    const getFramePosition = async (frame: WebFrameMain) => {
      const point = (await frame.executeJavaScript(
        `(${() => {
          const iframe = document.querySelector('iframe');
          if (!iframe) return;
          const rect = iframe.getBoundingClientRect();
          return { x: Math.floor(rect.x), y: Math.floor(rect.y) };
        }})()`
      )) as Electron.Point;
      expect(point).to.be.an('object');
      return point;
    };

    const copyVideoFrameInFrame = async (frame: WebFrameMain, signal: AbortSignal) => {
      const point = (await frame.executeJavaScript(
        `(${() => {
          const video = document.querySelector('video');
          if (!video) return;
          const rect = video.getBoundingClientRect();
          return {
            x: Math.floor(rect.x + rect.width / 2),
            y: Math.floor(rect.y + rect.height / 2)
          };
        }})()`
      )) as Electron.Point;

      expect(point).to.be.an('object');

      // Translate coordinate to be relative of main frame
      if (frame.parent) {
        const framePosition = await getFramePosition(frame.parent);
        point.x += framePosition.x;
        point.y += framePosition.y;
      }

      expect(clipboard.readImage().isEmpty()).to.be.true();
      // wait for video to load
      await frame.executeJavaScript(
        `(${() => {
          const video = document.querySelector('video');
          if (!video) return;
          return new Promise((resolve) => {
            if (video.readyState >= 4) resolve(null);
            else video.addEventListener('canplaythrough', resolve, { once: true });
          });
        }})()`
      );
      frame.copyVideoFrameAt(point.x, point.y);
      await waitUntil(() => clipboard.availableFormats().includes('image/png'), signal);
      expect(clipboard.readImage().isEmpty()).to.be.false();
    };

    beforeEach(() => {
      clipboard.clear();
    });

    // TODO: Re-enable on Windows CI once Chromium fixes the intermittent
    // backwards-time DCHECK hit while copying video frames:
    // DCHECK failed: !delta.is_negative().
    ifit(!(process.platform === 'win32' && process.env.CI))('copies video frame in main frame', async (ctx) => {
      const w = new BrowserWindow({ show: false });
      await w.webContents.loadFile(path.join(fixtures, 'blank.html'));
      await insertVideoInFrame(w.webContents.mainFrame);
      await copyVideoFrameInFrame(w.webContents.mainFrame, ctx.signal);
      await waitUntil(() => clipboard.availableFormats().includes('image/png'), ctx.signal);
    });

    ifit(!(process.platform === 'win32' && process.env.CI))('copies video frame in subframe', async (ctx) => {
      const w = new BrowserWindow({ show: false });
      await w.webContents.loadFile(path.join(subframesPath, 'frame-with-frame.html'));
      const subframe = w.webContents.mainFrame.frames[0];
      expect(subframe).to.exist();
      await insertVideoInFrame(subframe);
      await copyVideoFrameInFrame(subframe, ctx.signal);
      await waitUntil(() => clipboard.availableFormats().includes('image/png'), ctx.signal);
    });
  });
});

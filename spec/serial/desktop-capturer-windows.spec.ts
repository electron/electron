import { BrowserWindow, desktopCapturer } from 'electron/main';

import { expect } from 'chai';
import { afterAll, afterEach, beforeAll, it } from 'vitest';

import { once } from 'node:events';
import { setTimeout } from 'node:timers/promises';

import { ifdescribe, ifit } from '../lib/spec-helpers';
import { closeAllWindows } from '../lib/window-helpers';

ifdescribe(process.platform !== 'linux')('desktopCapturer (serial)', () => {
  afterEach(closeAllWindows);

  ifit(process.platform !== 'linux')('moveAbove should move the window at the requested place', async function () {
    // DesktopCapturer.getSources() is guaranteed to return in the correct
    // z-order from foreground to background.
    const MAX_WIN = 4;
    const wList: BrowserWindow[] = [];

    const destroyWindows = () => {
      for (const w of wList) {
        w.destroy();
      }
    };

    try {
      for (let i = 0; i < MAX_WIN; i++) {
        const w = new BrowserWindow({ show: false, width: 100, height: 100 });
        wList.push(w);
      }
      expect(wList.length).to.equal(MAX_WIN);

      // Show and focus all the windows.
      for (const w of wList) {
        const wShown = once(w, 'show');
        const wFocused = once(w, 'focus');

        w.show();
        w.focus();

        await wShown;
        await wFocused;
      }

      // At this point our windows should be showing from bottom to top.

      // DesktopCapturer.getSources() returns sources sorted from foreground to
      // background, i.e. top to bottom.
      let sources = await desktopCapturer.getSources({
        types: ['window'],
        thumbnailSize: { width: 0, height: 0 }
      });

      expect(sources).to.be.an('array').that.is.not.empty();
      expect(sources.length).to.gte(MAX_WIN);

      // Only keep our windows, they must be in the MAX_WIN first windows.
      sources.splice(MAX_WIN, sources.length - MAX_WIN);
      expect(sources.length).to.equal(MAX_WIN);
      expect(sources.length).to.equal(wList.length);

      // Check that the sources and wList are sorted in the reverse order.
      // If they're not, skip remaining checks because either focus or
      // window placement are not reliable in the running test environment.
      const wListReversed = wList.slice().reverse();
      const proceed = sources.every((source, index) => source.id === wListReversed[index].getMediaSourceId());
      if (!proceed) return;

      // Move windows so wList is sorted from foreground to background.
      for (const [i, w] of wList.entries()) {
        if (i < wList.length - 1) {
          const next = wList[wList.length - 1];
          w.focus();
          w.moveAbove(next.getMediaSourceId());
          // Ensure the window has time to move.
          await setTimeout(2000);
        }
      }

      sources = await desktopCapturer.getSources({
        types: ['window'],
        thumbnailSize: { width: 0, height: 0 }
      });

      sources.splice(MAX_WIN, sources.length);
      expect(sources.length).to.equal(MAX_WIN);
      expect(sources.length).to.equal(wList.length);

      // Check that the sources and wList are sorted in the same order.
      for (const [index, source] of sources.entries()) {
        const wID = wList[index].getMediaSourceId();
        expect(source.id).to.equal(wID);
      }
    } finally {
      destroyWindows();
    }
  });

  ifdescribe(process.platform !== 'linux')('fetchWindowIcons', () => {
    let w: BrowserWindow;
    let testSource: Electron.DesktopCapturerSource | undefined;
    let appIcon: Electron.NativeImage | undefined;

    beforeAll(async () => {
      w = new BrowserWindow({
        width: 200,
        height: 200,
        show: true,
        title: 'desktop-capturer-test-window'
      });
      await w.loadURL('about:blank');
      const sources = await desktopCapturer.getSources({
        types: ['window'],
        fetchWindowIcons: true
      });
      testSource = sources.find((s) => s.name === 'desktop-capturer-test-window');
      appIcon = testSource?.appIcon;
    });

    afterAll(() => {
      if (w) w.destroy();
    });

    it('should find the test window in the list of captured sources', () => {
      expect(testSource, `The ${w.getTitle()} window was not found by desktopCapturer`).to.exist();
    });

    it('should return a non-null appIcon for the captured window', () => {
      expect(appIcon, 'appIcon property is null or undefined').to.exist();
    });

    it('should return an appIcon that is not an empty image', () => {
      expect(appIcon?.isEmpty()).to.be.false();
    });

    it('should return an appIcon that encodes to a valid PNG data URL', () => {
      const url = appIcon?.toDataURL();
      expect(url).to.be.a('string');
      // This is header 'data:image/png;base64,' length;
      expect(url?.length).to.be.greaterThan(22);
      expect(url?.startsWith('data:image/png;base64,')).to.be.true();
    });

    it('should return an appIcon with dimensions greater than 0x0 pixels', () => {
      const { width, height } = appIcon?.getSize() || { width: 0, height: 0 };
      expect(width).to.be.greaterThan(0);
      expect(height).to.be.greaterThan(0);
    });
  });
});

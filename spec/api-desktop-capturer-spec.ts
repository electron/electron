import { screen, desktopCapturer, BrowserWindow } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';
import { setTimeout } from 'node:timers/promises';

import { ifdescribe, ifit } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';

function getSourceTypes (): ('window' | 'screen')[] {
  if (process.platform === 'linux') {
    return ['screen'];
  }
  return ['window', 'screen'];
}

ifdescribe(!process.arch.includes('arm') && process.platform !== 'win32')('desktopCapturer', () => {
  it('should return a non-empty array of sources', async () => {
    const sources = await desktopCapturer.getSources({ types: getSourceTypes() });
    expect(sources).to.be.an('array').that.is.not.empty();
  });

  it('throws an error for invalid options', async () => {
    const promise = desktopCapturer.getSources(['window', 'screen'] as any);
    await expect(promise).to.be.eventually.rejectedWith(Error, 'Invalid options');
  });

  it('does not throw an error when called more than once (regression)', async () => {
    const sources1 = await desktopCapturer.getSources({ types: getSourceTypes() });
    expect(sources1).to.be.an('array').that.is.not.empty();

    const sources2 = await desktopCapturer.getSources({ types: getSourceTypes() });
    expect(sources2).to.be.an('array').that.is.not.empty();
  });

  // Linux doesn't return any window sources.
  ifit(process.platform !== 'linux')('responds to subsequent calls of different options', async () => {
    const promise1 = desktopCapturer.getSources({ types: ['window'] });
    await expect(promise1).to.eventually.be.fulfilled();

    const promise2 = desktopCapturer.getSources({ types: ['screen'] });
    await expect(promise2).to.eventually.be.fulfilled();
  });

  // Linux doesn't return any window sources.
  ifit(process.platform !== 'linux')('returns an empty display_id for window sources', async () => {
    const w2 = new BrowserWindow({ width: 200, height: 200 });
    await w2.loadURL('about:blank');

    const sources = await desktopCapturer.getSources({ types: ['window'] });
    w2.destroy();
    expect(sources).to.be.an('array').that.is.not.empty();
    for (const { display_id: displayId } of sources) {
      expect(displayId).to.be.a('string').and.be.empty();
    }
  });

  ifit(process.platform !== 'linux')('returns display_ids matching the Screen API', async () => {
    const displays = screen.getAllDisplays();
    const sources = await desktopCapturer.getSources({ types: ['screen'] });
    expect(sources).to.be.an('array').of.length(displays.length);

    for (const [i, source] of sources.entries()) {
      expect(source.display_id).to.equal(displays[i].id.toString());
    }
  });

  it('enabling thumbnail should return non-empty images', async () => {
    const w2 = new BrowserWindow({ show: false, width: 200, height: 200, webPreferences: { contextIsolation: false } });
    const wShown = once(w2, 'show');
    w2.show();
    await wShown;

    const isNonEmpties: boolean[] = (await desktopCapturer.getSources({
      types: getSourceTypes(),
      thumbnailSize: { width: 100, height: 100 }
    })).map(s => s.thumbnail.constructor.name === 'NativeImage' && !s.thumbnail.isEmpty());

    w2.destroy();
    expect(isNonEmpties).to.be.an('array').that.is.not.empty();
    expect(isNonEmpties.every(e => e === true)).to.be.true();
  });

  it('disabling thumbnail should return empty images', async () => {
    const w2 = new BrowserWindow({ show: false, width: 200, height: 200, webPreferences: { contextIsolation: false } });
    const wShown = once(w2, 'show');
    w2.show();
    await wShown;

    const isEmpties: boolean[] = (await desktopCapturer.getSources({
      types: getSourceTypes(),
      thumbnailSize: { width: 0, height: 0 }
    })).map(s => s.thumbnail.constructor.name === 'NativeImage' && s.thumbnail.isEmpty());

    w2.destroy();
    expect(isEmpties).to.be.an('array').that.is.not.empty();
    expect(isEmpties.every(e => e === true)).to.be.true();
  });

  // Linux doesn't return any window sources.
  ifit(process.platform !== 'linux')('getMediaSourceId should match DesktopCapturerSource.id', async function () {
    const w2 = new BrowserWindow({ show: false, width: 100, height: 100, webPreferences: { contextIsolation: false } });
    const wShown = once(w2, 'show');
    const wFocused = once(w2, 'focus');
    w2.show();
    w2.focus();
    await wShown;
    await wFocused;

    const mediaSourceId = w2.getMediaSourceId();
    const sources = await desktopCapturer.getSources({
      types: ['window'],
      thumbnailSize: { width: 0, height: 0 }
    });
    w2.destroy();

    expect(sources).to.be.an('array').that.is.not.empty();
    const foundSource = sources.find((source) => {
      return source.id === mediaSourceId;
    });
    expect(mediaSourceId).to.equal(foundSource!.id);
  });

  // Linux doesn't return any window sources.
  ifit(process.platform !== 'linux')('getSources should not incorrectly duplicate window_id', async function () {
    const w2 = new BrowserWindow({ show: false, width: 100, height: 100, webPreferences: { contextIsolation: false } });
    const wShown = once(w2, 'show');
    const wFocused = once(w2, 'focus');
    w2.show();
    w2.focus();
    await wShown;
    await wFocused;

    // ensure window_id isn't duplicated in getMediaSourceId,
    // which uses a different method than getSources
    const mediaSourceId = w2.getMediaSourceId();
    const ids = mediaSourceId.split(':');
    expect(ids[1]).to.not.equal(ids[2]);

    const sources = await desktopCapturer.getSources({
      types: ['window'],
      thumbnailSize: { width: 0, height: 0 }
    });
    w2.destroy();

    expect(sources).to.be.an('array').that.is.not.empty();
    for (const source of sources) {
      const sourceIds = source.id.split(':');
      expect(sourceIds[1]).to.not.equal(sourceIds[2]);
    }
  });

  // Regression test - see https://github.com/electron/electron/issues/43002
  it('does not affect window resizable state', async () => {
    const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
    await w.loadURL('about:blank');
    w.resizable = false;

    const wShown = once(w, 'show');
    w.show();
    await wShown;

    const sources = await desktopCapturer.getSources({ types: getSourceTypes() });
    expect(sources).to.be.an('array').that.is.not.empty();

    expect(w.resizable).to.be.false();
    await closeAllWindows();
  });

  // Linux doesn't return any window sources.
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
      const proceed = sources.every(
        (source, index) => source.id === wListReversed[index].getMediaSourceId());
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
});

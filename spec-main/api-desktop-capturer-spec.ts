import { expect } from 'chai';
import { screen, desktopCapturer, BrowserWindow } from 'electron/main';
import { emittedOnce } from './events-helpers';
import { ifdescribe, ifit } from './spec-helpers';
import { closeAllWindows } from './window-helpers';
import * as path from 'path';

const features = process._linkedBinding('electron_common_features');

ifdescribe(!process.arch.includes('arm') && process.platform !== 'win32')('desktopCapturer', () => {
  if (!features.isDesktopCapturerEnabled()) {
    // This condition can't go the `ifdescribe` call because its inner code
    // it still executed, and if the feature is disabled some function calls here fail.
    return;
  }

  let w: BrowserWindow;

  before(async () => {
    w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
    await w.loadURL('about:blank');
  });

  after(closeAllWindows);

  // TODO(nornagon): figure out why this test is failing on Linux and re-enable it.
  ifit(process.platform !== 'linux')('should return a non-empty array of sources', async () => {
    const sources = await desktopCapturer.getSources({ types: ['window', 'screen'] });
    expect(sources).to.be.an('array').that.is.not.empty();
  });

  it('throws an error for invalid options', async () => {
    const promise = desktopCapturer.getSources(['window', 'screen'] as any);
    await expect(promise).to.be.eventually.rejectedWith(Error, 'Invalid options');
  });

  // TODO(nornagon): figure out why this test is failing on Linux and re-enable it.
  ifit(process.platform !== 'linux')('does not throw an error when called more than once (regression)', async () => {
    const sources1 = await desktopCapturer.getSources({ types: ['window', 'screen'] });
    expect(sources1).to.be.an('array').that.is.not.empty();

    const sources2 = await desktopCapturer.getSources({ types: ['window', 'screen'] });
    expect(sources2).to.be.an('array').that.is.not.empty();
  });

  ifit(process.platform !== 'linux')('responds to subsequent calls of different options', async () => {
    const promise1 = desktopCapturer.getSources({ types: ['window'] });
    await expect(promise1).to.eventually.be.fulfilled();

    const promise2 = desktopCapturer.getSources({ types: ['screen'] });
    await expect(promise2).to.eventually.be.fulfilled();
  });

  // Linux doesn't return any window sources.
  ifit(process.platform !== 'linux')('returns an empty display_id for window sources on Windows and Mac', async () => {
    const w = new BrowserWindow({ width: 200, height: 200 });
    await w.loadURL('about:blank');

    const sources = await desktopCapturer.getSources({ types: ['window'] });
    w.destroy();
    expect(sources).to.be.an('array').that.is.not.empty();
    for (const { display_id: displayId } of sources) {
      expect(displayId).to.be.a('string').and.be.empty();
    }
  });

  ifit(process.platform !== 'linux')('returns display_ids matching the Screen API on Windows and Mac', async () => {
    const displays = screen.getAllDisplays();
    const sources = await desktopCapturer.getSources({ types: ['screen'] });
    expect(sources).to.be.an('array').of.length(displays.length);

    for (let i = 0; i < sources.length; i++) {
      expect(sources[i].display_id).to.equal(displays[i].id.toString());
    }
  });

  it('disabling thumbnail should return empty images', async () => {
    const w2 = new BrowserWindow({ show: false, width: 200, height: 200, webPreferences: { contextIsolation: false } });
    const wShown = emittedOnce(w2, 'show');
    w2.show();
    await wShown;

    const isEmpties: boolean[] = (await desktopCapturer.getSources({
      types: ['window', 'screen'],
      thumbnailSize: { width: 0, height: 0 }
    })).map(s => s.thumbnail.constructor.name === 'NativeImage' && s.thumbnail.isEmpty());

    w2.destroy();
    expect(isEmpties).to.be.an('array').that.is.not.empty();
    expect(isEmpties.every(e => e === true)).to.be.true();
  });

  it('getMediaSourceId should match DesktopCapturerSource.id', async () => {
    const w = new BrowserWindow({ show: false, width: 100, height: 100, webPreferences: { contextIsolation: false } });
    const wShown = emittedOnce(w, 'show');
    const wFocused = emittedOnce(w, 'focus');
    w.show();
    w.focus();
    await wShown;
    await wFocused;

    const mediaSourceId = w.getMediaSourceId();
    const sources = await desktopCapturer.getSources({
      types: ['window'],
      thumbnailSize: { width: 0, height: 0 }
    });
    w.destroy();

    // TODO(julien.isorce): investigate why |sources| is empty on the linux
    // bots while it is not on my workstation, as expected, with and without
    // the --ci parameter.
    if (process.platform === 'linux' && sources.length === 0) {
      it.skip('desktopCapturer.getSources returned an empty source list');
      return;
    }

    expect(sources).to.be.an('array').that.is.not.empty();
    const foundSource = sources.find((source) => {
      return source.id === mediaSourceId;
    });
    expect(mediaSourceId).to.equal(foundSource!.id);
  });

  it('getSources should not incorrectly duplicate window_id', async () => {
    const w = new BrowserWindow({ show: false, width: 100, height: 100, webPreferences: { contextIsolation: false } });
    const wShown = emittedOnce(w, 'show');
    const wFocused = emittedOnce(w, 'focus');
    w.show();
    w.focus();
    await wShown;
    await wFocused;

    // ensure window_id isn't duplicated in getMediaSourceId,
    // which uses a different method than getSources
    const mediaSourceId = w.getMediaSourceId();
    const ids = mediaSourceId.split(':');
    expect(ids[1]).to.not.equal(ids[2]);

    const sources = await desktopCapturer.getSources({
      types: ['window'],
      thumbnailSize: { width: 0, height: 0 }
    });
    w.destroy();

    // TODO(julien.isorce): investigate why |sources| is empty on the linux
    // bots while it is not on my workstation, as expected, with and without
    // the --ci parameter.
    if (process.platform === 'linux' && sources.length === 0) {
      it.skip('desktopCapturer.getSources returned an empty source list');
      return;
    }

    expect(sources).to.be.an('array').that.is.not.empty();
    for (const source of sources) {
      const sourceIds = source.id.split(':');
      expect(sourceIds[1]).to.not.equal(sourceIds[2]);
    }
  });

  // TODO(deepak1556): currently fails on all ci, enable it after upgrade.
  it.skip('moveAbove should move the window at the requested place', async () => {
    // DesktopCapturer.getSources() is guaranteed to return in the correct
    // z-order from foreground to background.
    const MAX_WIN = 4;
    const mainWindow = w;
    const wList = [mainWindow];
    try {
      for (let i = 0; i < MAX_WIN - 1; i++) {
        const w = new BrowserWindow({ show: true, width: 100, height: 100 });
        wList.push(w);
      }
      expect(wList.length).to.equal(MAX_WIN);

      // Show and focus all the windows.
      wList.forEach(async (w) => {
        const wFocused = emittedOnce(w, 'focus');
        w.focus();
        await wFocused;
      });
      // At this point our windows should be showing from bottom to top.

      // DesktopCapturer.getSources() returns sources sorted from foreground to
      // background, i.e. top to bottom.
      let sources = await desktopCapturer.getSources({
        types: ['window'],
        thumbnailSize: { width: 0, height: 0 }
      });

      // TODO(julien.isorce): investigate why |sources| is empty on the linux
      // bots while it is not on my workstation, as expected, with and without
      // the --ci parameter.
      if (process.platform === 'linux' && sources.length === 0) {
        wList.forEach((w) => {
          if (w !== mainWindow) {
            w.destroy();
          }
        });
        it.skip('desktopCapturer.getSources returned an empty source list');
        return;
      }

      expect(sources).to.be.an('array').that.is.not.empty();
      expect(sources.length).to.gte(MAX_WIN);

      // Only keep our windows, they must be in the MAX_WIN first windows.
      sources.splice(MAX_WIN, sources.length - MAX_WIN);
      expect(sources.length).to.equal(MAX_WIN);
      expect(sources.length).to.equal(wList.length);

      // Check that the sources and wList are sorted in the reverse order.
      const wListReversed = wList.slice(0).reverse();
      const canGoFurther = sources.every(
        (source, index) => source.id === wListReversed[index].getMediaSourceId());
      if (!canGoFurther) {
        // Skip remaining checks because either focus or window placement are
        // not reliable in the running test environment. So there is no point
        // to go further to test moveAbove as requirements are not met.
        return;
      }

      // Do the real work, i.e. move each window above the next one so that
      // wList is sorted from foreground to background.
      wList.forEach(async (w, index) => {
        if (index < (wList.length - 1)) {
          const wNext = wList[index + 1];
          w.moveAbove(wNext.getMediaSourceId());
        }
      });

      sources = await desktopCapturer.getSources({
        types: ['window'],
        thumbnailSize: { width: 0, height: 0 }
      });
      // Only keep our windows again.
      sources.splice(MAX_WIN, sources.length - MAX_WIN);
      expect(sources.length).to.equal(MAX_WIN);
      expect(sources.length).to.equal(wList.length);

      // Check that the sources and wList are sorted in the same order.
      sources.forEach((source, index) => {
        expect(source.id).to.equal(wList[index].getMediaSourceId());
      });
    } finally {
      wList.forEach((w) => {
        if (w !== mainWindow) {
          w.destroy();
        }
      });
    }
  });
  it('setSkipCursor does not crash', async () => {
    // test for unknownid
    expect(() => desktopCapturer.setSkipCursor('unknownid', true)).to.not.throw();
    // test for empty id
    expect(() => desktopCapturer.setSkipCursor('', false)).to.not.throw();
    // test for valid id
    expect(async () => {
      const sources = await getSources({
        types: ['screen']
      });
      if (sources.length > 0) {
        desktopCapturer.setSkipCursor(sources[0].id, true);
      }
    }).to.not.throw();
  });

  it('setSkipCursor called when sharing content', async () => {
    // test calling set skip cursor when we are sharing content in renderer
    const win = new BrowserWindow({
      show: false,
      width: 200,
      height: 200,
      webPreferences: {
        nodeIntegration: true,
        contextIsolation: false
      }
    });
    win.loadFile(path.join(__dirname, 'fixtures', 'api', 'desktop-capturer-setskipcursor.html'));
    win.show();

    const result = await win.webContents.executeJavaScript('testSetSkipCursor()');

    expect(result).to.equal('ok');

    win.destroy();
  });
});

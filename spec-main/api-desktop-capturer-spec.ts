import { expect } from 'chai';
import { screen, BrowserWindow, SourcesOptions } from 'electron/main';
import { desktopCapturer } from 'electron/common';
import { emittedOnce } from './events-helpers';
import { ifdescribe, ifit } from './spec-helpers';
import { closeAllWindows } from './window-helpers';

const features = process.electronBinding('features');

ifdescribe(features.isDesktopCapturerEnabled() && !process.arch.includes('arm') && process.platform !== 'win32')('desktopCapturer', () => {
  let w: BrowserWindow;
  before(async () => {
    w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } });
    await w.loadURL('about:blank');
  });
  after(closeAllWindows);

  const getSources: typeof desktopCapturer.getSources = (options: SourcesOptions) => {
    return w.webContents.executeJavaScript(`
      require('electron').desktopCapturer.getSources(${JSON.stringify(options)}).then(m => JSON.parse(JSON.stringify(m)))
    `);
  };

  const generateSpecs = (description: string, getSources: typeof desktopCapturer.getSources) => {
    describe(description, () => {
      // TODO(nornagon): figure out why this test is failing on Linux and re-enable it.
      ifit(process.platform !== 'linux')('should return a non-empty array of sources', async () => {
        const sources = await getSources({ types: ['window', 'screen'] });
        expect(sources).to.be.an('array').that.is.not.empty();
      });

      it('throws an error for invalid options', async () => {
        const promise = getSources(['window', 'screen'] as any);
        await expect(promise).to.be.eventually.rejectedWith(Error, 'Invalid options');
      });

      // TODO(nornagon): figure out why this test is failing on Linux and re-enable it.
      ifit(process.platform !== 'linux')('does not throw an error when called more than once (regression)', async () => {
        const sources1 = await getSources({ types: ['window', 'screen'] });
        expect(sources1).to.be.an('array').that.is.not.empty();

        const sources2 = await getSources({ types: ['window', 'screen'] });
        expect(sources2).to.be.an('array').that.is.not.empty();
      });

      ifit(process.platform !== 'linux')('responds to subsequent calls of different options', async () => {
        const promise1 = getSources({ types: ['window'] });
        await expect(promise1).to.eventually.be.fulfilled();

        const promise2 = getSources({ types: ['screen'] });
        await expect(promise2).to.eventually.be.fulfilled();
      });

      // Linux doesn't return any window sources.
      ifit(process.platform !== 'linux')('returns an empty display_id for window sources on Windows and Mac', async () => {
        const w = new BrowserWindow({ width: 200, height: 200 });
        await w.loadURL('about:blank');

        const sources = await getSources({ types: ['window'] });
        w.destroy();
        expect(sources).to.be.an('array').that.is.not.empty();
        for (const { display_id: displayId } of sources) {
          expect(displayId).to.be.a('string').and.be.empty();
        }
      });

      ifit(process.platform !== 'linux')('returns display_ids matching the Screen API on Windows and Mac', async () => {
        const displays = screen.getAllDisplays();
        const sources = await getSources({ types: ['screen'] });
        expect(sources).to.be.an('array').of.length(displays.length);

        for (let i = 0; i < sources.length; i++) {
          expect(sources[i].display_id).to.equal(displays[i].id.toString());
        }
      });
    });
  };

  generateSpecs('in renderer process', getSources);
  generateSpecs('in main process', desktopCapturer.getSources);

  ifit(process.platform !== 'linux')('returns an empty source list if blocked by the main process', async () => {
    w.webContents.once('desktop-capturer-get-sources', (event) => {
      event.preventDefault();
    });
    const sources = await getSources({ types: ['screen'] });
    expect(sources).to.be.empty();
  });

  it('disabling thumbnail should return empty images', async () => {
    const w2 = new BrowserWindow({ show: false, width: 200, height: 200 });
    const wShown = emittedOnce(w2, 'show');
    w2.show();
    await wShown;

    const isEmpties: boolean[] = await w.webContents.executeJavaScript(`
      require('electron').desktopCapturer.getSources({
        types: ['window', 'screen'],
        thumbnailSize: { width: 0, height: 0 }
      }).then((sources) => {
        return sources.map(s => s.thumbnail.constructor.name === 'NativeImage' && s.thumbnail.isEmpty())
      })
    `);

    w2.destroy();
    expect(isEmpties).to.be.an('array').that.is.not.empty();
    expect(isEmpties.every(e => e === true)).to.be.true();
  });

  it('getMediaSourceId should match DesktopCapturerSource.id', async () => {
    const w = new BrowserWindow({ show: false, width: 100, height: 100 });
    const wShown = emittedOnce(w, 'show');
    const wFocused = emittedOnce(w, 'focus');
    w.show();
    w.focus();
    await wShown;
    await wFocused;

    const mediaSourceId = w.getMediaSourceId();
    const sources = await getSources({
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

  describe('getMediaSourceIdForWebContents', () => {
    const getMediaSourceIdForWebContents: typeof desktopCapturer.getMediaSourceIdForWebContents = (webContentsId: number) => {
      return w.webContents.executeJavaScript(`
        require('electron').desktopCapturer.getMediaSourceIdForWebContents(${JSON.stringify(webContentsId)}).then(r => JSON.parse(JSON.stringify(r)))
      `);
    };

    it('should return a stream id for web contents', async () => {
      const result = await getMediaSourceIdForWebContents(w.webContents.id);
      expect(result).to.be.a('string').that.is.not.empty();
    });

    it('throws an error for invalid options', async () => {
      const promise = getMediaSourceIdForWebContents('not-an-id' as unknown as number);
      await expect(promise).to.be.eventually.rejectedWith(Error, 'TypeError: Error processing argument');
    });

    it('throws an error for invalid web contents id', async () => {
      const promise = getMediaSourceIdForWebContents(-200);
      await expect(promise).to.be.eventually.rejectedWith(Error, 'Failed to find WebContents');
    });
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
      let sources = await getSources({
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

      sources = await getSources({
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
});

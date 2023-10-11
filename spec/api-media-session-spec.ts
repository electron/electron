import { expect } from 'chai';
import * as path from 'node:path';
import { once } from 'node:events';
import { BrowserWindow } from 'electron/main';
import { closeAllWindows } from './lib/window-helpers';

describe('mediaSession module', () => {
  const fixtures = path.resolve(__dirname, 'fixtures');
  let w: BrowserWindow;

  beforeEach(async () => {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        // Allow autoplay for tests so MediaSession instance is created.
        autoplayPolicy: 'no-user-gesture-required'
      }
    });
    await w.webContents.loadFile(path.join(fixtures, 'pages', 'media-session.html'));
  });

  afterEach(closeAllWindows);

  describe('mediaSession playback APIs', () => {
    it('play()', async () => {
      w.webContents.mediaSession.play();
    });

    it('pause()', async () => {
      w.webContents.mediaSession.pause();
    });

    it('stop()', async () => {
      w.webContents.mediaSession.stop();
    });

    it.skip('doesn\'t crash in inactive audio state', async () => {
      await w.webContents.loadURL('about:blank');
      // TODO(samuelmaddock): prevent DCHECK crash related to inactive audio state?
      w.webContents.mediaSession.stop();
    });
  });

  describe('"actions-changed" event', () => {
    it('emits when "play" action is registered', async () => {
      // Initial empty change
      await once(w.webContents.mediaSession, 'actions-changed');
      const promise = once(w.webContents.mediaSession, 'actions-changed');
      await w.webContents.executeJavaScript('navigator.mediaSession.setActionHandler("play", () => {});');
      const [, details] = await promise;
      expect(details.actions).to.be.an('array');
      expect(details.actions).to.deep.equal(['play']);
    });
  });

  describe('"info-changed" event', () => {
    it('emits playback state changes', async () => {
      const [, details] = await once(w.webContents.mediaSession, 'info-changed');
      expect(details).to.have.ownProperty('playbackState').that.is.a('string');
      expect(details.playbackState).to.be.equal('playing');

      const pausePromise = once(w.webContents.mediaSession, 'info-changed');
      w.webContents.executeJavaScript('document.querySelector("video").pause();');
      const [, pauseDetails] = await pausePromise;
      expect(pauseDetails.playbackState).to.be.equal('paused');
    });
  });
});

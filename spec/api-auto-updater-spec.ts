import { autoUpdater } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';

import { ifit, ifdescribe } from './lib/spec-helpers';

ifdescribe(!process.mas)('autoUpdater module', function () {
  describe('checkForUpdates', function () {
    ifit(process.platform === 'win32')('emits an error on Windows if the feed URL is not set', async function () {
      const errorEvent = once(autoUpdater, 'error') as Promise<[Error]>;
      autoUpdater.setFeedURL({ url: '' });
      autoUpdater.checkForUpdates();
      const [error] = await errorEvent;
      expect(error.message).to.equal('Update URL is not set');
    });
  });

  describe('getFeedURL', () => {
    it('returns an empty string by default', () => {
      expect(autoUpdater.getFeedURL()).to.equal('');
    });

    ifit(process.platform === 'win32')('correctly fetches the previously set FeedURL', function () {
      const updateURL = 'https://fake-update.electron.io';
      autoUpdater.setFeedURL({ url: updateURL });
      expect(autoUpdater.getFeedURL()).to.equal(updateURL);
    });
  });

  describe('setFeedURL', function () {
    ifdescribe(process.platform === 'win32' || process.platform === 'darwin')('on Mac or Windows', () => {
      it('sets url successfully using old (url, headers) syntax', () => {
        const url = 'http://electronjs.org';
        try {
          (autoUpdater.setFeedURL as any)(url, { header: 'val' });
        } catch { /* ignore */ }
        expect(autoUpdater.getFeedURL()).to.equal(url);
      });

      it('throws if no url is provided when using the old style', () => {
        expect(() => (autoUpdater.setFeedURL as any)()).to.throw('Expected an options object with a \'url\' property to be provided');
      });

      it('sets url successfully using new ({ url }) syntax', () => {
        const url = 'http://mymagicurl.local';
        try {
          autoUpdater.setFeedURL({ url });
        } catch { /* ignore */ }
        expect(autoUpdater.getFeedURL()).to.equal(url);
      });

      it('throws if no url is provided when using the new style', () => {
        expect(() => autoUpdater.setFeedURL({ noUrl: 'lol' } as any)
        ).to.throw('Expected options object to contain a \'url\' string property in setFeedUrl call');
      });
    });
  });

  describe('quitAndInstall', () => {
    ifit(process.platform === 'win32')('emits an error on Windows when no update is available', async function () {
      const errorEvent = once(autoUpdater, 'error') as Promise<[Error]>;
      autoUpdater.quitAndInstall();
      const [error] = await errorEvent;
      expect(error.message).to.equal('No update available, can\'t quit and install');
    });
  });
});

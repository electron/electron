import { autoUpdater } from 'electron/main';

import { describe, expect, it } from 'vitest';

import { once } from 'node:events';

import { ifit, ifdescribe } from './lib/spec-helpers.ts';

ifdescribe(!process.mas)('autoUpdater module', () => {
  describe('checkForUpdates', () => {
    ifit(process.platform === 'win32')('emits an error on Windows if the feed URL is not set', async () => {
      const errorEvent = once(autoUpdater, 'error') as Promise<[Error]>;
      autoUpdater.setFeedURL({ url: '' });
      autoUpdater.checkForUpdates();
      const [error] = await errorEvent;
      expect(error.message).toBe('Update URL is not set');
    });
  });

  describe('getFeedURL', () => {
    it('returns an empty string by default', () => {
      expect(autoUpdater.getFeedURL()).toBe('');
    });

    ifit(process.platform === 'win32')('correctly fetches the previously set FeedURL', () => {
      const updateURL = 'https://fake-update.electron.io';
      autoUpdater.setFeedURL({ url: updateURL });
      expect(autoUpdater.getFeedURL()).toBe(updateURL);
    });
  });

  describe('setFeedURL', () => {
    ifdescribe(process.platform === 'win32' || process.platform === 'darwin')('on Mac or Windows', () => {
      it('sets url successfully using old (url, headers) syntax', () => {
        const url = 'http://electronjs.org';
        try {
          (autoUpdater.setFeedURL as any)(url, { header: 'val' });
        } catch {
          /* ignore */
        }
        expect(autoUpdater.getFeedURL()).toBe(url);
      });

      it('throws if no url is provided when using the old style', () => {
        expect(() => (autoUpdater.setFeedURL as any)()).toThrow(
          "Expected an options object with a 'url' property to be provided"
        );
      });

      it('sets url successfully using new ({ url }) syntax', () => {
        const url = 'http://mymagicurl.local';
        try {
          autoUpdater.setFeedURL({ url });
        } catch {
          /* ignore */
        }
        expect(autoUpdater.getFeedURL()).toBe(url);
      });

      it('throws if no url is provided when using the new style', () => {
        expect(() => autoUpdater.setFeedURL({ noUrl: 'lol' } as any)).toThrow(
          "Expected options object to contain a 'url' string property in setFeedUrl call"
        );
      });

      ifit(process.platform === 'darwin')('throws if the url is not valid UTF-8', () => {
        const url = 'http://feedurl.local';
        try {
          autoUpdater.setFeedURL({ url });
        } catch {
          /* ignore */
        }
        expect(() => autoUpdater.setFeedURL({ url: '\uD800' })).toThrow("Expected 'url' to be a valid URL");
        expect(autoUpdater.getFeedURL()).toBe(url);
      });

      ifit(process.platform === 'darwin')('throws if the url is not parseable', () => {
        const url = 'http://feedurl.local';
        try {
          autoUpdater.setFeedURL({ url });
        } catch {
          /* ignore */
        }
        expect(() => autoUpdater.setFeedURL({ url: 'http://feed url.local' })).toThrow(
          "Expected 'url' to be a valid URL"
        );
        expect(autoUpdater.getFeedURL()).toBe(url);
      });
    });
  });

  describe('quitAndInstall', () => {
    ifit(process.platform === 'win32')('emits an error on Windows when no update is available', async () => {
      const errorEvent = once(autoUpdater, 'error') as Promise<[Error]>;
      autoUpdater.quitAndInstall();
      const [error] = await errorEvent;
      expect(error.message).toBe("No update available, can't quit and install");
    });
  });
});

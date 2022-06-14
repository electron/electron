import { expect } from 'chai';
import { nativeTheme, systemPreferences, BrowserWindow, ipcMain } from 'electron/main';
import * as os from 'os';
import * as path from 'path';
import * as semver from 'semver';

import { delay, ifdescribe } from './spec-helpers';
import { emittedOnce } from './events-helpers';
import { closeAllWindows } from './window-helpers';

describe('nativeTheme module', () => {
  describe('nativeTheme.shouldUseDarkColors', () => {
    it('returns a boolean', () => {
      expect(nativeTheme.shouldUseDarkColors).to.be.a('boolean');
    });
  });

  describe('nativeTheme.themeSource', () => {
    afterEach(async () => {
      nativeTheme.themeSource = 'system';
      // Wait for any pending events to emit
      await delay(20);

      closeAllWindows();
    });

    it('is system by default', () => {
      expect(nativeTheme.themeSource).to.equal('system');
    });

    it('should override the value of shouldUseDarkColors', () => {
      nativeTheme.themeSource = 'dark';
      expect(nativeTheme.shouldUseDarkColors).to.equal(true);
      nativeTheme.themeSource = 'light';
      expect(nativeTheme.shouldUseDarkColors).to.equal(false);
    });

    it('should emit the "updated" event when it is set and the resulting "shouldUseDarkColors" value changes', async () => {
      let updatedEmitted = emittedOnce(nativeTheme, 'updated');
      nativeTheme.themeSource = 'dark';
      await updatedEmitted;
      updatedEmitted = emittedOnce(nativeTheme, 'updated');
      nativeTheme.themeSource = 'light';
      await updatedEmitted;
    });

    it('should not emit the "updated" event when it is set and the resulting "shouldUseDarkColors" value is the same', async () => {
      nativeTheme.themeSource = 'dark';
      // Wait a few ticks to allow an async events to flush
      await delay(20);
      let called = false;
      nativeTheme.once('updated', () => {
        called = true;
      });
      nativeTheme.themeSource = 'dark';
      // Wait a few ticks to allow an async events to flush
      await delay(20);
      expect(called).to.equal(false);
    });

    ifdescribe(process.platform === 'darwin' && semver.gte(os.release(), '18.0.0'))('on macOS 10.14', () => {
      it('should update appLevelAppearance when set', () => {
        nativeTheme.themeSource = 'dark';
        expect(systemPreferences.appLevelAppearance).to.equal('dark');
        nativeTheme.themeSource = 'light';
        expect(systemPreferences.appLevelAppearance).to.equal('light');
      });
    });

    const getPrefersColorSchemeIsDark = async (w: Electron.BrowserWindow) => {
      const isDark: boolean = await w.webContents.executeJavaScript(
        'matchMedia("(prefers-color-scheme: dark)").matches'
      );
      return isDark;
    };

    it('should override the result of prefers-color-scheme CSS media query', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: false, nodeIntegration: true } });
      await w.loadFile(path.resolve(__dirname, 'fixtures', 'blank.html'));
      await w.webContents.executeJavaScript(`
        window.matchMedia('(prefers-color-scheme: dark)')
          .addEventListener('change', () => require('electron').ipcRenderer.send('theme-change'))
      `);
      const originalSystemIsDark = await getPrefersColorSchemeIsDark(w);
      let changePromise: Promise<any[]> = emittedOnce(ipcMain, 'theme-change');
      nativeTheme.themeSource = 'dark';
      if (!originalSystemIsDark) await changePromise;
      expect(await getPrefersColorSchemeIsDark(w)).to.equal(true);
      changePromise = emittedOnce(ipcMain, 'theme-change');
      nativeTheme.themeSource = 'light';
      await changePromise;
      expect(await getPrefersColorSchemeIsDark(w)).to.equal(false);
      changePromise = emittedOnce(ipcMain, 'theme-change');
      nativeTheme.themeSource = 'system';
      if (originalSystemIsDark) await changePromise;
      expect(await getPrefersColorSchemeIsDark(w)).to.equal(originalSystemIsDark);
      w.close();
    });
  });

  describe('nativeTheme.shouldUseInvertedColorScheme', () => {
    it('returns a boolean', () => {
      expect(nativeTheme.shouldUseInvertedColorScheme).to.be.a('boolean');
    });
  });

  describe('nativeTheme.shouldUseHighContrastColors', () => {
    it('returns a boolean', () => {
      expect(nativeTheme.shouldUseHighContrastColors).to.be.a('boolean');
    });
  });

  describe('nativeTheme.inForcedColorsMode', () => {
    it('returns a boolean', () => {
      expect(nativeTheme.inForcedColorsMode).to.be.a('boolean');
    });
  });
});

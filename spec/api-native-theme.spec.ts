import { nativeTheme, BrowserWindow, ipcMain } from 'electron/main';

import { afterEach, describe, expect, it } from 'vitest';

import { once } from 'node:events';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';

import { ifdescribe, startRemoteControlApp } from './lib/spec-helpers.ts';
import { closeAllWindows } from './lib/window-helpers.ts';

describe('nativeTheme module', () => {
  describe('nativeTheme.shouldUseDarkColors', () => {
    it('returns a boolean', () => {
      expect(nativeTheme.shouldUseDarkColors).toBeTypeOf('boolean');
    });
  });

  describe('nativeTheme.themeSource', () => {
    afterEach(async () => {
      nativeTheme.themeSource = 'system';
      // Wait for any pending events to emit
      await setTimeout(20);

      await closeAllWindows();
    });

    it('is system by default', () => {
      expect(nativeTheme.themeSource).toBe('system');
    });

    it('should override the value of shouldUseDarkColors', () => {
      nativeTheme.themeSource = 'dark';
      expect(nativeTheme.shouldUseDarkColors).toBe(true);
      nativeTheme.themeSource = 'light';
      expect(nativeTheme.shouldUseDarkColors).toBe(false);
    });

    it('should emit the "updated" event when it is set and the resulting "shouldUseDarkColors" value changes', async () => {
      nativeTheme.themeSource = 'light';
      let updatedEmitted = once(nativeTheme, 'updated');
      nativeTheme.themeSource = 'dark';
      await updatedEmitted;
      updatedEmitted = once(nativeTheme, 'updated');
      nativeTheme.themeSource = 'light';
      await updatedEmitted;
    });

    it('should not emit the "updated" event when it is set and the resulting "shouldUseDarkColors" value is the same', async () => {
      nativeTheme.themeSource = 'dark';
      // Wait a few ticks to allow an async events to flush
      await setTimeout(20);
      let called = false;
      nativeTheme.once('updated', () => {
        called = true;
      });
      nativeTheme.themeSource = 'dark';
      // Wait a few ticks to allow an async events to flush
      await setTimeout(20);
      expect(called).toBe(false);
    });

    const getPrefersColorSchemeIsDark = async (w: Electron.BrowserWindow) => {
      const isDark: boolean = await w.webContents.executeJavaScript(
        'matchMedia("(prefers-color-scheme: dark)").matches'
      );
      return isDark;
    };

    it('should override the result of prefers-color-scheme CSS media query', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: false, nodeIntegration: true } });
      await w.loadFile(path.resolve(import.meta.dirname, 'fixtures', 'blank.html'));
      await w.webContents.executeJavaScript(`
        window.matchMedia('(prefers-color-scheme: dark)')
          .addEventListener('change', () => require('electron').ipcRenderer.send('theme-change'))
      `);
      const originalSystemIsDark = await getPrefersColorSchemeIsDark(w);
      let changePromise = once(ipcMain, 'theme-change');
      nativeTheme.themeSource = 'dark';
      if (!originalSystemIsDark) await changePromise;
      expect(await getPrefersColorSchemeIsDark(w)).toBe(true);
      changePromise = once(ipcMain, 'theme-change');
      nativeTheme.themeSource = 'light';
      await changePromise;
      expect(await getPrefersColorSchemeIsDark(w)).toBe(false);
      changePromise = once(ipcMain, 'theme-change');
      nativeTheme.themeSource = 'system';
      if (originalSystemIsDark) await changePromise;
      expect(await getPrefersColorSchemeIsDark(w)).toBe(originalSystemIsDark);
      w.close();
    });
  });

  describe('nativeTheme.shouldUseInvertedColorScheme', () => {
    it('returns a boolean', () => {
      expect(nativeTheme.shouldUseInvertedColorScheme).toBeTypeOf('boolean');
    });
  });

  describe('nativeTheme.shouldUseHighContrastColors', () => {
    it('returns a boolean', () => {
      expect(nativeTheme.shouldUseHighContrastColors).toBeTypeOf('boolean');
    });

    it('is reflected in the prefers-contrast CSS media query', async () => {
      // --force-high-contrast makes the native theme report a preference for
      // more contrast on every platform, regardless of the OS setting.
      const rc = await startRemoteControlApp(['--force-high-contrast']);
      const result = await rc.remotely(async () => {
        const { BrowserWindow, nativeTheme } = require('electron/main');
        const w = new BrowserWindow({ show: false });
        await w.loadURL('about:blank');
        const prefersMoreContrast = await w.webContents.executeJavaScript(
          'matchMedia("(prefers-contrast: more)").matches'
        );
        return {
          shouldUseHighContrastColors: nativeTheme.shouldUseHighContrastColors,
          prefersMoreContrast
        };
      });
      expect(result).toEqual({ shouldUseHighContrastColors: true, prefersMoreContrast: true });
    });
  });

  describe('nativeTheme.shouldUseDarkColorsForSystemIntegratedUI', () => {
    it('returns a boolean', () => {
      expect(nativeTheme.shouldUseDarkColorsForSystemIntegratedUI).toBeTypeOf('boolean');
    });
  });

  describe('nativeTheme.inForcedColorsMode', () => {
    it('returns a boolean', () => {
      expect(nativeTheme.inForcedColorsMode).toBeTypeOf('boolean');
    });
  });

  describe('nativeTheme.prefersReducesTransparency', () => {
    it('returns a boolean', () => {
      expect(nativeTheme.prefersReducedTransparency).toBeTypeOf('boolean');
    });
  });

  ifdescribe(process.platform === 'darwin')('nativeTheme.shouldDifferentiateWithoutColor', () => {
    it('returns a boolean', () => {
      expect(nativeTheme.shouldDifferentiateWithoutColor).toBeTypeOf('boolean');
    });
  });
});

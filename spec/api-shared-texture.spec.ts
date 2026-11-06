import { BaseWindow, BrowserWindow, ipcMain, nativeImage, sharedTexture } from 'electron';

import { afterEach, describe, expect, it } from 'vitest';

import { once } from 'node:events';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';

import { defer, ifdescribe } from './lib/spec-helpers.ts';
import { closeWindow } from './lib/window-helpers.ts';

const fixtures = path.resolve(import.meta.dirname, 'fixtures');

// Tests only run properly on macOS arm64 for now
const skip = process.platform !== 'darwin' || process.arch !== 'arm64';
ifdescribe(!skip)('sharedTexture module', { retry: 0 }, () => {
  const dirPath = path.join(fixtures, 'api', 'shared-texture');
  const osrPath = path.join(dirPath, 'osr.html');
  const targetImage = nativeImage.createFromPath(path.join(dirPath, 'image.png'));

  afterEach(async () => {
    ipcMain.removeAllListeners();
    for (const w of BaseWindow.getAllWindows()) {
      await closeWindow(w);
    }
  });

  const captureTexture = async () => {
    const osr = new BrowserWindow({
      width: 128,
      height: 128,
      show: false,
      webPreferences: {
        offscreen: { useSharedTexture: true }
      }
    });
    osr.webContents.stopPainting();
    await osr.loadFile(osrPath);

    return new Promise<Electron.OffscreenSharedTexture>((resolve, reject) => {
      let captured = false;
      const onPaint = (event: Electron.Event<Electron.WebContentsPaintEventParams>) => {
        const texture = event.texture;
        if (captured) {
          texture?.release();
          return;
        }
        captured = true;
        osr.webContents.stopPainting();
        if (!texture) {
          reject(new Error('Offscreen paint did not produce a shared texture'));
          return;
        }
        resolve(texture);
      };
      osr.webContents.on('paint', onPaint);
      defer(() => {
        osr.webContents.removeListener('paint', onPaint);
      });
      osr.webContents.startPainting();
    });
  };

  const runSharedTextureTest = async (api: 'subtle' | 'managed', iframe = false) => {
    const win = new BrowserWindow({
      width: 256,
      height: 256,
      show: false,
      webPreferences: {
        preload: path.join(dirPath, api, 'preload.js'),
        nodeIntegrationInSubFrames: iframe,
        backgroundThrottling: false
      }
    });

    const rendererError = new Promise<never>((_resolve, reject) => {
      ipcMain.on('shared-texture-error', (_event, error: string) => reject(new Error(error)));
      win.webContents.on('preload-error', (_event, preloadPath, error) => {
        reject(new Error(`Preload failed: ${preloadPath}`, { cause: error }));
      });
    });

    const run = async () => {
      const ready = once(ipcMain, 'shared-texture-ready');
      const [[readyEvent]] = await Promise.all([
        ready,
        win.loadFile(path.join(dirPath, api, iframe ? 'frame.html' : 'index.html'))
      ]);
      const event: Electron.IpcMainEvent = readyEvent;
      const targetFrame = event.senderFrame;
      expect(targetFrame).toBe(iframe ? win.webContents.mainFrame.frames[0] : win.webContents.mainFrame);
      if (!targetFrame) throw new Error('Target frame not found');

      const texture = await captureTexture();
      if (api === 'subtle') {
        const imported = sharedTexture.subtle.importSharedTexture(texture.textureInfo);
        const done = once(ipcMain, 'shared-texture-done');
        targetFrame.send('shared-texture', imported.startTransferSharedTexture());
        try {
          await done;
        } finally {
          await new Promise<void>((resolve) => imported.release(resolve));
          texture.release();
        }
      } else {
        let releaseSource: () => void;
        const released = new Promise<void>((resolve) => {
          releaseSource = resolve;
        });
        const imported = sharedTexture.importSharedTexture({
          textureInfo: texture.textureInfo,
          allReferencesReleased: () => {
            texture.release();
            releaseSource();
          }
        });
        try {
          await sharedTexture.sendSharedTexture({ frame: targetFrame, importedSharedTexture: imported });
        } finally {
          imported.release();
        }
        await released;
      }

      const captured = await win.webContents.capturePage({ x: 16, y: 16, width: 128, height: 128 });
      const verified = once(ipcMain, 'verify-captured-image-done');
      targetFrame.send('verify-captured-image', {
        captured: captured.toDataURL(),
        target: targetImage.toDataURL()
      });
      const [, result] = await verified;
      // Allow tiny color-space and device-pixel-ratio resizing differences, not missing frames.
      expect(result.total).toBeGreaterThan(0);
      expect(result.difference / result.total).toBeLessThan(0.01);
    };

    await Promise.race([run(), rendererError]);
  };

  describe('import shared texture produced by osr', () => {
    it('successfully imported and rendered with subtle api', { timeout: 10000 }, () => runSharedTextureTest('subtle'));

    it('successfully imported and rendered with managed api, without iframe', { timeout: 10000 }, () =>
      runSharedTextureTest('managed')
    );

    it('keeps subtle methods usable after the imported object is garbage collected', { timeout: 10000 }, async () => {
      const v8Util = process._linkedBinding('electron_common_v8_util');
      const texture = await captureTexture();
      try {
        // Retain only the method closures and let the returned object become unreachable.
        const retained = (() => {
          const imported = sharedTexture.subtle.importSharedTexture(texture.textureInfo);
          return {
            getFrameCreationSyncToken: imported.getFrameCreationSyncToken,
            release: imported.release
          };
        })();

        for (let i = 0; i < 3; i++) {
          await setTimeout(10);
          v8Util.requestGarbageCollectionForTesting();
        }
        await setTimeout(10);

        expect(retained.getFrameCreationSyncToken()).toHaveProperty('syncToken', expect.any(String));
        retained.release();
        // A second release must be a no-op rather than a crash.
        expect(() => retained.release()).not.toThrow();
        expect(() => retained.getFrameCreationSyncToken()).toThrow(/released/);
      } finally {
        texture.release();
      }
    });

    it('successfully imported and rendered with managed api, with iframe', { timeout: 10000 }, () =>
      runSharedTextureTest('managed', true)
    );
  });
});

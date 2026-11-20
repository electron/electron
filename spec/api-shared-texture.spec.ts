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
const TEST_TIMEOUT_MS = Number(process.env.ELECTRON_SPEC_TIMEOUT) || 30_000;
const STEP_TIMEOUT_MS = 20_000;
const TEST_TIMEOUT_MARGIN_MS = 2_000;

// Each wait gets a labeled deadline that always expires before the test's own timeout, so a
// stall fails with the step that stalled, and how long the earlier steps took, instead of a
// bare test timeout.
const createStepper = () => {
  const testDeadline = Date.now() + TEST_TIMEOUT_MS - TEST_TIMEOUT_MARGIN_MS;
  const completed: string[] = [];
  return async <T>(description: string, promise: Promise<T>): Promise<T> => {
    const start = Date.now();
    const timeoutMs = Math.max(0, Math.min(STEP_TIMEOUT_MS, testDeadline - start));
    let timer: ReturnType<typeof globalThis.setTimeout> | undefined;
    const deadline = new Promise<never>((_resolve, reject) => {
      timer = globalThis.setTimeout(() => {
        const earlier = completed.length > 0 ? ` (earlier steps: ${completed.join(', ')})` : '';
        reject(new Error(`Timed out after ${Date.now() - start}ms waiting for ${description}${earlier}`));
      }, timeoutMs);
    });
    try {
      return await Promise.race([promise, deadline]);
    } finally {
      globalThis.clearTimeout(timer);
      completed.push(`${description}: ${Date.now() - start}ms`);
    }
  };
};

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
    // The load event can fire before the decoded image reaches a compositor frame, and
    // startPainting() captures whatever frame is current, so wait for the image to be presented.
    await osr.webContents.executeJavaScript(`(async () => {
      await document.querySelector('img').decode();
      await new Promise((resolve) => requestAnimationFrame(() => requestAnimationFrame(resolve)));
    })()`);

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
    const step = createStepper();
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

    const verifyCapture = async (targetFrame: Electron.WebFrameMain) => {
      const captured = await step(
        'capturePage() of the receiving window',
        win.webContents.capturePage({ x: 16, y: 16, width: 128, height: 128 })
      );
      const verified = once(ipcMain, 'verify-captured-image-done');
      targetFrame.send('verify-captured-image', {
        captured: captured.toDataURL(),
        target: targetImage.toDataURL()
      });
      const [, result] = await step('the renderer to compare the captured image', verified);
      // Allow tiny color-space and device-pixel-ratio resizing differences, not missing frames.
      expect(result.total).toBeGreaterThan(0);
      expect(result.difference / result.total).toBeLessThan(0.01);
    };

    const run = async () => {
      const ready = once(ipcMain, 'shared-texture-ready');
      const [[readyEvent]] = await step(
        'the receiving page to set up WebGPU',
        Promise.all([ready, win.loadFile(path.join(dirPath, api, iframe ? 'frame.html' : 'index.html'))])
      );
      const event: Electron.IpcMainEvent = readyEvent;
      const targetFrame = event.senderFrame;
      expect(targetFrame).toBe(iframe ? win.webContents.mainFrame.frames[0] : win.webContents.mainFrame);
      if (!targetFrame) throw new Error('Target frame not found');

      const texture = await step('an offscreen paint with a shared texture', captureTexture());
      // The pixels are checked as soon as the receiver has drawn the frame. Releasing the texture
      // goes through the receiver's VideoFrame teardown, GPU sync tokens and IPC, which can take
      // several seconds on a busy machine, so it is checked separately afterwards.
      const rendered = once(ipcMain, 'shared-texture-rendered');
      if (api === 'subtle') {
        const imported = sharedTexture.subtle.importSharedTexture(texture.textureInfo);
        let importReleased = false;
        const releaseImport = () =>
          new Promise<void>((resolve) => {
            importReleased = true;
            imported.release(() => {
              texture.release();
              resolve();
            });
          });
        const done = once(ipcMain, 'shared-texture-done');
        targetFrame.send('shared-texture', imported.startTransferSharedTexture());
        try {
          await step('the receiver to draw the shared texture', rendered);
          await verifyCapture(targetFrame);
          await step('the receiver to release its imported texture', done);
          await step('the main process release callback', releaseImport());
        } finally {
          if (!importReleased) releaseImport();
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
          await step(
            'sendSharedTexture() to finish',
            sharedTexture.sendSharedTexture({ frame: targetFrame, importedSharedTexture: imported })
          );
        } finally {
          imported.release();
        }
        await step('the receiver to draw the shared texture', rendered);
        await verifyCapture(targetFrame);
        await step('all references to the shared texture to be released', released);
      }
    };

    await Promise.race([run(), rendererError]);
  };

  describe('import shared texture produced by osr', () => {
    it('successfully imported and rendered with subtle api', () => runSharedTextureTest('subtle'));

    it('successfully imported and rendered with managed api, without iframe', () => runSharedTextureTest('managed'));

    it('keeps subtle methods usable after the imported object is garbage collected', async () => {
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

    it('successfully imported and rendered with managed api, with iframe', () => runSharedTextureTest('managed', true));
  });
});

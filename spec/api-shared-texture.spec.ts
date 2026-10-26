import { BaseWindow, BrowserWindow, app, ipcMain, nativeImage, sharedTexture } from 'electron';

import { afterEach, describe, expect, it } from 'vitest';

import { randomUUID } from 'node:crypto';
import * as path from 'node:path';

import { ifdescribe } from './lib/spec-helpers.ts';
import { closeWindow } from './lib/window-helpers.ts';

const fixtures = path.resolve(import.meta.dirname, 'fixtures');

// Tests only run properly on macOS arm64 for now
const skip = process.platform !== 'darwin' || process.arch !== 'arm64';
ifdescribe(!skip)('sharedTexture module', () => {
  const debugSpec = false;
  const dirPath = path.join(fixtures, 'api', 'shared-texture');
  const osrPath = path.join(dirPath, 'osr.html');
  const imagePath = path.join(dirPath, 'image.png');
  const targetImage = nativeImage.createFromPath(imagePath);

  describe('import shared texture produced by osr', () => {
    afterEach(async () => {
      ipcMain.removeAllListeners();
      for (const w of BaseWindow.getAllWindows()) {
        await closeWindow(w);
      }
    });

    it('successfully imported and rendered with subtle api', { timeout: debugSpec ? 100000 : 10000 }, async () => {
      type CapturedTextureHolder = {
        importedSubtle: Electron.SharedTextureImportedSubtle;
        texture: Electron.OffscreenSharedTexture;
      };

      const capturedTextures = new Map<string, CapturedTextureHolder>();
      const preloadPath = path.join(dirPath, 'subtle', 'preload.js');
      const htmlPath = path.join(dirPath, 'subtle', 'index.html');

      return new Promise<void>((resolve, reject) => {
        const createWindow = () => {
          const win = new BrowserWindow({
            width: 256,
            height: 256,
            show: debugSpec,
            webPreferences: {
              preload: preloadPath
            }
          });

          const osr = new BrowserWindow({
            width: 128,
            height: 128,
            show: debugSpec,
            webPreferences: {
              offscreen: {
                useSharedTexture: true
              }
            }
          });

          osr.webContents.setFrameRate(1);
          osr.webContents.on('paint', (event: any) => {
            // Step 1: Input source of shared texture handle.
            const texture = event.texture;

            if (!texture) {
              console.error('No texture, GPU may be unavailable, skipping.');
              resolve();
            }

            // Step 2: Import as SharedTextureImported
            const importedSubtle = sharedTexture.subtle.importSharedTexture(texture.textureInfo);

            // Step 3: Prepare for transfer to another process (win's renderer)
            const transfer = importedSubtle.startTransferSharedTexture();

            const id = randomUUID();
            capturedTextures.set(id, { importedSubtle, texture });

            // Step 4: Send the shared texture to the renderer process (goto preload.js)
            win.webContents.send('shared-texture', id, transfer);
          });

          ipcMain.on('shared-texture-done', (event: any, id: string) => {
            // Step 12: Release the shared texture resources at main process
            const data = capturedTextures.get(id);
            if (data) {
              capturedTextures.delete(id);
              const { importedSubtle, texture } = data;

              // Step 13: Release the imported shared texture
              importedSubtle.release(() => {
                // Step 14: Release the shared texture once GPU is done
                texture.release();
              });

              // Step 15: Slightly timeout and capture the node screenshot
              setTimeout(async () => {
                // Step 16: Compare the captured image with the target image
                const captured = await win.webContents.capturePage({
                  x: 16,
                  y: 16,
                  width: 128,
                  height: 128
                });

                // Step 17: Resize the target image to match the captured image size, in case dpr != 1
                const target = targetImage.resize({ ...captured.getSize() });

                // Step 18: nativeImage have error comparing pixel data when color space is different,
                // send to browser for comparison using canvas.
                win.webContents.send('verify-captured-image', {
                  captured: captured.toDataURL(),
                  target: target.toDataURL()
                });
              }, 300);
            }
          });

          ipcMain.on('verify-captured-image-done', (event: any, result: { difference: number; total: number }) => {
            // Step 22: Verify the result from renderer process
            try {
              // macOS may have tiny color difference after the whole rendering process,
              // and the color may change slightly when resizing at device pixel ratio != 1.
              // Limit error should not be different more than 1% of the whole image.
              const ratio = result.difference / result.total;
              expect(ratio).toBeLessThan(0.01);
              resolve();
            } catch (e) {
              reject(e);
            }
          });

          ipcMain.on('webgpu-unavailable', () => {
            console.error('WebGPU is not available, skipping.');
            resolve();
          });

          win.loadFile(htmlPath);
          osr.loadFile(osrPath);
        };

        app.whenReady().then(() => {
          createWindow();
        });
      });
    });

    const runSharedTextureManagedTest = async (iframe: boolean, signal: AbortSignal): Promise<void> => {
      const preloadPath = path.join(dirPath, 'managed', 'preload.js');
      const htmlPath = path.join(dirPath, 'managed', iframe ? 'frame.html' : 'index.html');

      // A timeout only says the test ran out of time, so log which step it
      // was waiting on.
      let step = 'creating windows';
      signal.addEventListener('abort', () => {
        console.error(`sharedTexture managed api test (iframe: ${iframe}) timed out while ${step}`);
      });

      const win = new BrowserWindow({
        width: 256,
        height: 256,
        show: debugSpec,
        webPreferences: {
          preload: preloadPath,
          nodeIntegrationInSubFrames: iframe
        }
      });

      const osr = new BrowserWindow({
        width: 128,
        height: 128,
        show: debugSpec,
        webPreferences: {
          offscreen: {
            useSharedTexture: true
          }
        }
      });

      // The receiving page registers its shared texture receiver and then
      // initializes WebGPU asynchronously. A texture sent before it reports
      // ready is either delivered to no receiver (and never released) or
      // reaches a page that cannot render it yet, so don't produce any OSR
      // frame until then.
      step = 'waiting for the receiving page to initialize WebGPU';
      const receiverReady = new Promise<boolean>((resolve) => {
        win.webContents.ipc.once('webgpu-ready', () => resolve(true));
        win.webContents.ipc.once('webgpu-unavailable', () => resolve(false));
      });
      await win.loadFile(htmlPath);

      const targetFrame = iframe ? win.webContents.mainFrame.frames[0] : win.webContents.mainFrame;
      expect(targetFrame, 'target frame').toBeDefined();
      // Without the preload API the page can never report ready.
      expect(
        await targetFrame.executeJavaScript("typeof window.textures === 'object'"),
        'preload API exposed in the receiving frame'
      ).toBe(true);

      if (!(await receiverReady)) {
        console.error('WebGPU is not available, skipping.');
        return;
      }

      const renderFailed = new Promise<never>((resolve, reject) => {
        win.webContents.ipc.once('render-failed', (event, message: string) => {
          reject(new Error(`The receiving page failed to render the shared texture: ${message}`));
        });
      });
      renderFailed.catch(() => {});

      // Hand out one OSR frame at a time. Frames nobody is waiting for are
      // released right away so they don't drain the OSR frame pool.
      let paintWaiter: ((texture: Electron.OffscreenSharedTexture | null) => void) | null = null;
      osr.webContents.on('paint', (event: any) => {
        const texture: Electron.OffscreenSharedTexture | undefined = event.texture;
        if (paintWaiter) {
          const waiter = paintWaiter;
          paintWaiter = null;
          waiter(texture ?? null);
        } else {
          texture?.release();
        }
      });
      const nextPaint = () =>
        new Promise<Electron.OffscreenSharedTexture | null>((resolve) => {
          paintWaiter = resolve;
        });

      const renderAndCompare = async (texture: Electron.OffscreenSharedTexture) => {
        // Import the texture, transfer it to the receiving frame, which draws
        // it with WebGPU and releases it once the result has been presented.
        let imported!: Electron.SharedTextureImported;
        const allReferencesReleased = new Promise<void>((resolve) => {
          imported = sharedTexture.importSharedTexture({
            textureInfo: texture.textureInfo,
            allReferencesReleased: () => {
              // Release the shared texture source once every process is done with it.
              texture.release();
              resolve();
            }
          });
        });
        step = 'transferring the shared texture';
        try {
          await sharedTexture.sendSharedTexture({ frame: targetFrame, importedSharedTexture: imported });
        } finally {
          imported.release();
        }
        step = 'waiting for the receiving page to render and release the shared texture';
        await Promise.race([allReferencesReleased, renderFailed]);

        step = 'capturing the receiving window';
        const captured = await win.webContents.capturePage({
          x: 16,
          y: 16,
          width: 128,
          height: 128
        });

        // Resize the target image to match the captured image size, in case dpr != 1
        const target = targetImage.resize({ ...captured.getSize() });

        // nativeImage have error comparing pixel data when color space is different,
        // send to browser for comparison using canvas.
        step = 'comparing the captured image in the receiving page';
        const result = await new Promise<{ difference: number; total: number }>((resolve) => {
          win.webContents.ipc.once('verify-captured-image-done', (event, result) => resolve(result));
          targetFrame.send('verify-captured-image', {
            captured: captured.toDataURL(),
            target: target.toDataURL()
          });
        });
        return result.difference / result.total;
      };

      osr.webContents.setFrameRate(1);
      step = 'waiting for the first OSR paint';
      let paint = nextPaint();
      await osr.loadFile(osrPath);

      // osr.html is static, so OSR usually paints it only once or twice. A
      // frame painted before its image is drawn doesn't match the target
      // image, so compare a few OSR frames until one matches. Mismatching
      // every one of them is a real failure and is reported as such.
      // macOS may have tiny color difference after the whole rendering process,
      // and the color may change slightly when resizing at device pixel ratio != 1.
      // Limit error should not be different more than 1% of the whole image.
      const maxFrames = 3;
      const ratios: number[] = [];
      for (let i = 0; i < maxFrames; i++) {
        const texture = await paint;
        if (!texture) {
          console.error('No texture, GPU may be unavailable, skipping.');
          return;
        }
        const ratio = await renderAndCompare(texture);
        if (ratio < 0.01) return;
        ratios.push(ratio);

        // A static page is only painted again when it changes, and
        // invalidate() doesn't produce a shared texture, so restart painting
        // to have the capturer deliver a fresh frame of the current content.
        step = `waiting for OSR paint ${i + 2}`;
        paint = nextPaint();
        osr.webContents.stopPainting();
        osr.webContents.startPainting();
      }
      throw new Error(
        `None of ${maxFrames} OSR frames rendered as the target image, difference ratios: ${ratios.join(', ')}`
      );
    };

    it(
      'successfully imported and rendered with managed api, without iframe',
      { timeout: debugSpec ? 100000 : 10000 },
      ({ signal }) => runSharedTextureManagedTest(false, signal)
    );

    it(
      'keeps subtle methods usable after the imported object is garbage collected',
      { timeout: debugSpec ? 100000 : 10000 },
      async () => {
        const v8Util = process._linkedBinding('electron_common_v8_util');
        const setTimeoutAsync = (ms: number) => new Promise<void>((resolve) => setTimeout(resolve, ms));

        const osr = new BrowserWindow({
          width: 128,
          height: 128,
          show: debugSpec,
          webPreferences: {
            offscreen: {
              useSharedTexture: true
            }
          }
        });
        osr.webContents.setFrameRate(1);

        const texture = await new Promise<Electron.OffscreenSharedTexture | null>((resolve) => {
          osr.webContents.once('paint', (event: any) => resolve(event.texture ?? null));
          osr.loadFile(osrPath);
        });
        if (!texture) {
          console.error('No texture, GPU may be unavailable, skipping.');
          return;
        }

        // Retain only the method closures and let the returned object become unreachable.
        const retained = (() => {
          const imported = sharedTexture.subtle.importSharedTexture(texture.textureInfo);
          return {
            getFrameCreationSyncToken: imported.getFrameCreationSyncToken,
            release: imported.release
          };
        })();

        for (let i = 0; i < 3; i++) {
          await setTimeoutAsync(10);
          v8Util.requestGarbageCollectionForTesting();
        }
        await setTimeoutAsync(10);

        expect(retained.getFrameCreationSyncToken()).toHaveProperty('syncToken', expect.any(String));
        retained.release();
        // A second release must be a no-op rather than a crash.
        expect(() => retained.release()).not.toThrow();
        expect(() => retained.getFrameCreationSyncToken()).toThrow(/released/);
        texture.release();
      }
    );

    it(
      'successfully imported and rendered with managed api, with iframe',
      { timeout: debugSpec ? 100000 : 10000 },
      ({ signal }) => runSharedTextureManagedTest(true, signal)
    );
  });
});

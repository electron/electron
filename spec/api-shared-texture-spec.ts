import { BaseWindow } from 'electron';

import { expect } from 'chai';

import { randomUUID } from 'node:crypto';
import * as path from 'node:path';

import { ifdescribe } from './lib/spec-helpers';
import { closeWindow } from './lib/window-helpers';

const fixtures = path.resolve(__dirname, 'fixtures');

// Tests only run properly on macOS arm64 for now
const skip = process.platform !== 'darwin' || process.arch !== 'arm64';
ifdescribe(!skip)('sharedTexture module', () => {
  const {
    nativeImage
  } = require('electron');

  const debugSpec = false;
  const dirPath = path.join(fixtures, 'api', 'shared-texture');
  const osrPath = path.join(dirPath, 'osr.html');
  const imagePath = path.join(dirPath, 'image.png');
  const targetImage = nativeImage.createFromPath(imagePath);

  describe('import shared texture produced by osr', () => {
    const {
      app,
      BrowserWindow,
      sharedTexture,
      ipcMain
    } = require('electron');

    afterEach(async () => {
      ipcMain.removeAllListeners();
      for (const w of BaseWindow.getAllWindows()) {
        await closeWindow(w);
      }
    });

    it('successfully imported and rendered with subtle api', async function () {
      this.timeout(debugSpec ? 100000 : 10000);
      type CapturedTextureHolder = {
        importedSubtle: Electron.SharedTextureImportedSubtle,
        texture: Electron.OffscreenSharedTexture
      }

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

          ipcMain.on('verify-captured-image-done', (event: any, result: { difference: number, total: number }) => {
          // Step 22: Verify the result from renderer process
            try {
            // macOS may have tiny color difference after the whole rendering process,
            // and the color may change slightly when resizing at device pixel ratio != 1.
            // Limit error should not be different more than 1% of the whole image.
              const ratio = result.difference / result.total;
              expect(ratio).to.be.lessThan(0.01);
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

    const runSharedTextureManagedTest = (iframe: boolean): Promise<void> => {
      const preloadPath = path.join(dirPath, 'managed', 'preload.js');
      const htmlPath = path.join(dirPath, 'managed', iframe ? 'frame.html' : 'index.html');

      return new Promise<void>((resolve, reject) => {
        const createWindow = () => {
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

          osr.webContents.setFrameRate(1);
          osr.webContents.on('paint', async (event: any) => {
            const targetFrame = iframe ? win.webContents.mainFrame.frames[0] : win.webContents.mainFrame;
            if (!targetFrame) {
              reject(new Error('Target frame not found'));
              return;
            }

            // Step 1: Input source of shared texture handle.
            const texture = event.texture;

            if (!texture) {
              console.error('No texture, GPU may be unavailable, skipping.');
              resolve();
              return;
            }

            // Step 2: Import as SharedTextureImported
            const imported = sharedTexture.importSharedTexture({
              textureInfo: texture.textureInfo,
              allReferencesReleased: () => {
              // Release the shared texture source once GPU is done.
              // Will be called when all processes have finished using the shared texture.
                texture.release();

                // Slightly timeout and capture the node screenshot
                setTimeout(async () => {
                // Compare the captured image with the target image
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
                  targetFrame.send('verify-captured-image', {
                    captured: captured.toDataURL(),
                    target: target.toDataURL()
                  });
                }, 300);
              }
            });

            // Step 3: Transfer to another process (win's renderer)
            await sharedTexture.sendSharedTexture({
              frame: iframe ? targetFrame : win.webContents.mainFrame,
              importedSharedTexture: imported
            });

            // Step 4: Release the imported and wait for signal to release the source
            imported.release();
          });

          ipcMain.on('verify-captured-image-done', (event: any, result: { difference: number, total: number }) => {
          // Verify the result from renderer process
            try {
            // macOS may have tiny color difference after the whole rendering process,
            // and the color may change slightly when resizing at device pixel ratio != 1.
            // Limit error should not be different more than 1% of the whole image.
              const ratio = result.difference / result.total;
              expect(ratio).to.be.lessThan(0.01);
              resolve();
            } catch (e) {
              setTimeout(() => reject(e), 1000000);
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
    };

    it('successfully imported and rendered with managed api, without iframe', async () => {
      return runSharedTextureManagedTest(false);
    }).timeout(debugSpec ? 100000 : 10000);

    it('successfully imported and rendered with managed api, with iframe', async () => {
      return runSharedTextureManagedTest(true);
    }).timeout(debugSpec ? 100000 : 10000);
  });
});

import { BaseWindow } from 'electron';

import { expect } from 'chai';

import { randomUUID } from 'node:crypto';
import * as path from 'node:path';

import { closeWindow } from './lib/window-helpers';

const fixtures = path.resolve(__dirname, 'fixtures');

describe('sharedTexture module', () => {
  describe('import shared texture produced by osr', () => {
    afterEach(async () => {
      for (const w of BaseWindow.getAllWindows()) {
        await closeWindow(w);
      }
    });

    const debugSpec = false;
    it('successfully imported and rendered', (done) => {
      const {
        app,
        BrowserWindow,
        sharedTexture,
        ipcMain,
        nativeImage
      } = require('electron');

      const capturedTextures = new Map<string, any>();
      const dirPath = path.join(fixtures, 'api', 'shared-texture');
      const preloadPath = path.join(dirPath, 'preload.js');
      const htmlPath = path.join(dirPath, 'index.html');
      const osrPath = path.join(dirPath, 'osr.html');
      const imagePath = path.join(dirPath, 'image.png');
      const targetImage = nativeImage.createFromPath(imagePath);

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
        osr.webContents.on('paint', (event) => {
          // Step 1: Input source of shared texture handle.
          const texture = event.texture;

          if (!texture) {
            console.error('No texture, GPU may be unavailable, skipping.');
            done();
            return;
          }

          // Step 2: Import as SharedTextureImported
          console.log(texture.textureInfo);
          const imported = sharedTexture.importSharedTexture(texture.textureInfo);

          // Step 3: Prepare for transfer to another process (win's renderer)
          const transfer = imported.startTransferSharedTexture();

          const id = randomUUID();
          capturedTextures.set(id, { imported, texture });

          // Step 4: Send the shared texture to the renderer process (goto preload.js)
          win.webContents.send('shared-texture', id, transfer);
        });

        ipcMain.on('shared-texture-done', (event: any, id: string) => {
          // Step 12: Release the shared texture resources at main process
          const data = capturedTextures.get(id);
          if (data) {
            capturedTextures.delete(id);
            const { imported, texture } = data;

            // Step 13: Release the imported shared texture
            imported.release(() => {
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
            console.log('image difference: ', ratio);
            expect(ratio).to.be.lessThan(0.01);
            done();
          } catch (e) {
            done(e);
          }
        });

        ipcMain.on('webgpu-unavailable', () => {
          console.error('WebGPU is not available, skipping.');
          done();
        });

        win.loadFile(htmlPath);
        osr.loadFile(osrPath);
      };

      app.whenReady().then(() => {
        createWindow();
      });
    }).timeout(debugSpec ? 100000 : 10000);
  });
});

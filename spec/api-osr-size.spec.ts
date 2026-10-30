import { BrowserWindow } from 'electron';

import { afterEach, describe, expect, it } from 'vitest';

import { closeWindow } from './lib/window-helpers.ts';

// OSR has no OS window to clamp the root display surface size, so an
// oversized size provided by the embedder would be passed to GPU composition
// unchanged. Once either dimension exceeds the backend maxRenderTargetSize
// (16384 on D3D11 and Metal), the Skia characterization becomes invalid and
// the GPU process crashes. SetSize clamps such sizes.
describe('offscreen window size clamping', () => {
  afterEach(async () => {
    for (const w of BrowserWindow.getAllWindows()) {
      await closeWindow(w);
    }
  });

  it('clamps oversized OSR window sizes to the max render target size', async () => {
    const win = new BrowserWindow({
      width: 800,
      height: 600,
      show: false,
      webPreferences: {
        offscreen: true
      }
    });

    win.setSize(30000, 30000);

    // The resize is propagated to the render widget host view asynchronously.
    await new Promise((resolve) => setTimeout(resolve, 250));

    const [width, height] = win.webContents.getSize();
    expect(width).toBeLessThanOrEqual(16384);
    expect(height).toBeLessThanOrEqual(16384);
  });
});

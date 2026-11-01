import { BrowserWindow } from 'electron';

import { afterEach, describe, expect, it } from 'vitest';

import { closeWindow } from './lib/window-helpers.ts';

// OSR has no OS window to clamp the root display surface size, so an
// oversized size provided by the embedder would be passed to GPU composition
// unchanged. Once either dimension exceeds the backend max render target size
// (16384 on D3D11/Metal, 8192 on D3D feature level 10), the Skia
// characterization becomes invalid and the GPU process crashes. SetSize clamps
// such sizes.
describe('offscreen window size clamping', () => {
  afterEach(async () => {
    for (const w of BrowserWindow.getAllWindows()) {
      await closeWindow(w);
    }
  });

  // Physical-pixel limit of the root surface. Paint images are reported in
  // physical pixels (logical size * device scale factor), so this is compared
  // directly against the paint size regardless of the device scale factor.
  const kMaxSurfaceSize = 16384;

  const waitForClampedPaint = async (
    win: BrowserWindow,
    timeoutMs = 10000
  ): Promise<Electron.NativeImage> => {
    let last: Electron.NativeImage | null = null;
    const onPaint = (
      _event: Electron.Event,
      _dirtyRect: Electron.Rectangle,
      image: Electron.NativeImage
    ) => {
      last = image;
    };
    win.webContents.on('paint', onPaint);
    try {
      await win.loadURL('about:blank');
      // Drain paints emitted by the initial load so that only paints emitted
      // after setSize() below are considered.
      const initialDeadline = Date.now() + 5000;
      while (
        Date.now() < initialDeadline &&
        (!last || last.getSize()[0] === 0)
      ) {
        await new Promise((resolve) => setTimeout(resolve, 50));
      }
      last = null;
      win.setSize(30000, 30000);
      const deadline = Date.now() + timeoutMs;
      while (Date.now() < deadline) {
        if (last) {
          const [width, height] = last.getSize();
          if (
            width > 0 &&
            width <= kMaxSurfaceSize &&
            height > 0 &&
            height <= kMaxSurfaceSize
          ) {
            return last;
          }
        }
        await new Promise((resolve) => setTimeout(resolve, 50));
      }
      throw new Error('timed out waiting for a clamped OSR paint');
    } finally {
      win.webContents.removeListener('paint', onPaint);
    }
  };

  it('clamps oversized OSR window sizes to the max render target size', async () => {
    const win = new BrowserWindow({
      width: 800,
      height: 600,
      show: false,
      webPreferences: {
        offscreen: true
      }
    });

    const image = await waitForClampedPaint(win);
    const [width, height] = image.getSize();
    expect(width).toBeGreaterThan(0);
    expect(width).toBeLessThanOrEqual(kMaxSurfaceSize);
    expect(height).toBeGreaterThan(0);
    expect(height).toBeLessThanOrEqual(kMaxSurfaceSize);
  });

  it('clamps OSR window sizes accounting for device scale factor', async () => {
    const win = new BrowserWindow({
      width: 800,
      height: 600,
      show: false,
      webPreferences: {
        offscreen: {
          deviceScaleFactor: 2
        }
      }
    });

    const image = await waitForClampedPaint(win);
    const [width, height] = image.getSize();
    expect(width).toBeGreaterThan(0);
    expect(width).toBeLessThanOrEqual(kMaxSurfaceSize);
    expect(height).toBeGreaterThan(0);
    expect(height).toBeLessThanOrEqual(kMaxSurfaceSize);
  });
});

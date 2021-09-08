import * as path from 'path';
import * as fs from 'fs';
import { screen, desktopCapturer, NativeImage } from 'electron';

const fixtures = path.resolve(__dirname, '..', 'spec', 'fixtures');

/** Chroma key green. */
export const CHROMA_COLOR_HEX = '#00b140';

const captureScreen = async (point: Electron.Point = { x: 0, y: 0 }): Promise<NativeImage> => {
  const display = screen.getDisplayNearestPoint(point);
  const sources = await desktopCapturer.getSources({ types: ['screen'], thumbnailSize: display.size });
  // Toggle to save screen captures for debugging.
  const DEBUG_CAPTURE = false;
  if (DEBUG_CAPTURE) {
    for (const source of sources) {
      await fs.promises.writeFile(path.join(fixtures, `screenshot_${source.display_id}.png`), source.thumbnail.toPNG());
    }
  }
  const screenCapture = sources.find(source => source.display_id === `${display.id}`);
  // Fails when HDR is enabled on Windows.
  // https://bugs.chromium.org/p/chromium/issues/detail?id=1247730
  if (!screenCapture) throw new Error(`Unable to find screen capture for display '${display.id}'`);
  return screenCapture.thumbnail;
};

const formatHexByte = (val: number): string => {
  const str = val.toString(16);
  return str.length === 2 ? str : `0${str}`;
};

/**
 * Get the hex color at the given point.
 *
 * NOTE: The color can be off on Windows when using a scale factor different
 * than 1x.
 */
export const colorAtPoint = async (point: Electron.Point): Promise<string> => {
  const image = await captureScreen(point);
  const pixel = image.crop({ ...point, width: 1, height: 1 });
  const [b, g, r] = pixel.toBitmap();
  return `#${formatHexByte(r)}${formatHexByte(g)}${formatHexByte(b)}`;
};

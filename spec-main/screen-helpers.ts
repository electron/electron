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
  if (!screenCapture) {
    const displayIds = sources.map(source => source.display_id);
    throw new Error(`Unable to find screen capture for display '${display.id}'\n\tAvailable displays: ${displayIds.join(', ')}`);
  }
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
  // TODO(samuelmaddock): NativeImage.toBitmap() should return the raw pixel
  // color, but it sometimes differs. Why is that?
  const [b, g, r] = pixel.toBitmap();
  return `#${formatHexByte(r)}${formatHexByte(g)}${formatHexByte(b)}`;
};

const hexToRgba = (hexColor: string) => {
  const match = hexColor.match(/^#([0-9a-fA-F]{6,8})$/);
  if (!match) return;

  const colorStr = match[1];
  return [
    parseInt(colorStr.substring(0, 2), 16),
    parseInt(colorStr.substring(2, 4), 16),
    parseInt(colorStr.substring(4, 6), 16),
    parseInt(colorStr.substring(6, 8), 16) || 0xFF
  ];
};

/** Calculate euclidian distance between colors. */
const colorDistance = (hexColorA: string, hexColorB: string) => {
  const colorA = hexToRgba(hexColorA);
  const colorB = hexToRgba(hexColorB);
  if (!colorA || !colorB) return -1;
  return Math.sqrt(
    Math.pow(colorB[0] - colorA[0], 2) +
    Math.pow(colorB[1] - colorA[1], 2) +
    Math.pow(colorB[2] - colorA[2], 2)
  );
};

/**
 * Determine if colors are similar based on distance. This can be useful when
 * comparing colors which may differ based on lossy compression.
 */
export const areColorsSimilar = (
  hexColorA: string,
  hexColorB: string,
  distanceThreshold = 90
): boolean => {
  const distance = colorDistance(hexColorA, hexColorB);
  return distance <= distanceThreshold;
};

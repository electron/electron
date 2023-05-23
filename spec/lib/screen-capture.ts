import { screen, desktopCapturer, NativeImage } from 'electron';
import { Bitmap } from './native-img';

/**
 * Captures an image of a display screen. If no display is provided, the primary
 * display will be used. Optionally, the user may provide either a target
 * `Electron.Display` or an `Electron.Point` which will be resolved to the
 * display containing that point.
 */
export async function captureScreenImg (
  targetDisplay?: Electron.Display | Electron.Point
): Promise<NativeImage> {
  // Resolve the display based on the input
  const display =
    targetDisplay === undefined
      ? screen.getPrimaryDisplay()
      : 'x' in targetDisplay && 'y' in targetDisplay
        ? screen.getDisplayNearestPoint(targetDisplay as Electron.Point)
        : (targetDisplay as Electron.Display);

  const allSources = await desktopCapturer.getSources({
    types: ['screen'],
    thumbnailSize: display.size
  });
  const source = allSources.find(
    (source) => source.display_id === display.id.toString()
  );
  if (!source) {
    throw new Error(
      `Unable to find screen capture for display '${
        display.id
      }'\n\tAvailable displays: ${allSources.join(', ')}`
    );
  }

  return source.thumbnail;
}

/**
 * The same as `captureScreenImg` from this file, but conveniently returns a
 * `Bitmap` object from the captured screen image.
 */
export async function captureScreenBitmap (
  targetDisplay?: Electron.Display | Electron.Point
): Promise<Bitmap> {
  return new Bitmap(await captureScreenImg(targetDisplay));
}

import { Color } from './color';

/**
 * Provides ergonomic and efficient access to the bitmap data of an
 * `Electron.NativeImage`.
 */
export class Bitmap {
  /**
   * The underlying bitmap as a buffer of raw image data in BGR8 format,
   * left-to-right then top-to-bottom order.
   *
   * For example, a 2x2 image with the colors white, red, green, and blue
   * arranged in clockwise order starting in the upper-left corner would be
   * encoded like this:
   *
   * ```js
   * Buffer.from([
   *   0xFF, 0xFF, 0xFF, // white, upper-left corner
   *   0x00, 0x00, 0xFF, // red, upper-right corner
   *   0xFF, 0x00, 0x00, // green, bottom-left corner
   *   0x00, 0xFF, 0x00, // blue, bottom-right corner
   * ])
   * // (NB: WRGB in clockwise order is WRBG when laid out in memory.)
   * ```
   */
  #bitmap: Buffer;
  #size: Electron.Size;

  /**
   * Get the bitmap of an `Electron.NativeImage`.
   *
   * Note: it is implicitly expected that this image's raw bitmap data is in
   * BGR8 format, left-to-right then top-to-bottom order.
   */
  constructor (img: Electron.NativeImage) {
    this.#bitmap = img.toBitmap();
    this.#size = img.getSize();
  }

  /**
   * Gets the color at a point on this bitmap.
   */
  colorAt (point: Electron.Point): Color {
    // While very succinct, there is a lot going on in this next line:
    // 1. We want to find the index of the pixel at `point`. Since pixels are
    //     in left-to-right then top-to-bottom order, the pixel at (x, y) can be
    //     found at index (x * width + y).
    // 2. Since the pixel format is BGR8, each pixel occupies 3 bytes of the
    //     buffer per pixel; thus, the start of the target pixel is at
    //     pixelIndex * 3.
    // 3. We use `.subarray` to get a 'slice' of the buffer starting at the
    //     target pixel. Importantly, `.subarray` doesn't copy the underlying
    //     data, only providing a different view of the same data. That keeps
    //     this operation quick and cheap to run.
    //     (Note: we could supply the correct end index to `.subarray` too, but
    //     it's not necessary here.)
    // 4. We deconstruct the resulting buffer to just the first three bytes.
    const [b, g, r] = this.#bitmap.subarray((point.x * this.#size.width + point.y) * 3);

    // Return the color :)
    return { r, g, b };
  }
}

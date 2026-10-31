import { type NativeImage, nativeImage } from 'electron/common';
import { BrowserWindow } from 'electron/main';

import { afterEach, assert, describe, expect, it } from 'vitest';

import * as path from 'node:path';

import { createArtifact } from './lib/artifacts.ts';
import { closeAllWindows } from './lib/window-helpers.ts';

const FIXTURE_PATH = path.resolve(import.meta.dirname, 'fixtures', 'api', 'corner-smoothing');

/**
 * Rendered images may "match" but slightly differ due to rendering artifacts
 * like anti-aliasing and vector path resolution, among others. This tolerance
 * is the cutoff for whether two images "match" or not.
 *
 * From testing, matching images were found to have an average global difference
 * up to about 1.3. The shape page rendered with the feature in the other state
 * differs from its reference by about 5.7, so the tolerance must stay well
 * below that.
 *
 * See the documentation on `getAverageImageDifference` for more information.
 */
const COMPARISON_TOLERANCE = 2.5;

/**
 * How long to keep capturing new frames of the page while they do not match
 * the expected image. See `pageCaptureTestRecipe`.
 */
const CAPTURE_TIMEOUT = 5000;

/**
 * Returns the average global difference of two images, which determines if
 * they are similar enough to "match" each other.
 *
 * "Average global difference" is the average difference of pixel components
 * (RGB each) across an entire image.
 *
 * The cutoff for matching/not-matching is defined by the `COMPARISON_TOLERANCE`
 * constant.
 */
function getAverageImageDifference(img1: NativeImage, img2: NativeImage): number {
  expect(
    img1.getSize(),
    'Cannot compare images with different sizes. Run tests with --force-device-scale-factor=1'
  ).toEqual(img2.getSize());

  const bitmap1 = img1.toBitmap();
  const bitmap2 = img2.toBitmap();

  const { width, height } = img1.getSize();

  let totalDiff = 0;
  for (let x = 0; x < width; x++) {
    for (let y = 0; y < height; y++) {
      const index = (x + y * width) * 4;
      const pixel1 = bitmap1.subarray(index, index + 4);
      const pixel2 = bitmap2.subarray(index, index + 4);
      const diff = Math.abs(pixel1[0] - pixel2[0]) + Math.abs(pixel1[1] - pixel2[1]) + Math.abs(pixel1[2] - pixel2[2]);
      totalDiff += diff;
    }
  }

  return totalDiff / (width * height);
}

/**
 * Recipe for tests.
 *
 * The page is rendered, captured as an image, then compared to an expected
 * result image.
 */
async function pageCaptureTestRecipe(
  pagePath: string,
  expectedImgPath: string,
  artifactName: string,
  cornerSmoothingAvailable: boolean = true
): Promise<void> {
  const w = new BrowserWindow({
    show: false,
    width: 800,
    height: 600,
    useContentSize: true,
    webPreferences: {
      disableBlinkFeatures: cornerSmoothingAvailable ? undefined : 'ElectronCSSCornerSmoothing'
    }
  });
  await w.loadFile(pagePath);
  w.show();

  const expectedImg = nativeImage.createFromPath(expectedImgPath);
  expect(expectedImg.isEmpty(), 'Failed to read expected reference image').toBe(false);

  // The window paints while it is hidden and the page is still loading.
  // One animation frame after showing the window, the renderer may not have
  // submitted the frame for the loaded page yet, so a capture can still get
  // a frame of a partly parsed page or a partly loaded image. Capture new
  // frames until one matches the reference or time runs out.
  // The page is static, so captures converge on its final rendering.
  const deadline = Date.now() + CAPTURE_TIMEOUT;
  let actualImg: NativeImage;
  let difference: number;
  let captures = 0;
  do {
    // Wait for a render frame to prepare the page.
    await w.webContents.executeJavaScript('new Promise((resolve) => { requestAnimationFrame(() => resolve()); })');

    actualImg = await w.webContents.capturePage();
    captures++;
    expect(actualImg.isEmpty(), 'Failed to capture page image').toBe(false);

    difference = getAverageImageDifference(actualImg, expectedImg);
  } while (difference > COMPARISON_TOLERANCE && Date.now() < deadline);

  if (difference > COMPARISON_TOLERANCE) {
    const artifactFileName = `corner-rounding-expected-${artifactName}.png`;
    await createArtifact(artifactFileName, actualImg.toPNG());

    assert.fail(
      `Actual image did not match expected reference image (average difference ${difference.toFixed(2)} after ${captures} capture${captures === 1 ? '' : 's'}, tolerance ${COMPARISON_TOLERANCE}). Actual: "${artifactFileName}" in artifacts, Expected: "${path.relative(
        path.resolve(import.meta.dirname, '..'),
        expectedImgPath
      )}" in source`
    );
  }
}

describe('-electron-corner-smoothing', () => {
  afterEach(async () => {
    await closeAllWindows();
  });

  describe('shape', () => {
    for (const available of [true, false]) {
      it(`matches the reference with web preference = ${available}`, async () => {
        await pageCaptureTestRecipe(
          path.join(FIXTURE_PATH, 'shape', 'test.html'),
          path.join(FIXTURE_PATH, 'shape', `expected-${available}.png`),
          `shape-${available}`,
          available
        );
      });
    }
  });

  describe('system-ui keyword', () => {
    const { platform } = process;
    it(`matches the reference for platform = ${platform}`, async () => {
      await pageCaptureTestRecipe(
        path.join(FIXTURE_PATH, 'system-ui-keyword', 'test.html'),
        path.join(FIXTURE_PATH, 'system-ui-keyword', `expected-${platform}.png`),
        `system-ui-${platform}`
      );
    });
  });
});

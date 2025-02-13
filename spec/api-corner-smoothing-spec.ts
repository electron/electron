import { nativeImage } from 'electron/common';
import { BrowserWindow, screen } from 'electron/main';

import { AssertionError, expect } from 'chai';

import path = require('node:path');

import { createArtifact } from './lib/artifacts';
import { closeAllWindows } from './lib/window-helpers';

const FIXTURE_PATH = path.resolve(
  __dirname,
  'fixtures',
  'api',
  'corner-smoothing'
);

/** Recipe for tests. */
async function test (
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
      cornerSmoothingCSS: cornerSmoothingAvailable
    }
  });
  await w.loadFile(pagePath);
  w.show();

  // Wait for a render frame to prepare the page.
  await w.webContents.executeJavaScript(
    'new Promise((resolve) => { requestAnimationFrame(() => resolve()); })'
  );

  let actualImg = await w.webContents.capturePage();
  expect(actualImg.isEmpty()).to.be.false('Failed to capture page image');

  // Resize the image to a 1.0 scale factor
  const [x, y] = w.getPosition();
  const display = screen.getDisplayNearestPoint({ x, y });
  if (display.scaleFactor !== 1) {
    const { width, height } = actualImg.getSize();
    actualImg = actualImg.resize({
      width: width / 2.0,
      height: height / 2.0
    });
  }

  const expectedImg = nativeImage.createFromPath(expectedImgPath);
  if (expectedImg.isEmpty()) {
    // TODO: remove this, just getting artifacts from CI
    const artifactFileName = `corner-rounding-expected-${artifactName}.png`;
    await createArtifact(artifactFileName, actualImg.toPNG());
    throw new AssertionError(
      `Failed to read expected reference image. Actual: "${artifactFileName}" in artifacts`
    );
  }
  expect(expectedImg.isEmpty()).to.be.false(
    'Failed to read expected reference image'
  );

  // Compare the actual page image to the expected reference image, creating an
  // artifact if they do not match.
  const matches = actualImg.toBitmap().equals(expectedImg.toBitmap());
  if (!matches) {
    const artifactFileName = `corner-rounding-expected-${artifactName}.png`;
    await createArtifact(artifactFileName, actualImg.toPNG());

    throw new AssertionError(
      `Actual image did not match expected reference image. Actual: "${artifactFileName}" in artifacts, Expected: "${path.relative(
        path.resolve(__dirname, '..'),
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
        await test(
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
      await test(
        path.join(FIXTURE_PATH, 'system-ui-keyword', 'test.html'),
        path.join(
          FIXTURE_PATH,
          'system-ui-keyword',
          `expected-${platform}.png`
        ),
        `system-ui-${platform}`
      );
    });
  });
});

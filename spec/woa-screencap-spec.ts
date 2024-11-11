import { screen } from 'electron/main';

import { createArtifactWithRandomId } from './lib/artifacts';
import { ScreenCapture } from './lib/screen-helpers';
import { ifit } from './lib/spec-helpers';

describe('trying to get screen capture on WOA', () => {
  ifit(process.platform === 'win32' && process.arch === 'arm64')('does it work', async () => {
    const display = screen.getPrimaryDisplay();
    const screenCapture = new ScreenCapture(display);
    const frame = await screenCapture.captureFrame();
    // Save the image as an artifact for better debugging
    const currentDate = new Date();
    const artifactName = await createArtifactWithRandomId(
      (id) => `screen-cap-${id}-${currentDate.toISOString()}.png`,
      frame.toPNG()
    );
    console.log(`Created artifact at: ${artifactName}`);
  });
});

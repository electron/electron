import { nativeImage } from 'electron/common';
import { BaseWindow, BrowserWindow, ImageView } from 'electron/main';

import * as path from 'node:path';

import { closeAllWindows } from './lib/window-helpers';

describe('ImageView', () => {
  afterEach(async () => {
    await closeAllWindows();
  });

  it('can be instantiated with no arguments', () => {
    // eslint-disable-next-line no-new
    new ImageView();
  });

  it('can set an empty NativeImage', () => {
    const view = new ImageView();
    const image = nativeImage.createEmpty();
    view.setImage(image);
  });

  it('can set a NativeImage', () => {
    const view = new ImageView();
    const image = nativeImage.createFromPath(path.join(__dirname, 'fixtures', 'assets', 'logo.png'));
    view.setImage(image);
  });

  it('can change its NativeImage', () => {
    const view = new ImageView();
    const image1 = nativeImage.createFromPath(path.join(__dirname, 'fixtures', 'assets', 'logo.png'));
    const image2 = nativeImage.createFromPath(path.join(__dirname, 'fixtures', 'assets', 'capybara.png'));
    view.setImage(image1);
    view.setImage(image2);
  });

  it('can be embedded in a BaseWindow', () => {
    const w = new BaseWindow();
    const view = new ImageView();
    w.setContentView(view);
  });

  it('can be embedded in a BrowserWindow', () => {
    const w = new BrowserWindow();
    const view = new ImageView();
    w.setContentView(view);
  });
});

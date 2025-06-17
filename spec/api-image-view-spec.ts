import { nativeImage } from 'electron/common';
import { BaseWindow, BrowserWindow, ImageView } from 'electron/main';

import { expect } from 'chai';

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
    const w = new BaseWindow({ show: false });
    const view = new ImageView();
    const image = nativeImage.createFromPath(path.join(__dirname, 'fixtures', 'assets', 'capybara.png'));
    view.setImage(image);
    w.setContentView(view);
    w.setContentSize(image.getSize().width, image.getSize().height);
    view.setBounds({
      x: 0,
      y: 0,
      width: image.getSize().width,
      height: image.getSize().height
    });
  });

  it('can be embedded in a BrowserWindow', () => {
    const w = new BrowserWindow({ show: false });
    const image = nativeImage.createFromPath(path.join(__dirname, 'fixtures', 'assets', 'logo.png'));
    const view = new ImageView();
    view.setImage(image);
    w.contentView.addChildView(view);
    w.setContentSize(image.getSize().width, image.getSize().height);
    view.setBounds({
      x: 0,
      y: 0,
      width: image.getSize().width,
      height: image.getSize().height
    });

    expect(w.contentView.children).to.include(view);
  });

  it('can be removed from a BrowserWindow', async () => {
    const w = new BrowserWindow({ show: false });
    const image = nativeImage.createFromPath(path.join(__dirname, 'fixtures', 'assets', 'logo.png'));
    const view = new ImageView();
    view.setImage(image);

    w.contentView.addChildView(view);
    expect(w.contentView.children).to.include(view);

    await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'blank.html'));

    w.contentView.removeChildView(view);
    expect(w.contentView.children).to.not.include(view);
  });
});

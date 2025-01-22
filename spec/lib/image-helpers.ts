import { BaseWindow, NativeImage } from 'electron';

import { once } from 'node:events';

/**
 * Opens a window to display a native image. Useful for quickly debugging tests
 * rather than writing a file and opening manually.
 */
export async function previewNativeImage (image: NativeImage) {
  const previewWindow = new BaseWindow({
    title: 'NativeImage preview',
    backgroundColor: '#444444'
  });
  const ImageView = (require('electron') as any).ImageView;
  const imgView = new ImageView();
  imgView.setImage(image);
  previewWindow.contentView.addChildView(imgView);
  imgView.setBounds({ x: 0, y: 0, ...image.getSize() });
  await once(previewWindow, 'close');
};

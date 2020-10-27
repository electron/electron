import { View } from 'electron/main';

const { ImageView } = process._linkedBinding('electron_browser_image_view');

Object.setPrototypeOf(ImageView.prototype, View.prototype);

export default ImageView;

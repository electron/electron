import { View } from 'electron';

const { ImageView } = process.electronBinding('image_view');

Object.setPrototypeOf(ImageView.prototype, View.prototype);

export default ImageView;

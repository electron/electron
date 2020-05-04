const electron = require('electron');

const { View } = electron;
const { ImageView } = process.electronBinding('image_view');

Object.setPrototypeOf(ImageView.prototype, View.prototype);

module.exports = ImageView;

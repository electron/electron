const electron = require('electron');

const { View } = electron;
const { ImageView } = process.electronBinding('image_view');

Object.setPrototypeOf(ImageView.prototype, View.prototype);

ImageView.prototype._init = function () {
  // Call parent class's _init.
  View.prototype._init.call(this);
};

module.exports = ImageView;

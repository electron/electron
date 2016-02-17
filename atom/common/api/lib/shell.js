'use strict';

const bindings = process.atomBinding('shell');

exports.beep = bindings.beep;
exports.moveItemToTrash = bindings.moveItemToTrash;
exports.openItem = bindings.openItem;
exports.showItemInFolder = bindings.showItemInFolder;

exports.openExternal = (url, options) => {
  var activate = true;
  if (options != null && options.activate != null) {
    activate = !!options.activate;
  }

  return bindings._openExternal(url, activate);
};

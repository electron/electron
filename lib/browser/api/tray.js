'use strict';

const { createTray } = process.electronBinding('tray');

function Tray (...args) {
  return createTray(...args);
}

module.exports = Tray;

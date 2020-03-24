'use strict';

const { EventEmitter } = require('events');
const { deprecate } = require('electron');
const { createTray } = process.electronBinding('tray');

function Tray (...args) {
  return createTray(...args);
}

module.exports = Tray;

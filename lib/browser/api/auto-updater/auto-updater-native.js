'use strict';

const EventEmitter = require('events').EventEmitter;
const { autoUpdater, AutoUpdater } = process._linkedBinding('electron_browser_auto_updater');

// AutoUpdater is an EventEmitter.
Object.setPrototypeOf(AutoUpdater.prototype, EventEmitter.prototype);
EventEmitter.call(autoUpdater);

module.exports = autoUpdater;

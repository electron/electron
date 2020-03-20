'use strict';

import { createLazyInstance } from '../utils';
const { EventEmitter } = require('events');
const { Screen, createScreen } = process.electronBinding('screen');

// Screen is an EventEmitter.
Object.setPrototypeOf(Screen.prototype, EventEmitter.prototype);

module.exports = createLazyInstance(createScreen, Screen, true);

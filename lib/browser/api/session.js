'use strict';

const { EventEmitter } = require('events');
const { app, deprecate } = require('electron');
const { fromPartition } = process.electronBinding('session');

// Public API.
Object.defineProperties(exports, {
  defaultSession: {
    enumerable: true,
    get () { return fromPartition(''); }
  },
  fromPartition: {
    enumerable: true,
    value: fromPartition
  }
});

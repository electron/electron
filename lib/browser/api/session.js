'use strict';

const { EventEmitter } = require('events');
const { app, deprecate } = require('electron');
const { fromPartition, Session, Cookies, Protocol } = process.electronBinding('session');

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

Object.setPrototypeOf(Cookies.prototype, EventEmitter.prototype);
Object.setPrototypeOf(Session.prototype, EventEmitter.prototype);

Session.prototype._init = function () {
  app.emit('session-created', this);
};

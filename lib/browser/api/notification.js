'use strict';

const { EventEmitter } = require('events');
const { Notification, isSupported } = process.electronBinding('notification');

Object.setPrototypeOf(Notification.prototype, EventEmitter.prototype);

Notification.isSupported = isSupported;

module.exports = Notification;

'use strict';

const { Notification, isSupported } = process.electronBinding('notification');

Notification.isSupported = isSupported;

module.exports = Notification;

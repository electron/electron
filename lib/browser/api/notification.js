'use strict';

const { createNotification, isSupported } = process.electronBinding('notification');

function Notification (...args) {
  return createNotification(...args);
}

Notification.isSupported = isSupported;

module.exports = Notification;

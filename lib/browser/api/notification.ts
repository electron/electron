const {
  Notification: ElectronNotification,
  isSupported
} = process.electronBinding('notification', 'common');

ElectronNotification.isSupported = isSupported;

export default ElectronNotification;

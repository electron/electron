const {
  Notification: ElectronNotification,
  isSupported
} = process._linkedBinding('electron_common_notification');

ElectronNotification.isSupported = isSupported;

export default ElectronNotification;

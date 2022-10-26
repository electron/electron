const {
  Notification: ElectronNotification,
  isSupported
} = process._linkedBinding('electron_browser_notification');

ElectronNotification.isSupported = isSupported;

export default ElectronNotification;

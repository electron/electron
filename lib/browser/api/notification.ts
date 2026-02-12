const {
  Notification: ElectronNotification,
  isSupported,
  getHistory
} = process._linkedBinding('electron_browser_notification');

ElectronNotification.isSupported = isSupported;
ElectronNotification.getHistory = getHistory;

export default ElectronNotification;

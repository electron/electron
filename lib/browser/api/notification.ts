const {
  Notification: ElectronNotification,
  isSupported,
  getHistory,
  remove,
  removeAll
} = process._linkedBinding('electron_browser_notification');

ElectronNotification.isSupported = isSupported;
ElectronNotification.getHistory = getHistory;
ElectronNotification.remove = remove;
ElectronNotification.removeAll = removeAll;

export default ElectronNotification;

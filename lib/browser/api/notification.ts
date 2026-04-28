const binding = process._linkedBinding('electron_browser_notification');

const ElectronNotification = binding.Notification;
ElectronNotification.isSupported = binding.isSupported;
ElectronNotification.getHistory = binding.getHistory;

if (process.platform === 'win32' && binding.handleActivation) {
  ElectronNotification.handleActivation = binding.handleActivation;
}

export default ElectronNotification;

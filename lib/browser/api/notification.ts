const { Notification: ElectronNotification, isSupported } = process.electronBinding('notification');

ElectronNotification.isSupported = isSupported;

export default ElectronNotification;

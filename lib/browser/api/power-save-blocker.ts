const { powerSaveBlocker } = process.electronBinding('power_save_blocker', 'browser');
export default powerSaveBlocker;

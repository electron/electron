const { powerSaveBlocker } = process._linkedBinding('electron_browser_power_save_blocker');
export default powerSaveBlocker;

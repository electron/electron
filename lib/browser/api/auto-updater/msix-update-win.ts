const { updateMsix, registerPackage, registerRestartOnUpdate, getPackageInfo } =
  process._linkedBinding('electron_browser_msix_updater');

export { updateMsix, registerPackage, registerRestartOnUpdate, getPackageInfo };

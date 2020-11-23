const { getWebPreference } = process._linkedBinding('electron_renderer_web_frame');

const enableRemoteModule = getWebPreference(window, 'enableRemoteModule');

// Renderer side modules, please sort alphabetically.
export const rendererModuleList: ElectronInternal.ModuleEntry[] = [
  { name: 'contextBridge', loader: () => require('./context-bridge') },
  { name: 'crashReporter', loader: () => require('./crash-reporter') },
  { name: 'ipcRenderer', loader: () => require('./ipc-renderer') },
  { name: 'nativeImage', loader: () => require('./native-image') },
  { name: 'webFrame', loader: () => require('./web-frame') }
];

if (BUILDFLAG(ENABLE_DESKTOP_CAPTURER)) {
  rendererModuleList.push({
    name: 'desktopCapturer',
    loader: () => require('@electron/internal/renderer/api/desktop-capturer')
  });
}

if (BUILDFLAG(ENABLE_REMOTE_MODULE) && enableRemoteModule) {
  rendererModuleList.push({
    name: 'remote',
    loader: () => require('@electron/internal/renderer/api/remote')
  });
}

export const moduleList: ElectronInternal.ModuleEntry[] = [
  {
    name: 'contextBridge',
    loader: () => require('@electron/internal/renderer/api/context-bridge')
  },
  {
    name: 'crashReporter',
    loader: () => require('@electron/internal/renderer/api/crash-reporter')
  },
  {
    name: 'ipcRenderer',
    loader: () => require('@electron/internal/renderer/api/ipc-renderer')
  },
  {
    name: 'nativeImage',
    loader: () => require('@electron/internal/common/api/native-image')
  },
  {
    name: 'webFrame',
    loader: () => require('@electron/internal/renderer/api/web-frame')
  },
  // The internal modules, invisible unless you know their names.
  {
    name: 'deprecate',
    loader: () => require('@electron/internal/common/api/deprecate'),
    private: true
  }
];

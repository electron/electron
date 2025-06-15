// Common modules, please sort alphabetically
export const commonModuleList: ElectronInternal.ModuleEntry[] = [
  { name: 'nativeImage', loader: () => require('./native-image') },
  { name: 'sharedTexture', loader: () => require('./shared-texture') },
  { name: 'shell', loader: () => require('./shell') }
];

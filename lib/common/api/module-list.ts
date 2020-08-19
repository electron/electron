// Common modules, please sort alphabetically
export const commonModuleList: ElectronInternal.ModuleEntry[] = [
  { name: 'clipboard', loader: () => require('./clipboard') },
  { name: 'shell', loader: () => require('./shell') },
  // The internal modules, invisible unless you know their names.
  { name: 'deprecate', loader: () => require('./deprecate'), private: true }
];

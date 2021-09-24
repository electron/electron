const handleESModule = (loader: ElectronInternal.ModuleLoader) => () => {
  const value = loader();
  if (value.__esModule && value.default) return value.default;
  return value;
};

// Attaches properties to |targetExports|.
export function defineProperties (targetExports: Object, moduleList: ElectronInternal.ModuleEntry[]) {
  const descriptors: PropertyDescriptorMap = {};
  for (const module of moduleList) {
    descriptors[module.name] = {
      enumerable: true,
      get: handleESModule(module.loader)
    };
  }
  return Object.defineProperties(targetExports, descriptors);
}

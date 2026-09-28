const handleESModule = (loader: ElectronInternal.ModuleLoader) => {
  let resolved: any;
  return () => {
    if (resolved !== undefined) return resolved;
    const value = loader();
    if (!value.__esModule) {
      resolved = value;
    } else if (value.default) {
      resolved = value.default;
    } else {
      // require() of a bundled ES module returns a fresh, read-only copy of
      // its namespace on every call; settle on a single plain object instead so
      // that the module keeps its identity and its exports can be replaced
      // (e.g. stubbed out by tests) like those of any CommonJS module.
      resolved = { ...value };
    }
    return resolved;
  };
};

// Attaches properties to |targetExports|.
export function defineProperties(targetExports: Object, moduleList: ElectronInternal.ModuleEntry[]) {
  const descriptors: PropertyDescriptorMap = {};
  for (const module of moduleList) {
    descriptors[module.name] = {
      enumerable: true,
      get: handleESModule(module.loader)
    };
  }
  return Object.defineProperties(targetExports, descriptors);
}

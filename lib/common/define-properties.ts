// Attaches properties to |targetExports|.
export function defineProperties(targetExports: any, moduleList: ElectronInternal.ModuleEntry[]) {
  const descriptors: PropertyDescriptorMap = {};
  for (const module of moduleList) {
    descriptors[module.name] = {
      configurable: true,
      enumerable: true,
      get() {
        const raw = module.loader();
        const value = (raw && raw.__esModule && raw.default) ? raw.default : raw;
        Object.defineProperty(targetExports, module.name, {
          value,
          enumerable: true,
          configurable: true,
          writable: false
        });
        return value;
      }
    };
  }
  return Object.defineProperties(targetExports, descriptors);
}

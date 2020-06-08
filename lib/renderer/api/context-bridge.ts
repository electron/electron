const { hasSwitch } = process.electronBinding('command_line');
const binding = process.electronBinding('context_bridge');

const contextIsolationEnabled = hasSwitch('context-isolation');

const checkContextIsolationEnabled = () => {
  if (!contextIsolationEnabled) throw new Error('contextBridge API can only be used when contextIsolation is enabled');
};

const contextBridge: Electron.ContextBridge = {
  exposeInMainWorld: (key: string, api: Record<string, any>) => {
    checkContextIsolationEnabled();
    return binding.exposeAPIInMainWorld(key, api);
  }
} as any;

export default contextBridge;

export const internalContextBridge = {
  contextIsolationEnabled,
  overrideGlobalValueFromIsolatedWorld: (keys: string[], value: any) => {
    return binding._overrideGlobalValueFromIsolatedWorld(keys, value, false);
  },
  overrideGlobalValueWithDynamicPropsFromIsolatedWorld: (keys: string[], value: any) => {
    return binding._overrideGlobalValueFromIsolatedWorld(keys, value, true);
  },
  overrideGlobalPropertyFromIsolatedWorld: (keys: string[], getter: Function, setter?: Function) => {
    return binding._overrideGlobalPropertyFromIsolatedWorld(keys, getter, setter || null);
  },
  isInMainWorld: () => binding._isCalledFromMainWorld() as boolean
};

if (binding._isDebug) {
  contextBridge.internalContextBridge = internalContextBridge;
}

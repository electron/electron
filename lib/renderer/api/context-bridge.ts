const binding = process._linkedBinding('electron_renderer_context_bridge');

const checkContextIsolationEnabled = () => {
  if (!process.contextIsolated) throw new Error('contextBridge API can only be used when contextIsolation is enabled');
};

const contextBridge: Electron.ContextBridge = {
  exposeInMainWorld: (key, api) => {
    checkContextIsolationEnabled();
    return binding.exposeAPIInWorld(0, key, api);
  },
  exposeInIsolatedWorld: (worldId, key, api) => {
    checkContextIsolationEnabled();
    return binding.exposeAPIInWorld(worldId, key, api);
  },
  executeInMainWorld: (script) => {
    checkContextIsolationEnabled();
    return binding.executeInWorld(0, script);
  }
};

export default contextBridge;

export const internalContextBridge = {
  contextIsolationEnabled: process.contextIsolated,
  overrideGlobalValueFromIsolatedWorld: (keys: string[], value: any) => {
    return binding._overrideGlobalValueFromIsolatedWorld(keys, value, false);
  },
  overrideGlobalValueWithDynamicPropsFromIsolatedWorld: (keys: string[], value: any) => {
    return binding._overrideGlobalValueFromIsolatedWorld(keys, value, true);
  },
  overrideGlobalPropertyFromIsolatedWorld: (keys: string[], getter: Function, setter?: Function) => {
    return binding._overrideGlobalPropertyFromIsolatedWorld(keys, getter, setter || null);
  }
};

if (binding._isDebug) {
  contextBridge.internalContextBridge = internalContextBridge;
}

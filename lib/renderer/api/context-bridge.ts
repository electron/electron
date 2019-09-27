const { hasSwitch } = process.electronBinding('command_line')
const binding = process.electronBinding('context_bridge')

const contextIsolationEnabled = hasSwitch('context-isolation')

const checkContextIsolationEnabled = () => {
  if (!contextIsolationEnabled) throw new Error('contextBridge API can only be used when contextIsolation is enabled')
}

const contextBridge = {
  exposeInMainWorld: (key: string, api: Record<string, any>) => {
    checkContextIsolationEnabled()
    return binding.exposeAPIInMainWorld(key, api)
  },
  debugGC: () => binding._debugGCMaps({})
}

if (!binding._debugGCMaps) delete contextBridge.debugGC

export default contextBridge

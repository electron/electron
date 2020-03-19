const { hasSwitch } = process.electronBinding('command_line')
const binding = process.electronBinding('context_bridge')
const v8Util = process.electronBinding('v8_util')

const contextIsolationEnabled = hasSwitch('context-isolation')

const checkContextIsolationEnabled = () => {
  if (!contextIsolationEnabled) throw new Error('contextBridge API can only be used when contextIsolation is enabled')
}

const contextBridge = {
  exposeInMainWorld: (key: string, api: Record<string, any>) => {
    checkContextIsolationEnabled()
    return binding.exposeAPIInMainWorld(key, api)
  },
  exposeSimpleObjectInMainWorld: (key: string, api: Record<string, string>) => {
    checkContextIsolationEnabled()
    return binding.exposeObjectInMainWorld(key, api)
  },
  prepareSimpleFunction: (fn: any) => {
    if (typeof fn !== 'function') {
      throw new Error(`Expected argument to be a function, instead got "${typeof fn}"`)
    }
    const wrapped = (...args: any[]) => fn(...args)
    v8Util.setHiddenValue(wrapped, '_cbSimple', true)
    return wrapped
  },
  debugGC: () => binding._debugGCMaps({})
}

if (!binding._debugGCMaps) delete contextBridge.debugGC

export default contextBridge

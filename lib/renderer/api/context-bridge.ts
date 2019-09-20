const { hasSwitch } = process.electronBinding('command_line')
const binding = process.electronBinding('context_bridge')

const reverseBindings: Record<string, any> = {}

binding._setReverseBindingStore(reverseBindings)

const contextIsolationEnabled = hasSwitch('context-isolation')

const checkContextIsolationEnabled = () => {
  if (!contextIsolationEnabled) throw new Error('contextBridge API can only be used when contextIsolation is enabled')
}

const contextBridge = {
  bindAPIInMainWorld: (key: string, api: Record<string, any>, options: { allowReverseBinding: boolean }) => {
    checkContextIsolationEnabled()
    return binding.exposeAPIInMainWorld(key, api, options)
  },
  getReverseBinding: (key: string) => {
    checkContextIsolationEnabled()
    return reverseBindings[key] || null
  }
}

export default contextBridge

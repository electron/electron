const binding = process.electronBinding('context_bridge')

const reverseBindings: Record<string, any> = {}

binding._setReverseBindingStore(reverseBindings)

const contextBridge = {
  bindAPIInMainWorld: (key: string, api: Record<string, any>, options: { allowReverseBinding: boolean }) => binding.exposeAPIInMainWorld(key, api, options),
  getReverseBinding: (key: string) => {
    return reverseBindings[key] || null
  }
}

export default contextBridge

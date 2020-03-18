import { app } from 'electron'

const { createPowerMonitor } = process.electronBinding('power_monitor')

function bindAllMethods (obj: any) {
  for (const method of Object.getOwnPropertyNames(obj)) {
    obj[method] = obj[method].bind(obj)
  }
}

let initialized = false
function initialize () {
  if (initialized) throw new Error('powerMonitor already initialized')
  const instance = createPowerMonitor()
  // The instance methods must be bound otherwise they would be invoked with
  // the Proxy as |this|, which gin finds distasteful.
  bindAllMethods(instance)
  initialized = true
  if (process.platform === 'linux') {
    // In order to delay system shutdown when e.preventDefault() is invoked
    // on a powerMonitor 'shutdown' event, we need an org.freedesktop.login1
    // shutdown delay lock. For more details see the "Taking Delay Locks"
    // section of https://www.freedesktop.org/wiki/Software/systemd/inhibit/
    //
    // So here we watch for 'shutdown' listeners to be added or removed and
    // set or unset our shutdown delay lock accordingly.
    instance.on('newListener', (event: string) => {
      // whenever the listener count is incremented to one...
      if (event === 'shutdown' && instance.listenerCount('shutdown') === 0) {
        instance.blockShutdown()
      }
    })
    instance.on('removeListener', (event: string) => {
      // whenever the listener count is decremented to zero...
      if (event === 'shutdown' && instance.listenerCount('shutdown') === 0) {
        instance.unblockShutdown()
      }
    })
  }
  return instance
}

// powerMonitor must be lazily initialized, as the native object can't be
// created until the app is ready.
const powerMonitor = new Proxy({} as Electron.PowerMonitor, {
  get (obj, prop) {
    if (!app.isReady()) throw new Error("The 'powerMonitor' module can't be used before the app is ready.")
    if (!initialized) {
      const instance = initialize()
      Object.assign(obj, instance)
    }
    return (obj as any)[prop]
  }
})

module.exports = powerMonitor

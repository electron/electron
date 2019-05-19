import { EventEmitter } from 'events'
import { createLazyInstance } from '../utils'

const { createPowerMonitor, PowerMonitor } = process.electronBinding('power_monitor')

// PowerMonitor is an EventEmitter.
Object.setPrototypeOf(PowerMonitor.prototype, EventEmitter.prototype)

const powerMonitor = createLazyInstance(createPowerMonitor, PowerMonitor, true) as Electron.PowerMonitor

// On Linux we need to call blockShutdown() to subscribe to shutdown event.
if (process.platform === 'linux') {
  const emitter = powerMonitor as NodeJS.EventEmitter

  emitter.on('newListener', (event: string) => {
    if (event === 'shutdown' && powerMonitor.listenerCount('shutdown') === 0) {
      powerMonitor.blockShutdown()
    }
  })

  emitter.on('removeListener', (event: string) => {
    if (event === 'shutdown' && powerMonitor.listenerCount('shutdown') === 0) {
      powerMonitor.unblockShutdown()
    }
  })
}

export = powerMonitor

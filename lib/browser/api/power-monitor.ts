'use strict'

import { createLazyInstance } from '../utils'

const { EventEmitter } = require('events')
const { createPowerMonitor, PowerMonitor } = process.electronBinding('power_monitor')

// PowerMonitor is an EventEmitter.
Object.setPrototypeOf(PowerMonitor.prototype, EventEmitter.prototype)

const powerMonitor = createLazyInstance(createPowerMonitor, PowerMonitor, true)

// On Linux we need to call blockShutdown() to subscribe to shutdown event.
if (process.platform === 'linux') {
  powerMonitor.on('newListener', (event:string) => {
    if (event === 'shutdown' && powerMonitor.listenerCount('shutdown') === 0) {
      powerMonitor.blockShutdown()
    }
  })

  powerMonitor.on('removeListener', (event: string) => {
    if (event === 'shutdown' && powerMonitor.listenerCount('shutdown') === 0) {
      powerMonitor.unblockShutdown()
    }
  })
}

module.exports = powerMonitor

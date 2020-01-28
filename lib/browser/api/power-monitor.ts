'use strict'

import { createLazyInstance } from '../utils'

const { EventEmitter } = require('events')
const { createPowerMonitor, PowerMonitor } = process.electronBinding('power_monitor')

// PowerMonitor is an EventEmitter.
Object.setPrototypeOf(PowerMonitor.prototype, EventEmitter.prototype)

const powerMonitor = createLazyInstance(createPowerMonitor, PowerMonitor, true)

if (process.platform === 'linux') {
  // In order to delay system shutdown when e.preventDefault() is invoked
  // on a powerMonitor 'shutdown' event, we need an org.freedesktop.login1
  // shutdown delay lock. For more details see the "Taking Delay Locks"
  // section of https://www.freedesktop.org/wiki/Software/systemd/inhibit/
  //
  // So here we watch for 'shutdown' listeners to be added or removed and
  // set or unset our shutdown delay lock accordingly.
  const { app } = require('electron')
  app.whenReady().then(() => {
    powerMonitor.on('newListener', (event: string) => {
      // whenever the listener count is incremented to one...
      if (event === 'shutdown' && powerMonitor.listenerCount('shutdown') === 0) {
        powerMonitor.blockShutdown()
      }
    })
    powerMonitor.on('removeListener', (event: string) => {
      // whenever the listener count is decremented to zero...
      if (event === 'shutdown' && powerMonitor.listenerCount('shutdown') === 0) {
        powerMonitor.unblockShutdown()
      }
    })
  })
}

module.exports = powerMonitor

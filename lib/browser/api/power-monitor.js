'use strict'

const { EventEmitter } = require('events')
const { powerMonitor, PowerMonitor } = process.atomBinding('power_monitor')
const { deprecate } = require('electron')

// PowerMonitor is an EventEmitter.
Object.setPrototypeOf(PowerMonitor.prototype, EventEmitter.prototype)
EventEmitter.call(powerMonitor)

// On Linux we need to call blockShutdown() to subscribe to shutdown event.
if (process.platform === 'linux') {
  powerMonitor.on('newListener', (event) => {
    if (event === 'shutdown' && powerMonitor.listenerCount('shutdown') === 0) {
      powerMonitor.blockShutdown()
    }
  })

  powerMonitor.on('removeListener', (event) => {
    if (event === 'shutdown' && powerMonitor.listenerCount('shutdown') === 0) {
      powerMonitor.unblockShutdown()
    }
  })
}

// TODO(nitsakh): Remove for electron 6.
PowerMonitor.prototype.querySystemIdleState = (idleThreshold, callback) => {
  deprecate.warn('powerMonitor.querySystemIdleState', 'powerMonitor.getSystemIdleState')
  powerMonitor._querySystemIdleState(idleThreshold, callback)
}

PowerMonitor.prototype.querySystemIdleTime = (callback) => {
  deprecate.warn('powerMonitor.querySystemIdleTime', 'powerMonitor.getSystemIdleTime')
  powerMonitor._querySystemIdleTime(callback)
}

module.exports = powerMonitor

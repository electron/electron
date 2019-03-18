'use strict'

const { EventEmitter } = require('events')
const { powerMonitor, PowerMonitor } = process.electronBinding('power_monitor')
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

// TODO(nitsakh): Remove in 7.0
powerMonitor.querySystemIdleState = function (threshold, callback) {
  deprecate.warn('powerMonitor.querySystemIdleState', 'powerMonitor.getSystemIdleState')
  if (typeof threshold !== 'number') {
    throw new Error('Must pass threshold as a number')
  }

  if (typeof callback !== 'function') {
    throw new Error('Must pass callback as a function argument')
  }

  const idleState = this.getSystemIdleState(threshold)

  process.nextTick(() => callback(idleState))
}

// TODO(nitsakh): Remove in 7.0
powerMonitor.querySystemIdleTime = function (callback) {
  deprecate.warn('powerMonitor.querySystemIdleTime', 'powerMonitor.getSystemIdleTime')
  if (typeof callback !== 'function') {
    throw new Error('Must pass function as an argument')
  }

  const idleTime = this.getSystemIdleTime()

  process.nextTick(() => callback(idleTime))
}

module.exports = powerMonitor

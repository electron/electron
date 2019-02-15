'use strict'

const { EventEmitter } = require('events')
const { powerMonitor, PowerMonitor } = process.atomBinding('power_monitor')

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

// TODO(deepak1556): Deprecate async api in favor of sync version in 5.0
powerMonitor.querySystemIdleState = function (threshold, callback) {
  if (typeof threshold !== 'number') {
    throw new Error('Must pass threshold as a number')
  }

  if (typeof callback !== 'function') {
    throw new Error('Must pass callback as a function argument')
  }

  const idleState = this._querySystemIdleState(threshold)

  process.nextTick(() => callback(idleState))
}

// TODO(deepak1556): Deprecate async api in favor of sync version in 5.0
powerMonitor.querySystemIdleTime = function (callback) {
  if (typeof callback !== 'function') {
    throw new Error('Must pass function as an argument')
  }

  const idleTime = this._querySystemIdleTime()

  process.nextTick(() => callback(idleTime))
}

module.exports = powerMonitor

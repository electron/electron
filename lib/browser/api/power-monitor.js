const {EventEmitter} = require('events')
const {powerMonitor, PowerMonitor} = process.atomBinding('power_monitor')

// PowerMonitor is an EventEmitter.
Object.setPrototypeOf(PowerMonitor.prototype, EventEmitter.prototype)
EventEmitter.call(powerMonitor)

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

module.exports = powerMonitor

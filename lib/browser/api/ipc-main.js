const EventEmitter = require('events').EventEmitter

module.exports = new EventEmitter()

const removeAllListeners = module.exports.removeAllListeners
module.exports.removeAllListeners = function (...args) {
  if (args.length === 0) {
    throw new Error('Removing all listeners from ipcMain will make Electron internals stop working.  Please specify a event name')
  }
  removeAllListeners.apply(this, args)
}

// Do not throw exception when channel name is "error".
module.exports.on('error', () => {})

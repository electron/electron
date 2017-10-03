const EventEmitter = require('events').EventEmitter

const emitter = new EventEmitter()

const removeAllListeners = emitter.removeAllListeners.bind(emitter)
emitter.removeAllListeners = function (...args) {
  if (args.length === 0) {
    throw new Error('Removing all listeners from ipcMain will make Electron internals stop working.  Please specify a event name')
  }
  removeAllListeners(...args)
}

// Do not throw exception when channel name is "error".
emitter.on('error', () => {})

module.exports = emitter

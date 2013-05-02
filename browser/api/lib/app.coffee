binding = process.atomBinding 'app'
EventEmitter = require('events').EventEmitter

class App extends EventEmitter
  quit: binding.quit
  terminate: binding.terminate

module.exports = new App

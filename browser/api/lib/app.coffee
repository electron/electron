bindings = process.atomBinding 'app'
EventEmitter = require('events').EventEmitter

Application = bindings.Application
Application::__proto__ = EventEmitter.prototype

app = new Application

app.commandLine =
  appendSwitch: bindings.appendSwitch,
  appendArgument: bindings.appendArgument

# Only one App object pemitted.
module.exports = app

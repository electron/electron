{EventEmitter} = require 'events'
{screen} = process.atomBinding 'screen'

screen.__proto__ = EventEmitter.prototype

module.exports = screen

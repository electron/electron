EventEmitter = require('events').EventEmitter

screen = process.atomBinding('screen').screen
screen.__proto__ = EventEmitter.prototype

module.exports = screen

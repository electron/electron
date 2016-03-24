const EventEmitter = require('events').EventEmitter
const screen = process.atomBinding('screen').screen

screen.__proto__ = EventEmitter.prototype

module.exports = screen

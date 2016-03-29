const EventEmitter = require('events').EventEmitter
const screen = process.atomBinding('screen').screen

Object.setPrototypeOf(screen, EventEmitter.prototype)

module.exports = screen

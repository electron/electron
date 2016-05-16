const {EventEmitter} = require('events')
const {screen} = process.atomBinding('screen')

Object.setPrototypeOf(screen, EventEmitter.prototype)

module.exports = screen

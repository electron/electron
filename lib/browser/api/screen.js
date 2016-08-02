const {EventEmitter} = require('events')
const {screen} = process.atomBinding('screen')

Object.setPrototypeOf(screen.__proto__, EventEmitter.prototype)

module.exports = screen

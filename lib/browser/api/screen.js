const {EventEmitter} = require('events')
const {screen, Screen} = process.atomBinding('screen')

Object.setPrototypeOf(Screen.prototype, EventEmitter.prototype)

module.exports = screen

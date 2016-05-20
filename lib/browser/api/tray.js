const {EventEmitter} = require('events')
const {Tray} = process.atomBinding('tray')

Object.setPrototypeOf(Tray.prototype, EventEmitter.prototype)

module.exports = Tray

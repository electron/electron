'use strict'

const { EventEmitter } = require('events')
const { screen, Screen } = process.electronBinding('screen')

// Screen is an EventEmitter.
Object.setPrototypeOf(Screen.prototype, EventEmitter.prototype)
EventEmitter.call(screen)

module.exports = screen

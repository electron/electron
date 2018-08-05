'use strict'

const { EventEmitter } = require('events')

const emitter = new EventEmitter()

// Do not throw exception when channel name is "error".
emitter.on('error', () => {})

module.exports = emitter

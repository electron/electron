'use strict'

const { EventEmitter } = require('events')
const { deprecate } = require('electron')
const { Tray } = process.electronBinding('tray')

Object.setPrototypeOf(Tray.prototype, EventEmitter.prototype)

module.exports = Tray

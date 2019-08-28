'use strict'

const { EventEmitter } = require('events')
const { deprecate } = require('electron')
const { Tray } = process.electronBinding('tray')

Object.setPrototypeOf(Tray.prototype, EventEmitter.prototype)

deprecate.fnToProperty(Tray.prototype, 'title', '_getTitle', '_setTitle')
deprecate.fnToProperty(Tray.prototype, 'ignoreDoubleClickEvents', '_getIgnoreDoubleClickEvents', '_setIgnoreDoubleClickEvents')

module.exports = Tray

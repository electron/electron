'use strict'

const {EventEmitter} = require('events')
const {TopLevelWindow} = process.atomBinding('top_level_window')
const v8Util = process.atomBinding('v8_util')

Object.setPrototypeOf(TopLevelWindow.prototype, EventEmitter.prototype)

module.exports = TopLevelWindow

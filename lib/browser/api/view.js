'use strict'

const { EventEmitter } = require('events')
const { View } = process.electronBinding('view')

Object.setPrototypeOf(View.prototype, EventEmitter.prototype)

View.prototype._init = function () {
}

module.exports = View

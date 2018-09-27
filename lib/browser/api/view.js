'use strict'

const { EventEmitter } = require('events')
const { View } = process.atomBinding('view')

Object.setPrototypeOf(View.prototype, EventEmitter.prototype)

View.prototype._init = function () {
}

module.exports = View

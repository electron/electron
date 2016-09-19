'use strict'

const {EventEmitter} = require('events')
const binding = process.atomBinding('net')
const {net, Net} = binding
const {URLRequest} = net

Object.setPrototypeOf(Net.prototype, EventEmitter.prototype)
Object.setPrototypeOf(URLRequest.prototype, EventEmitter.prototype)

module.exports = net
 
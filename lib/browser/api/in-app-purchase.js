'use strict'

const {EventEmitter} = require('events')
const {inAppPurchase, InAppPurchase} = process.atomBinding('in_app_purchase')

// inAppPurchase is an EventEmitter.
Object.setPrototypeOf(InAppPurchase.prototype, EventEmitter.prototype)
EventEmitter.call(inAppPurchase)

module.exports = inAppPurchase

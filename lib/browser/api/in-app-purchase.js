'use strict'

if (process.platform !== 'darwin') {
  throw new Error('The inAppPurchase module can only be used on macOS')
}

const {EventEmitter} = require('events')
const {inAppPurchase, InAppPurchase} = process.atomBinding('in_app_purchase')

// inAppPurchase is an EventEmitter.
Object.setPrototypeOf(InAppPurchase.prototype, EventEmitter.prototype)
EventEmitter.call(inAppPurchase)

module.exports = inAppPurchase
